/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
#include "Data/SlicesHandlerITKInterface.h"

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
	FileSystemModel(QObject* parent = nullptr) : QFileSystemModel(parent) {}

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

	FileSystemModel* fs_model = new FileSystemModel(completer);
	completer->setModel(fs_model);
	fs_model->setRootPath("");

	file_path->setCompleter(completer);
}
} // namespace

TraceTubesWidget::TraceTubesWidget(iseg::SlicesHandlerInterface* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler(hand3D)
{
	setToolTip(Format("Trace elongated structures"));

	m_MainOptions = new QWidget;
	{
		m_Metric = new QComboBox;
		m_Metric->insertItem(kIntensity, QString("Intensity"));
		m_Metric->insertItem(kHessian2D, QString("Vesselness 2D"));
		m_Metric->insertItem(kHessian3D, QString("Vesselness 3D"));
		m_Metric->insertItem(kTarget, QString("Target"));
		m_Metric->setCurrentIndex(kHessian2D);
		m_Metric->setToolTip(Format("Intensity sticks to pixels with similar intensity as the start/end points. Vesselness computes a tubular structure measure and sticks to vesselness values similar to start/end."));

		m_IntensityValue = new QLineEdit;
		m_IntensityValue->setValidator(new QDoubleValidator);
		m_IntensityValue->setToolTip(Format("Typical intensity value along path. Call estimate intensity to guess a meaningful value. If empty, the value will be guessed from the selected points."));

		m_IntensityWeight = new QLineEdit(QString::number(1.0 / 255.0));
		m_IntensityWeight->setValidator(new QDoubleValidator(0, 1e6, 6));
		m_IntensityWeight->setToolTip(Format("Weight assigned to intensity differences wrt start/end pixel value."));

		m_AngleWeight = new QLineEdit(QString::number(1));
		m_AngleWeight->setValidator(new QDoubleValidator(0, 1e6, 6));
		m_AngleWeight->setToolTip(Format("Penalty weight for non-smooth paths. If values are too large, piece-wise straight paths will be created."));

		m_LineRadius = new QLineEdit(QString::number(0));
		m_LineRadius->setValidator(new QDoubleValidator(0, 100, 4));

		m_ActiveRegionPadding = new QLineEdit(QString::number(2));
		m_ActiveRegionPadding->setValidator(new QIntValidator(1, 1000, m_ActiveRegionPadding));
		m_ActiveRegionPadding->setToolTip(Format("The region for computing the path is defined by the bounding box of the seed points, including a padding."));

		m_DebugMetricFilePath = new QLineEdit;
		add_completer(m_DebugMetricFilePath, this);
		m_DebugMetricFilePath->setToolTip(Format("In order to debug the metric parameters, you can set a file path to export the metric to."));

		m_ClearPoints = new QPushButton("Clear points");

		m_EstimateIntensity = new QPushButton("Estimate intensity");

		m_ExecuteButton = new QPushButton("Execute");

		auto layout = new QFormLayout();
		layout->addRow(QString("Metric"), m_Metric);
		layout->addRow(QString("Intensity"), m_IntensityValue);
		layout->addRow(QString("Intensity weight"), m_IntensityWeight);
		layout->addRow(QString("Angle weight"), m_AngleWeight);
		layout->addRow(QString("Line radius"), m_LineRadius);
		layout->addRow(QString("Region padding"), m_ActiveRegionPadding);
		layout->addRow(QString("Metric debug file"), m_DebugMetricFilePath);
		layout->addRow(m_ClearPoints);
		layout->addRow(m_EstimateIntensity);
		layout->addRow(m_ExecuteButton);

		m_MainOptions->setLayout(layout);
	}

	m_VesselnessOptions = new QWidget;
	{
		m_Sigma = new QLineEdit(QString::number(0.5));
		m_Sigma->setToolTip(Format("Sigma is the width of a Gaussian smoothing operator used to remove noise. Large values might remove important features."));

		m_DarkObjects = new QCheckBox;
		m_DarkObjects->setChecked(true);
		m_DarkObjects->setToolTip(Format("Enhance dark structures on a bright background if true."));

		m_Alpha = new QLineEdit(QString::number(0.5));
		m_Alpha->setValidator(new QDoubleValidator);
		m_Alpha->setToolTip(Format("Ratio of the smallest eigenvalue that has to be large to the larger ones. "
															 "Smaller values lead to increased sensitivity to the object dimensionality."));

		m_Beta = new QLineEdit(QString::number(0.5));
		m_Beta->setValidator(new QDoubleValidator);
		m_Beta->setToolTip(Format("Ratio of the largest eigenvalue that has to be small to the larger ones. "
															"Smaller values lead to increased sensitivity to the object dimensionality."));

		m_Gamma = new QLineEdit(QString::number(5.0));
		m_Gamma->setValidator(new QDoubleValidator);

		auto layout = new QFormLayout();
		layout->addRow(QString("Sigma"), m_Sigma);
		layout->addRow(QString("Dark objects"), m_DarkObjects);
		layout->addRow(QString("Alpha"), m_Alpha);
		layout->addRow(QString("Beta"), m_Beta);
		layout->addRow(QString("Gamma"), m_Gamma);

		m_VesselnessOptions->setLayout(layout);
	}

	m_TargetOptions = new QWidget;
	{
		m_PathFileName = new QLineEdit;
		add_completer(m_PathFileName, this);

		auto layout = new QFormLayout();
		layout->addRow(QString("Path file name"), m_PathFileName);

		m_TargetOptions->setLayout(layout);
	}

	m_OptionsStack = new QStackedWidget;
	m_OptionsStack->addWidget(m_EmptyOptions = new QWidget);
	m_OptionsStack->addWidget(m_VesselnessOptions);
	m_OptionsStack->addWidget(m_TargetOptions);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidget(m_MainOptions);

	auto top_level_layout = new QHBoxLayout;
	top_level_layout->addWidget(scroll_area);
	top_level_layout->addWidget(m_OptionsStack);

	setLayout(top_level_layout);
	setMinimumWidth(std::max(300, top_level_layout->sizeHint().width()));

	// update advanced options
	OnMetricChanged();

	QObject_connect(m_Metric, SIGNAL(currentIndexChanged(int)), this, SLOT(OnMetricChanged()));
	QObject_connect(m_ClearPoints, SIGNAL(clicked()), this, SLOT(ClearPoints()));
	QObject_connect(m_EstimateIntensity, SIGNAL(clicked()), this, SLOT(EstimateIntensity()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(DoWork()));
}

void TraceTubesWidget::OnMetricChanged()
{
	if (m_Metric->currentIndex() == kIntensity)
	{
		m_OptionsStack->setCurrentWidget(m_EmptyOptions);
	}
	else if (m_Metric->currentIndex() == kTarget)
	{
		m_OptionsStack->setCurrentWidget(m_TargetOptions);
	}
	else // kHessian2D/kHessian3D
	{
		m_OptionsStack->setCurrentWidget(m_VesselnessOptions);
	}

	m_EstimateIntensity->setEnabled(m_Metric->currentIndex() != kTarget);
}

void TraceTubesWidget::OnSlicenrChanged()
{
	UpdatePoints();
}

void TraceTubesWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
	OnMetricChanged();
}

void TraceTubesWidget::NewLoaded()
{
	OnSlicenrChanged();
}

void TraceTubesWidget::Cleanup()
{
	m_Points.clear();
}

std::string TraceTubesWidget::GetName()
{
	return std::string("Trace Tubes");
}

QIcon TraceTubesWidget::GetIcon(QDir picdir)
{
	return QIcon(picdir.absFilePath(QString("Bias.png")).ascii());
}

void TraceTubesWidget::OnMouseReleased(Point x)
{
	auto active_slice = m_Handler->ActiveSlice();
	if (active_slice >= m_Handler->StartSlice() && active_slice < m_Handler->EndSlice())
	{
		iseg::Point3D p;
		p.p = x;
		p.pz = m_Handler->ActiveSlice();
		m_Points.push_back(p);

		UpdatePoints();
	}
	else
	{
		iseg::Log::Warning("no points selected, because slice is not in active slices");
	}
}

void TraceTubesWidget::UpdatePoints()
{
	std::vector<iseg::Point> vp;
	for (auto v : m_Points)
	{
		if (v.pz == m_Handler->ActiveSlice())
			vp.push_back(v.p);
	}
	emit VpdynChanged(&vp);
}

int TraceTubesWidget::GetPadding() const
{
	int pad = m_ActiveRegionPadding->text().toInt();
	auto const line_radius = m_LineRadius->text().toDouble();
	if (line_radius > 0.0)
	{
		auto const spacing = m_Handler->Spacing();
		pad = std::max(pad, static_cast<int>(std::ceil(line_radius / spacing[0])));
		pad = std::max(pad, static_cast<int>(std::ceil(line_radius / spacing[1])));
		pad = std::max(pad, static_cast<int>(std::ceil(line_radius / spacing[2])));
	}
	return pad;
}

itk::Image<float, 3>::Pointer TraceTubesWidget::ComputeVesselness(const itk::ImageBase<3>::RegionType& requested_region) const
{
	using input_type = itk::SliceContiguousImage<float>;
	using hessian_filter_type = itk::HessianRecursiveGaussianImageFilter<input_type>;
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<double, 3>;
	using hessian_image_type = itk::Image<hessian_pixel_type, 3>;
	using image_type = itk::Image<float, 3>;
	using objectness_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, image_type>;

	double sigma = m_Sigma->text().toDouble();
	double alpha = m_Alpha->text().toDouble();
	double beta = m_Beta->text().toDouble();
	double gamma = m_Gamma->text().toDouble();

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler);
	auto source = itk_handler.GetSource(true);

	auto hessian_filter = hessian_filter_type::New();
	hessian_filter->SetInput(source);
	hessian_filter->SetSigma(sigma);

	auto objectness_filter = objectness_filter_type::New();
	objectness_filter->SetInput(hessian_filter->GetOutput());
	objectness_filter->SetBrightObject(!m_DarkObjects->isChecked());
	objectness_filter->SetObjectDimension(1);
	objectness_filter->SetScaleObjectnessMeasure(true);
	objectness_filter->SetAlpha(alpha);
	objectness_filter->SetBeta(beta);
	objectness_filter->SetGamma(gamma);
	objectness_filter->GetOutput()->SetRequestedRegion(requested_region);
	objectness_filter->Update();

	return objectness_filter->GetOutput();
}

itk::Image<float, 3>::Pointer TraceTubesWidget::ComputeBlobiness(const itk::ImageBase<3>::RegionType& requested_region) const
{
	using input_type = itk::SliceContiguousImage<float>;
	using output_type = itk::Image<float, 3>;
	using image_type = itk::Image<float, 2>;

	using hessian_filter_type = itk::HessianRecursiveGaussianImageFilter<image_type>;
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<double, 2>;
	using hessian_image_type = itk::Image<hessian_pixel_type, 2>;

	using objectness_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, image_type>;
	using slice_by_slice_filter_type = itk::SliceBySliceImageFilter<input_type, output_type, hessian_filter_type, objectness_filter_type>;

	double sigma = m_Sigma->text().toDouble();
	double alpha = m_Alpha->text().toDouble();
	double beta = m_Beta->text().toDouble();
	double gamma = m_Gamma->text().toDouble();

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler);
	auto source = itk_handler.GetSource(true);

	auto hessian_filter = hessian_filter_type::New();
	hessian_filter->SetSigma(sigma);

	auto objectness_filter = objectness_filter_type::New();
	objectness_filter->SetInput(hessian_filter->GetOutput());
	objectness_filter->SetBrightObject(!m_DarkObjects->isChecked());
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

itk::Image<float, 3>::Pointer TraceTubesWidget::ComputeObjectSdf(const itk::ImageBase<3>::RegionType& requested_region) const
{
	iseg::SlicesHandlerITKInterface itk_handler(m_Handler);
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

void TraceTubesWidget::ClearPoints()
{
	m_Points.clear();

	UpdatePoints();
}

void TraceTubesWidget::EstimateIntensity()
{
	if (!m_Points.empty())
	{
		using image_type = itk::Image<float, 3>;

		iseg::SlicesHandlerITKInterface itk_handler(m_Handler);
		auto source = itk_handler.GetSource(true);

		double intensity = 0.0;

		for (auto p : m_Points)
		{
			image_type::IndexType idx = {p.px, p.py, p.pz};

			if (m_Metric->currentIndex() == kIntensity)
			{
				intensity += source->GetPixel(idx);
			}
			else
			{
				image_type::SizeType size = {1, 1, 1};
				image_type::RegionType region(idx, size);
				region.PadByRadius(1);
				region.Crop(source->GetLargestPossibleRegion());

				if (m_Metric->currentIndex() == kHessian2D)
				{
					auto speed_image = ComputeBlobiness(region);
					std::cerr << "Value: " << speed_image->GetPixel(idx) << "\n";
					std::cerr << "Region: " << speed_image->GetBufferedRegion() << "\n";
					intensity += speed_image->GetPixel(idx);
				}
				else if (m_Metric->currentIndex() == kHessian3D)
				{
					auto speed_image = ComputeVesselness(region);
					std::cerr << "Value: " << speed_image->GetPixel(idx) << "\n";
					std::cerr << "Region: " << speed_image->GetBufferedRegion() << "\n";
					intensity += speed_image->GetPixel(idx);
				}
			}
		}

		intensity /= m_Points.size();
		m_IntensityValue->setText(QString::number(intensity));
	}
}

void TraceTubesWidget::DoWork()
{
	if (m_Points.size() < 2)
	{
		return;
	}

	using input_type = itk::SliceContiguousImage<float>;

	int pad = GetPadding();
	auto export_file_path = m_DebugMetricFilePath->text().toStdString();

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler);
	auto source = itk_handler.GetSource(true);

	input_type::IndexType idx_lo = {m_Points.front().px, m_Points.front().py, m_Points.front().pz};
	input_type::IndexType idx_hi = idx_lo;
	for (auto p : m_Points)
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

	if (m_Metric->currentIndex() == kIntensity)
	{
		auto speed_image = source;
		DoWorkTemplate<input_type>(speed_image, requested_region);
	}
	else if (m_Metric->currentIndex() == kHessian2D)
	{
		auto speed_image = ComputeBlobiness(requested_region);

		iseg::dump_image<itk::Image<float, 3>>(speed_image, export_file_path);

		DoWorkTemplate<itk::Image<float, 3>>(speed_image, requested_region);
	}
	else if (m_Metric->currentIndex() == kHessian3D)
	{
		auto speed_image = ComputeVesselness(requested_region);

		iseg::dump_image<itk::Image<float, 3>>(speed_image, export_file_path);

		DoWorkTemplate<itk::Image<float, 3>>(speed_image, requested_region);
	}
	else if (m_Metric->currentIndex() == kTarget)
	{
		auto speed_image = ComputeObjectSdf(requested_region);

		iseg::dump_image<itk::Image<float, 3>>(speed_image, export_file_path);

		DoWorkTemplate<itk::Image<float, 3>>(speed_image, requested_region);
	}
}

template<class TSpeedImage>
void TraceTubesWidget::DoWorkTemplate(TSpeedImage* speed_image, const itk::ImageBase<3>::RegionType& requested_region)
{
	using speed_image_type = TSpeedImage;
	using input_type = itk::SliceContiguousImage<float>;
	using output_type = itk::Image<unsigned char, 3>;

	using path_filter_type = itk::WeightedDijkstraImageFilter<speed_image_type>;
	using metric_type = itk::MyMetric<speed_image_type>;

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler);
	auto target = itk_handler.GetTarget(true);

	int pad = GetPadding();
	double intensity_weight = m_IntensityWeight->text().toDouble();
	double angle_weight = m_AngleWeight->text().toDouble();
	double length_weight = 1;
	double bone_length_penalty = 2;		// TODO
	double artery_length_penalty = 2; // TODO
	double line_radius = m_LineRadius->text().toDouble();

	bool has_intensity = false;
	double intensity_value = m_IntensityValue->text().toDouble(&has_intensity);
	if (m_Metric->currentIndex() == kTarget)
	{
		auto calculator = itk::MinimumMaximumImageCalculator<TSpeedImage>::New();
		calculator->SetImage(speed_image);
		calculator->SetRegion(requested_region);
		calculator->ComputeMinimum();
		intensity_value = calculator->GetMinimum();
		has_intensity = true;
	}

	std::vector<typename path_filter_type::PathType::VertexListPointer> paths;

	for (size_t k = 0; k + 1 < m_Points.size(); ++k)
	{
		auto a = m_Points[k];
		auto b = m_Points[k + 1];
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

	for (const auto& verts : paths)
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

	if (!m_PathFileName->text().isEmpty() && m_Metric->currentIndex() == kTarget)
	{
		auto fname = m_PathFileName->text().toStdString();
		std::ofstream ofile(fname, std::ofstream::out);
		if (ofile.is_open())
		{
			for (const auto& verts : paths)
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

	iseg::DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	iseg::Paste<output_type, input_type>(image_with_path, target, requested_region);

	emit EndDatachange(this);
}
