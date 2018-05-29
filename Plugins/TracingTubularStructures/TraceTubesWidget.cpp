/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "TraceTubesWidget.h"

#include "itkWeightedDijkstraImageFilter.h"

#include "Data/BrushInteraction.h"
#include "Data/ItkUtils.h"
#include "Data/SliceHandlerItkWrapper.h"

#include <itkBinaryDilateImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkHessianToObjectnessMeasureImageFilter.h>
//#include <itkMultiScaleHessianBasedMeasureImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>

#include <QComboBox>
#include <QCheckBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QProgressDialog>
#include <QPushButton>

using iseg::Point;

TraceTubesWidget::TraceTubesWidget(iseg::SliceHandlerInterface* hand3D,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), _handler(hand3D)
{
	setToolTip(Format("Trace elongated structures"));

	_active_slice = _handler->active_slice();

	_metric = new QComboBox;
	_metric->insertItem(QString("Intensity"));
	_metric->insertItem(QString("Vesselness"));
	_metric->setCurrentIndex(0);

	_intensity_weight = new QLineEdit(QString::number(1.0 / 255.0));
	_intensity_weight->setValidator(new QDoubleValidator(0, 1e6, 4));
	_intensity_weight->setToolTip(Format("Weight assigned to intensity differences wrt start/end pixel value."));

	_angle_weight = new QLineEdit(QString::number(1));
	_angle_weight->setValidator(new QDoubleValidator(0, 1e6, 4));
	_angle_weight->setToolTip(Format("Penalty weight for non-smooth paths. If values are too large, piece-wise straight paths will be created."));

	_line_radius = new QLineEdit(QString::number(1));
	_line_radius->setValidator(new QDoubleValidator(0, 100, 4));

	_active_region_padding = new QLineEdit(QString::number(1));
	_active_region_padding->setValidator(new QIntValidator(0, 1000, _active_region_padding));
	_active_region_padding->setToolTip(Format("The region for computing the path is defined by the bounding box of the seed points, including a padding."));

	_clear_points = new QPushButton("Clear points");

	_execute_button = new QPushButton("Execute");

	auto layout = new QFormLayout();
	layout->addRow(QString("Metric"), _metric);
	layout->addRow(QString("Intensity weight"), _intensity_weight);
	layout->addRow(QString("Angle weight"), _angle_weight);
	layout->addRow(QString("Line radius"), _line_radius);
	layout->addRow(QString("Region padding"), _active_region_padding);
	layout->addRow(_clear_points);
	layout->addRow(_execute_button);

	_main_options = new QWidget;
	_main_options->setLayout(layout);

	_sigma = new QLineEdit(QString::number(1.0));

	_dark_objects = new QCheckBox;
	_dark_objects->setChecked(true);

	auto alayout = new QFormLayout();
	alayout->addRow(QString("Sigma"), _sigma);
	alayout->addRow(QString("Dark objects"), _dark_objects);

	_vesselness_options = new QWidget;
	_vesselness_options->setLayout(alayout);

	auto hlayout = new QHBoxLayout;
	hlayout->addWidget(_main_options);
	hlayout->addWidget(_vesselness_options);

	setLayout(hlayout);
	setMinimumWidth(std::max(300, hlayout->sizeHint().width()));

	//QObject::connect(_intensity_weight, SIGNAL(textChanged(const QString&)), this, SLOT(update_radius()));
	QObject::connect(_metric, SIGNAL(currentIndexChanged(int)), this, SLOT(on_metric_changed()));
	QObject::connect(_clear_points, SIGNAL(clicked()), this, SLOT(clear_points()));
	QObject::connect(_execute_button, SIGNAL(clicked()), this, SLOT(do_work()));
}

void TraceTubesWidget::on_metric_changed()
{
	_vesselness_options->setVisible(_metric->currentIndex() != 0);
}

void TraceTubesWidget::on_slicenr_changed()
{
	_active_slice = _handler->active_slice();

	update_points();
}

void TraceTubesWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
	on_metric_changed();

	//if (!_brush)
	//{
	//	_brush = new iseg::BrushInteraction(_handler,
	//			[this](iseg::DataSelection sel) { begin_datachange(sel, this); },
	//			[this](iseg::EndUndoAction a) { end_datachange(this, a); });
	//}
	//else
	//{
	//	_brush->init(_handler);
	//}
}

void TraceTubesWidget::newloaded()
{
	on_slicenr_changed();
}

void TraceTubesWidget::cleanup()
{
	_points.clear();
}

std::string TraceTubesWidget::GetName()
{
	return std::string("Trace Tubes");
}

QIcon TraceTubesWidget::GetIcon(QDir picdir)
{
	return QIcon(picdir.absFilePath(QString("Bias.png")).ascii());
}

void TraceTubesWidget::on_mouse_clicked(Point p)
{
	//_brush->on_mouse_clicked(p);
}

void TraceTubesWidget::on_mouse_released(Point x)
{
	//_brush->on_mouse_released(p);

	iseg::Point3D p;
	p.p = x;
	p.pz = _active_slice;
	_points.push_back(p);

	update_points();
}

void TraceTubesWidget::on_mouse_moved(Point p)
{
	//_brush->on_mouse_moved(p);
}

void TraceTubesWidget::keyReleaseEvent(QKeyEvent* key)
{
	//if (key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return)
	//{
	//	key->accept();
	//}
}

void TraceTubesWidget::update_points()
{
	std::vector<iseg::Point> vp;
	for (auto v : _points)
	{
		if (v.pz == _active_slice)
			vp.push_back(v.p);
	}
	emit vpdyn_changed(&vp);
}

void TraceTubesWidget::clear_points()
{
	_points.clear();

	update_points();
}

void TraceTubesWidget::do_work()
{
	if (_points.size() < 2)
	{
		return;
	}

	using input_type = itk::SliceContiguousImage<float>;

	int pad = _active_region_padding->text().toInt();

	iseg::SliceHandlerItkWrapper itk_handler(_handler);
	auto source = itk_handler.GetImage(iseg::SliceHandlerItkWrapper::kSource, true);

	input_type::IndexType idx_lo = {_points.front().px, _points.front().py, _points.front().pz};
	input_type::IndexType idx_hi = idx_lo;
	for (auto p : _points)
	{
		idx_lo[0] = std::min<input_type::IndexValueType>(idx_lo[0], p.px);
		idx_lo[1] = std::min<input_type::IndexValueType>(idx_lo[1], p.py);
		idx_lo[2] = std::min<input_type::IndexValueType>(idx_lo[2], p.pz);

		idx_hi[0] = std::max<input_type::IndexValueType>(idx_hi[0], p.px);
		idx_hi[1] = std::max<input_type::IndexValueType>(idx_hi[1], p.py);
		idx_hi[2] = std::max<input_type::IndexValueType>(idx_hi[2], p.pz);
	}
	input_type::RegionType requested_region;
	requested_region.SetIndex(idx_lo);
	requested_region.SetUpperIndex(idx_hi);
	requested_region.PadByRadius(pad);
	requested_region.Crop(source->GetLargestPossibleRegion());

	if (_metric->currentIndex() == 0)
	{
		auto speed_image = source;
		do_work_template<input_type>(speed_image, requested_region);
	}
	else
	{
		using hessian_filter_type = itk::HessianRecursiveGaussianImageFilter<input_type>;
		using hessian_pixel_type = itk::SymmetricSecondRankTensor<double, 3>;
		using hessian_image_type = itk::Image<hessian_pixel_type, 3>;
		using image_type = itk::Image<float,3>;
		using objectness_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, image_type>;

		double sigma = _sigma->text().toDouble();

		auto hessianFilter = hessian_filter_type::New();
		hessianFilter->SetInput(source);
		hessianFilter->SetSigma( sigma );

		auto objectnessFilter = objectness_filter_type::New();
		objectnessFilter->SetInput(hessianFilter->GetOutput());
		objectnessFilter->SetBrightObject( !_dark_objects->isChecked() );
		objectnessFilter->SetObjectDimension(1);
		objectnessFilter->SetScaleObjectnessMeasure( true );
		objectnessFilter->SetAlpha( 0.5 ); // default 0.5
		objectnessFilter->SetBeta( 0.5 ); // default 0.5
		objectnessFilter->SetGamma( 5.0 ); // default 5
		objectnessFilter->GetOutput()->SetRequestedRegion(requested_region);
		objectnessFilter->Update();

		auto speed_image = objectnessFilter->GetOutput();
		do_work_template<image_type>(speed_image, requested_region);
  	}
}

template<class TSpeedImage>
void TraceTubesWidget::do_work_template(TSpeedImage* speed_image, const itk::ImageBase<3>::RegionType& requested_region)
{
	using speed_image_type = TSpeedImage;
	using input_type = itk::SliceContiguousImage<float>;
	using output_type = itk::Image<unsigned char, 3>;

	using path_filter_type = itk::WeightedDijkstraImageFilter<speed_image_type>;
	using metric_type = itk::MyMetric<speed_image_type>;

	iseg::SliceHandlerItkWrapper itk_handler(_handler);
	auto target = itk_handler.GetImage(iseg::SliceHandlerItkWrapper::kTarget, true);

	int pad = _active_region_padding->text().toInt();
	double intensity_weight = _intensity_weight->text().toDouble();
	double angle_weight = _angle_weight->text().toDouble();
	double length_weight = 1;
	double bone_length_penalty = 2;		// TODO
	double artery_length_penalty = 2; // TODO
	double line_radius = _line_radius->text().toDouble();

	std::vector<typename path_filter_type::PathType::VertexListPointer> paths;

	for (size_t k = 0; k + 1 < _points.size(); ++k)
	{
		auto a = _points[k];
		auto b = _points[k + 1];
		input_type::IndexType aidx = {a.px, a.py, a.pz};
		input_type::IndexType bidx = {b.px, b.py, b.pz};

		input_type::RegionType region;
		for (int i = 0; i < 3; i++)
		{
			region.SetIndex(i, std::min(aidx[i], bidx[i]));
			region.SetSize(i, 1 + std::max(aidx[i], bidx[i]) - region.GetIndex(i));
		}
		region.PadByRadius(pad);
		region.Crop(speed_image->GetLargestPossibleRegion());

		auto dijkstra = path_filter_type::New();
		dijkstra->SetInput(speed_image);
		dijkstra->SetStartIndex(aidx);
		dijkstra->SetEndIndex(bidx);
		dijkstra->SetRegion(region);
		dijkstra->GetMetric().m_IntensityWeight = intensity_weight;
		dijkstra->GetMetric().m_AngleWeight = angle_weight;
		dijkstra->GetMetric().m_LengthWeight = length_weight;
		dijkstra->Update();

		auto path = dijkstra->GetOutput(0);
		auto verts = path->GetVertexList();
		paths.push_back(verts);
	}

	// voxelize line
	auto image_with_path = output_type::New();
	image_with_path->SetBufferedRegion(requested_region);
	image_with_path->SetLargestPossibleRegion(speed_image->GetLargestPossibleRegion());
	image_with_path->SetSpacing(speed_image->GetSpacing());
	image_with_path->Allocate(true);

	for (auto verts : paths)
	{
		for (auto v : *verts)
		{
			output_type::IndexType idx;
			idx[0] = v[0];
			idx[1] = v[1];
			idx[2] = v[2];
			image_with_path->SetPixel(idx, 255);
		}
	}

	if (line_radius > 0)
	{
		auto ball = iseg::MakeBall<bool, 3>(speed_image->GetSpacing(), line_radius);
		auto se = itk::FlatStructuringElement<3>::FromImage(ball);

		auto dilate = itk::BinaryDilateImageFilter<output_type, output_type, itk::FlatStructuringElement<3>>::New();
		dilate->SetInput(image_with_path);
		dilate->SetKernel(se);
		dilate->SetDilateValue(255);
		dilate->GetOutput()->SetRequestedRegion(requested_region);
		dilate->Update();

		image_with_path = dilate->GetOutput();
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	iseg::Paste<output_type, input_type>(image_with_path, target, requested_region);

	emit end_datachange(this);
}
