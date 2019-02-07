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
#include "Data/LogApi.h"
#include "Data/SliceHandlerItkWrapper.h"

#include <itkBinaryThresholdImageFilter.h>
#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>
#include <itkHessianToObjectnessMeasureImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkSignedDanielssonDistanceMapImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <itkSliceBySliceImageFilter.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDoubleValidator>
#include <QFileSystemModel>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>

#include <boost/format.hpp>

using iseg::Point;

namespace {
class FileSystemModel : public QFileSystemModel
{
public:
	FileSystemModel(QObject* parent = 0) : QFileSystemModel(parent) {}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (role == Qt::DisplayRole && index.column() == 0)
		{
			QString path = QDir::toNativeSeparators(filePath(index));
			if (path.endsWith(QDir::separator()))
				path.chop(1);
			return path;
		}

		return QFileSystemModel::data(index, role);
	}
};

void add_completer(QLineEdit* file_path, QWidget* parent)
{
	auto completer = new QCompleter(parent);
	completer->setMaxVisibleItems(10);

	FileSystemModel* fsModel = new FileSystemModel(completer);
	completer->setModel(fsModel);
	fsModel->setRootPath("");

	file_path->setCompleter(completer);
}
} // namespace

TraceTubesWidget::TraceTubesWidget(iseg::SliceHandlerInterface* hand3D,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), _handler(hand3D)
{
	setToolTip(Format("Trace elongated structures"));

	_main_options = new QWidget;
	{
		_metric = new QComboBox;
		_metric->insertItem(kIntensity, QString("Intensity"));
		_metric->insertItem(kHessian2D, QString("Vesselness 2D"));
		_metric->insertItem(kHessian3D, QString("Vesselness 3D"));
		_metric->insertItem(kTarget, QString("Target"));
		_metric->setCurrentIndex(kHessian2D);
		_metric->setToolTip(Format("Intensity sticks to pixels with similar intensity as the start/end points. Vesselness computes a tubular structure measure and sticks to vesselness values similar to start/end."));

		_intensity_value = new QLineEdit;
		_intensity_value->setValidator(new QDoubleValidator);
		_intensity_value->setToolTip(Format("Typical intensity value along path. Call estimate intensity to guess a meaningful value. If empty, the value will be guessed from the selected points."));

		_intensity_weight = new QLineEdit(QString::number(1.0 / 255.0));
		_intensity_weight->setValidator(new QDoubleValidator(0, 1e6, 6));
		_intensity_weight->setToolTip(Format("Weight assigned to intensity differences wrt start/end pixel value."));

		_angle_weight = new QLineEdit(QString::number(1));
		_angle_weight->setValidator(new QDoubleValidator(0, 1e6, 6));
		_angle_weight->setToolTip(Format("Penalty weight for non-smooth paths. If values are too large, piece-wise straight paths will be created."));

		_line_radius = new QLineEdit(QString::number(0));
		_line_radius->setValidator(new QDoubleValidator(0, 100, 4));

		_active_region_padding = new QLineEdit(QString::number(2));
		_active_region_padding->setValidator(new QIntValidator(1, 1000, _active_region_padding));
		_active_region_padding->setToolTip(Format("The region for computing the path is defined by the bounding box of the seed points, including a padding."));

		_debug_metric_file_path = new QLineEdit;
		add_completer(_debug_metric_file_path, this);
		_debug_metric_file_path->setToolTip(Format("In order to debug the metric parameters, you can set a file path to export the metric to."));

		_clear_points = new QPushButton("Clear points");

		_estimate_intensity = new QPushButton("Estimate intensity");

		_execute_button = new QPushButton("Execute");

		auto layout = new QFormLayout();
		layout->addRow(QString("Metric"), _metric);
		layout->addRow(QString("Intensity"), _intensity_value);
		layout->addRow(QString("Intensity weight"), _intensity_weight);
		layout->addRow(QString("Angle weight"), _angle_weight);
		layout->addRow(QString("Line radius"), _line_radius);
		layout->addRow(QString("Region padding"), _active_region_padding);
		layout->addRow(QString("Metric debug file"), _debug_metric_file_path);
		layout->addRow(_clear_points);
		layout->addRow(_estimate_intensity);
		layout->addRow(_execute_button);

		_main_options->setLayout(layout);
	}

	_vesselness_options = new QWidget;
	{
		_sigma = new QLineEdit(QString::number(0.5));
		_sigma->setToolTip(Format("Sigma is the width of a Gaussian smoothing operator used to remove noise. Large values might remove important features."));

		_dark_objects = new QCheckBox;
		_dark_objects->setChecked(true);
		_dark_objects->setToolTip(Format("Enhance dark structures on a bright background if true."));

		_alpha = new QLineEdit(QString::number(0.5));
		_alpha->setValidator(new QDoubleValidator);
		_alpha->setToolTip(Format(
				"Ratio of the smallest eigenvalue that has to be large to the larger ones. "
				"Smaller values lead to increased sensitivity to the object dimensionality."));

		_beta = new QLineEdit(QString::number(0.5));
		_beta->setValidator(new QDoubleValidator);
		_beta->setToolTip(Format(
				"Ratio of the largest eigenvalue that has to be small to the larger ones. "
				"Smaller values lead to increased sensitivity to the object dimensionality."));

		_gamma = new QLineEdit(QString::number(5.0));
		_gamma->setValidator(new QDoubleValidator);

		auto layout = new QFormLayout();
		layout->addRow(QString("Sigma"), _sigma);
		layout->addRow(QString("Dark objects"), _dark_objects);
		layout->addRow(QString("Alpha"), _alpha);
		layout->addRow(QString("Beta"), _beta);
		layout->addRow(QString("Gamma"), _gamma);

		_vesselness_options->setLayout(layout);
	}

	_target_options = new QWidget;
	{
		_path_file_name = new QLineEdit;
		add_completer(_path_file_name, this);

		auto layout = new QFormLayout();
		layout->addRow(QString("Path file name"), _path_file_name);

		_target_options->setLayout(layout);
	}

	_options_stack = new QStackedWidget;
	_options_stack->addWidget(_empty_options = new QWidget);
	_options_stack->addWidget(_vesselness_options);
	_options_stack->addWidget(_target_options);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidget(_main_options);

	auto top_level_layout = new QHBoxLayout;
	top_level_layout->addWidget(scroll_area);
	top_level_layout->addWidget(_options_stack);

	setLayout(top_level_layout);
	setMinimumWidth(std::max(300, top_level_layout->sizeHint().width()));

	// update advanced options
	on_metric_changed();

	QObject::connect(_metric, SIGNAL(currentIndexChanged(int)), this, SLOT(on_metric_changed()));
	QObject::connect(_clear_points, SIGNAL(clicked()), this, SLOT(clear_points()));
	QObject::connect(_estimate_intensity, SIGNAL(clicked()), this, SLOT(estimate_intensity()));
	QObject::connect(_execute_button, SIGNAL(clicked()), this, SLOT(do_work()));
}

void TraceTubesWidget::on_metric_changed()
{
	if (_metric->currentIndex() == kIntensity)
	{
		_options_stack->setCurrentWidget(_empty_options);
	}
	else if (_metric->currentIndex() == kTarget)
	{
		_options_stack->setCurrentWidget(_target_options);
	}
	else // kHessian2D/kHessian3D
	{
		_options_stack->setCurrentWidget(_vesselness_options);
	}

	_estimate_intensity->setEnabled(_metric->currentIndex() != kTarget);
}

void TraceTubesWidget::on_slicenr_changed()
{
	update_points();
}

void TraceTubesWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
	on_metric_changed();
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

void TraceTubesWidget::on_mouse_released(Point x)
{
	auto active_slice = _handler->active_slice();
	if (active_slice >= _handler->start_slice() && active_slice < _handler->end_slice())
	{
		iseg::Point3D p;
		p.p = x;
		p.pz = _handler->active_slice();
		_points.push_back(p);

		update_points();
	}
	else
	{
		iseg::Log::warning("no points selected, because slice is not in active slices");
	}
}

void TraceTubesWidget::update_points()
{
	std::vector<iseg::Point> vp;
	for (auto v : _points)
	{
		if (v.pz == _handler->active_slice())
			vp.push_back(v.p);
	}
	emit vpdyn_changed(&vp);
}

int TraceTubesWidget::get_padding() const
{
	int pad = _active_region_padding->text().toInt();
	auto const line_radius = _line_radius->text().toDouble();
	if (line_radius > 0.0)
	{
		auto const spacing = _handler->spacing();
		pad = std::max(pad, static_cast<int>(std::ceil(line_radius / spacing[0])));
		pad = std::max(pad, static_cast<int>(std::ceil(line_radius / spacing[1])));
		pad = std::max(pad, static_cast<int>(std::ceil(line_radius / spacing[2])));
	}
	return pad;
}

itk::Image<float, 3>::Pointer TraceTubesWidget::compute_vesselness(const itk::ImageBase<3>::RegionType& requested_region) const
{
	using input_type = itk::SliceContiguousImage<float>;
	using hessian_filter_type = itk::HessianRecursiveGaussianImageFilter<input_type>;
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<double, 3>;
	using hessian_image_type = itk::Image<hessian_pixel_type, 3>;
	using image_type = itk::Image<float, 3>;
	using objectness_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, image_type>;

	double sigma = _sigma->text().toDouble();
	double alpha = _alpha->text().toDouble();
	double beta = _beta->text().toDouble();
	double gamma = _gamma->text().toDouble();

	iseg::SliceHandlerItkWrapper itk_handler(_handler);
	auto source = itk_handler.GetSource(true);

	auto hessian_filter = hessian_filter_type::New();
	hessian_filter->SetInput(source);
	hessian_filter->SetSigma(sigma);

	auto objectness_filter = objectness_filter_type::New();
	objectness_filter->SetInput(hessian_filter->GetOutput());
	objectness_filter->SetBrightObject(!_dark_objects->isChecked());
	objectness_filter->SetObjectDimension(1);
	objectness_filter->SetScaleObjectnessMeasure(true);
	objectness_filter->SetAlpha(alpha);
	objectness_filter->SetBeta(beta);
	objectness_filter->SetGamma(gamma);
	objectness_filter->GetOutput()->SetRequestedRegion(requested_region);
	objectness_filter->Update();

	return objectness_filter->GetOutput();
}

itk::Image<float, 3>::Pointer TraceTubesWidget::compute_blobiness(const itk::ImageBase<3>::RegionType& requested_region) const
{
	using input_type = itk::SliceContiguousImage<float>;
	using output_type = itk::Image<float, 3>;
	using image_type = itk::Image<float, 2>;

	using hessian_filter_type = itk::HessianRecursiveGaussianImageFilter<image_type>;
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<double, 2>;
	using hessian_image_type = itk::Image<hessian_pixel_type, 2>;

	using objectness_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, image_type>;
	using slice_by_slice_filter_type = itk::SliceBySliceImageFilter<input_type, output_type, hessian_filter_type, objectness_filter_type>;

	double sigma = _sigma->text().toDouble();
	double alpha = _alpha->text().toDouble();
	double beta = _beta->text().toDouble();
	double gamma = _gamma->text().toDouble();

	iseg::SliceHandlerItkWrapper itk_handler(_handler);
	auto source = itk_handler.GetSource(true);

	auto hessian_filter = hessian_filter_type::New();
	hessian_filter->SetSigma(sigma);

	auto objectness_filter = objectness_filter_type::New();
	objectness_filter->SetInput(hessian_filter->GetOutput());
	objectness_filter->SetBrightObject(!_dark_objects->isChecked());
	objectness_filter->SetObjectDimension(0);
	objectness_filter->SetScaleObjectnessMeasure(true);
	objectness_filter->SetAlpha(alpha);
	objectness_filter->SetBeta(beta);
	objectness_filter->SetGamma(gamma);

	auto slice_filter = slice_by_slice_filter_type::New();
	slice_filter->SetInput(source);
	slice_filter->SetInputFilter(hessian_filter);
	slice_filter->SetOutputFilter(objectness_filter);
	slice_filter->GetOutput()->SetRequestedRegion(requested_region);
	slice_filter->Update();

	return slice_filter->GetOutput();
}

itk::Image<float, 3>::Pointer TraceTubesWidget::compute_object_sdf(const itk::ImageBase<3>::RegionType& requested_region) const
{
	iseg::SliceHandlerItkWrapper itk_handler(_handler);
	auto target = itk_handler.GetTarget(true);

	using input_type = itk::SliceContiguousImage<float>;
	using mask_type = itk::Image<unsigned char, 3>;
	using real_type = itk::Image<float, 3>;
	using roi_filter_type = itk::RegionOfInterestImageFilter<input_type, mask_type>;
	using threshold_filter_type = itk::BinaryThresholdImageFilter<mask_type, mask_type>;
	using distance_filter_type = itk::SignedDanielssonDistanceMapImageFilter<mask_type, real_type>;

	// it seems the distance transform always runs on whole image, which is slow
	// therefore, I extract ROI and afterwards graft the output to fake a bufferedregion
	// with correct start index.
	auto roi_filter = roi_filter_type::New();
	roi_filter->SetInput(target);
	roi_filter->SetRegionOfInterest(requested_region);

	auto threshold = threshold_filter_type::New();
	threshold->SetInput(roi_filter->GetOutput());
	threshold->SetLowerThreshold(1);
	threshold->SetInsideValue(255);
	threshold->SetOutsideValue(0);

	auto distance = distance_filter_type::New();
	distance->SetInput(threshold->GetOutput());
	distance->SetUseImageSpacing(true);
	distance->SetSquaredDistance(false);
	distance->Update();

	auto output = real_type::New();
	output->Graft(distance->GetOutput());
	output->SetLargestPossibleRegion(target->GetLargestPossibleRegion());
	output->SetBufferedRegion(requested_region);
	return output;
}

void TraceTubesWidget::clear_points()
{
	_points.clear();

	update_points();
}

void TraceTubesWidget::estimate_intensity()
{
	if (_points.size() > 0)
	{
		using image_type = itk::Image<float, 3>;

		iseg::SliceHandlerItkWrapper itk_handler(_handler);
		auto source = itk_handler.GetSource(true);

		double intensity = 0.0;

		for (auto p : _points)
		{
			image_type::IndexType idx = {p.px, p.py, p.pz};

			if (_metric->currentIndex() == kIntensity)
			{
				intensity += source->GetPixel(idx);
			}
			else
			{
				image_type::SizeType size = {1, 1, 1};
				image_type::RegionType region(idx, size);
				region.PadByRadius(1);
				region.Crop(source->GetLargestPossibleRegion());

				if (_metric->currentIndex() == kHessian2D)
				{
					auto speed_image = compute_blobiness(region);
					std::cerr << "Value: " << speed_image->GetPixel(idx) << "\n";
					std::cerr << "Region: " << speed_image->GetBufferedRegion() << "\n";
					intensity += speed_image->GetPixel(idx);
				}
				else if (_metric->currentIndex() == kHessian3D)
				{
					auto speed_image = compute_vesselness(region);
					std::cerr << "Value: " << speed_image->GetPixel(idx) << "\n";
					std::cerr << "Region: " << speed_image->GetBufferedRegion() << "\n";
					intensity += speed_image->GetPixel(idx);
				}
			}
		}

		intensity /= _points.size();
		_intensity_value->setText(QString::number(intensity));
	}
}

void TraceTubesWidget::do_work()
{
	if (_points.size() < 2)
	{
		return;
	}

	using input_type = itk::SliceContiguousImage<float>;

	int pad = get_padding();
	auto export_file_path = _debug_metric_file_path->text().toStdString();

	iseg::SliceHandlerItkWrapper itk_handler(_handler);
	auto source = itk_handler.GetSource(true);

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

	if (_metric->currentIndex() == kIntensity)
	{
		auto speed_image = source;
		do_work_template<input_type>(speed_image, requested_region);
	}
	else if (_metric->currentIndex() == kHessian2D)
	{
		auto speed_image = compute_blobiness(requested_region);

		iseg::dump_image<itk::Image<float, 3>>(speed_image, export_file_path);

		do_work_template<itk::Image<float, 3>>(speed_image, requested_region);
	}
	else if (_metric->currentIndex() == kHessian3D)
	{
		auto speed_image = compute_vesselness(requested_region);

		iseg::dump_image<itk::Image<float, 3>>(speed_image, export_file_path);

		do_work_template<itk::Image<float, 3>>(speed_image, requested_region);
	}
	else if (_metric->currentIndex() == kTarget)
	{
		auto speed_image = compute_object_sdf(requested_region);

		iseg::dump_image<itk::Image<float, 3>>(speed_image, export_file_path);

		do_work_template<itk::Image<float, 3>>(speed_image, requested_region);
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
	auto target = itk_handler.GetTarget(true);

	int pad = get_padding();
	double intensity_weight = _intensity_weight->text().toDouble();
	double angle_weight = _angle_weight->text().toDouble();
	double length_weight = 1;
	double bone_length_penalty = 2;		// TODO
	double artery_length_penalty = 2; // TODO
	double line_radius = _line_radius->text().toDouble();

	bool has_intensity = false;
	double intensity_value = _intensity_value->text().toDouble(&has_intensity);
	if (_metric->currentIndex() == kTarget)
	{
		auto calculator = itk::MinimumMaximumImageCalculator<TSpeedImage>::New();
		calculator->SetImage(speed_image);
		calculator->SetRegion(requested_region);
		calculator->ComputeMinimum();
		intensity_value = calculator->GetMinimum();
		has_intensity = true;
	}

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
		dijkstra->Metric().m_IntensityWeight = intensity_weight;
		dijkstra->Metric().m_AngleWeight = angle_weight;
		dijkstra->Metric().m_LengthWeight = length_weight;
		if (has_intensity)
		{
			dijkstra->Metric().m_InitIntensity = false;
			dijkstra->Metric().m_StartValue = intensity_value;
			dijkstra->Metric().m_EndValue = intensity_value;
		}
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
	image_with_path->Allocate();
	image_with_path->FillBuffer(0);

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
		using real_type = itk::Image<float, 3>;
		using distance_type = itk::DanielssonDistanceMapImageFilter<output_type, real_type>;
		using threshold_type = itk::BinaryThresholdImageFilter<real_type, output_type>;

		auto distance = distance_type::New();
		distance->SetInput(image_with_path);
		distance->SetInputIsBinary(true);
		distance->SetUseImageSpacing(true);
		distance->SetSquaredDistance(true);

		auto threshold = threshold_type::New();
		threshold->SetInput(distance->GetOutput());
		threshold->SetUpperThreshold(line_radius * line_radius);
		threshold->SetInsideValue(255);
		threshold->SetOutsideValue(0);
		threshold->GetOutput()->SetRequestedRegion(requested_region);
		threshold->Update();

		image_with_path = threshold->GetOutput();
	}

	if (!_path_file_name->text().isEmpty() && _metric->currentIndex() == kTarget)
	{
		auto fname = _path_file_name->text().toStdString();
		std::ofstream ofile(fname, std::ofstream::out);
		if (ofile.is_open())
		{
			for (auto verts : paths)
			{
				for (auto v : *verts)
				{
					itk::Point<double> p;
					target->TransformContinuousIndexToPhysicalPoint(v, p);
					ofile << p[0] << ", " << p[1] << ", " << p[2] << "\n";
				}
			}
			ofile.close();
		}
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	iseg::Paste<output_type, input_type>(image_with_path, target, requested_region);

	emit end_datachange(this);
}
