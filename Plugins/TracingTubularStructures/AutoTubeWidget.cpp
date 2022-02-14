/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 *
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 *
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "AutoTubeWidget.h"

#include "Data/ItkUtils.h"
#include "Data/Logger.h"
#include "Data/SlicesHandlerITKInterface.h"

#include "Thirdparty/IJ/BinaryThinningImageFilter3D/itkBinaryThinningImageFilter3D.h"
#include "Thirdparty/IJ/NonMaxSuppression/itkNonMaxSuppressionImageFilter.h"

#include <itkBinaryThinningImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkCurvesLevelSetImageFilter.h>
#include <itkFastMarchingImageFilter.h>
#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>
#include <itkHessianToObjectnessMeasureImageFilter.h>
#include <itkImage.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkMultiScaleHessianBasedMeasureImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkSigmoidImageFilter.h>
#include <itkSliceBySliceImageFilter.h>
#include <itkThresholdImageFilter.h>

#include <accumulators/percentile.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <QFormLayout>
#include <QMessageBox>
#include <QScrollArea>

#include <algorithm>
#include <sstream>

namespace acc = boost::accumulators;
using boost::adaptors::transformed;
using boost::algorithm::join;

template<typename TInputImage, typename TOutputImage, unsigned int Dimension>
class BinaryThinningImageFilter : public itk::BinaryThinningImageFilter<TInputImage, TOutputImage>
{
};

template<typename TInputImage, typename TOutputImage>
class BinaryThinningImageFilter<TInputImage, TOutputImage, 3> : public itk::BinaryThinningImageFilter3D<TInputImage, TOutputImage>
{
};

AutoTubeWidget::AutoTubeWidget(iseg::SlicesHandlerInterface* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Sandbox Tool"));

	m_Metric2d = new QCheckBox;
	m_Metric2d->setChecked(false);
	m_Metric2d->setToolTip(Format("Use 2D pseudo vesselness (blob feature per slice), or use full 3D vesselness."));

	m_NonMaxSuppression = new QCheckBox;
	m_NonMaxSuppression->setChecked(true);
	m_NonMaxSuppression->setToolTip(Format("Extract approx. one pixel wide paths based on non-maximum suppression."));

	m_Skeletonize = new QCheckBox;
	m_Skeletonize->setChecked(true);
	m_Skeletonize->setToolTip(Format("Compute 1-pixel wide centerlines (skeleton)."));

	m_SigmaLow = new QLineEdit(QString::number(0.3));
	m_SigmaLow->setValidator(new QDoubleValidator);

	m_SigmaHi = new QLineEdit(QString::number(0.6));
	m_SigmaHi->setValidator(new QDoubleValidator);

	m_NumberSigmaLevels = new QLineEdit(QString::number(2));
	m_NumberSigmaLevels->setValidator(new QIntValidator);

	m_Threshold = new QLineEdit;
	m_Threshold->setValidator(new QDoubleValidator);

	m_MaxRadius = new QLineEdit(QString::number(1));
	m_MaxRadius->setValidator(new QDoubleValidator);

	m_MinObjectSize = new QLineEdit(QString::number(10));
	m_MinObjectSize->setValidator(new QIntValidator);

	m_SelectObjectsButton = new QPushButton("Select Mask");
	m_SelectedObjects = new QLineEdit;
	m_SelectedObjects->setReadOnly(true);

	m_ExecuteButton = new QPushButton("Execute");

	auto layout = new QFormLayout;
	layout->addRow("Sigma Min", m_SigmaLow);
	layout->addRow("Sigma Max", m_SigmaHi);
	layout->addRow("Number of Sigmas", m_NumberSigmaLevels);
	layout->addRow("2D Vesselness", m_Metric2d);
	layout->addRow("Feature Threshold", m_Threshold);
	layout->addRow("Non-maximum Suppression", m_NonMaxSuppression);
	layout->addRow("Centerlines", m_Skeletonize);
	//layout->addRow("Maximum radius", _max_radius);
	layout->addRow("Minimum object size", m_MinObjectSize);
	layout->addRow(m_SelectObjectsButton, m_SelectedObjects);
	layout->addRow(m_ExecuteButton);

	auto big_view = new QWidget;
	big_view->setLayout(layout);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidget(big_view);

	auto top_layout = new QGridLayout;
	top_layout->addWidget(scroll_area, 0, 0);
	setLayout(top_layout);

	QObject_connect(m_SelectObjectsButton, SIGNAL(clicked()), this, SLOT(SelectObjects()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(DoWork()));
}

void AutoTubeWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void AutoTubeWidget::NewLoaded()
{
	OnSlicenrChanged();
}

void AutoTubeWidget::OnSlicenrChanged()
{
}

void AutoTubeWidget::Cleanup()
{
	m_SelectedObjects->setText("");
	m_CachedFeatureImage.img = nullptr;
}

void AutoTubeWidget::OnMouseClicked(iseg::Point p)
{
}

void AutoTubeWidget::SelectObjects()
{
	auto sel = m_Handler3D->TissueSelection();
	std::cout << "sel " << sel[0] << std::endl;
	std::string text = join(sel | transformed([](int d) { return std::to_string(d); }), ", ");
	m_SelectedObjects->setText(QString::fromStdString(text));
}

void AutoTubeWidget::DoWork()
{
	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	try
	{
		if ((true)) //(all_slices->isChecked())
		{
			using input_type = itk::SliceContiguousImage<float>;
			using tissues_type = itk::SliceContiguousImage<iseg::tissues_size_t>;
			auto source = itk_handler.GetSource(true); // active_slices -> correct seed z-position
			auto target = itk_handler.GetTarget(true);
			auto tissues = itk_handler.GetTissues(true);
			DoWorkNd<input_type, tissues_type, input_type>(source, tissues, target);
		}
		else
		{
			using input_type = itk::Image<float, 2>;
			using tissues_type = itk::Image<iseg::tissues_size_t, 2>;
			auto source = itk_handler.GetSourceSlice();
			auto target = itk_handler.GetTargetSlice();
			auto tissues = itk_handler.GetTissuesSlice();
			DoWorkNd<input_type, tissues_type, input_type>(source, tissues, target);
		}
	}
	catch (const std::exception& e)
	{
		QMessageBox::warning(this, "iSeg", QString("Error: ") + e.what(), QMessageBox::Ok | QMessageBox::Default);
	}
}

template<class TInput, class TImage>
typename TImage::Pointer AutoTubeWidget::ComputeFeatureImage(TInput* source) const
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<float, ImageDimension>;
	using hessian_image_type = itk::Image<hessian_pixel_type, ImageDimension>;
	using feature_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, TImage>;
	using multiscale_hessian_filter_type = itk::MultiScaleHessianBasedMeasureImageFilter<TInput, hessian_image_type, TImage>;

	double sigm_min = m_SigmaLow->text().toDouble();
	double sigm_max = m_SigmaHi->text().toDouble();
	int num_levels = m_NumberSigmaLevels->text().toInt();

	auto objectness_filter = feature_filter_type::New();
	objectness_filter->SetBrightObject(false);
	objectness_filter->SetObjectDimension(1);
	objectness_filter->SetScaleObjectnessMeasure(true);
	objectness_filter->SetAlpha(0.5);
	objectness_filter->SetBeta(0.5);
	objectness_filter->SetGamma(5.0);

	auto multi_scale_enhancement_filter = multiscale_hessian_filter_type::New();
	multi_scale_enhancement_filter->SetInput(source);
	multi_scale_enhancement_filter->SetHessianToMeasureFilter(objectness_filter);
	multi_scale_enhancement_filter->SetSigmaStepMethodToEquispaced();
	multi_scale_enhancement_filter->SetSigmaMinimum(std::min(sigm_min, sigm_max));
	multi_scale_enhancement_filter->SetSigmaMaximum(std::max(sigm_min, sigm_max));
	multi_scale_enhancement_filter->SetNumberOfSigmaSteps(std::min(1, num_levels));
	multi_scale_enhancement_filter->Update();
	return multi_scale_enhancement_filter->GetOutput();
}

template<class TInput, class TImage>
typename TImage::Pointer AutoTubeWidget::ComputeFeatureImage2d(TInput* source) const
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using input_type = TInput;
	using output_type = TImage;
	using image_type = itk::Image<float, 2>;

	using hessian_filter_type = itk::HessianRecursiveGaussianImageFilter<image_type>;
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<double, 2>;
	using hessian_image_type = itk::Image<hessian_pixel_type, 2>;

	using feature_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, image_type>;
	using multiscale_hessian_filter_type = itk::MultiScaleHessianBasedMeasureImageFilter<image_type, hessian_image_type, image_type>;
	using slice_by_slice_filter_type = itk::SliceBySliceImageFilter<input_type, output_type, multiscale_hessian_filter_type>;

	double sigm_min = m_SigmaLow->text().toDouble();
	double sigm_max = m_SigmaHi->text().toDouble();
	int num_levels = m_NumberSigmaLevels->text().toInt();

	auto objectness_filter = feature_filter_type::New();
	objectness_filter->SetBrightObject(false);
	objectness_filter->SetObjectDimension(ImageDimension == 2 ? 1 : 0); // for 2D analysis we are looking for lines in a single slice
	objectness_filter->SetScaleObjectnessMeasure(true);
	objectness_filter->SetAlpha(0.5);
	objectness_filter->SetBeta(0.5);
	objectness_filter->SetGamma(5.0);

	auto multi_scale_enhancement_filter = multiscale_hessian_filter_type::New();
	multi_scale_enhancement_filter->SetHessianToMeasureFilter(objectness_filter);
	multi_scale_enhancement_filter->SetSigmaStepMethodToEquispaced();
	multi_scale_enhancement_filter->SetSigmaMinimum(std::min(sigm_min, sigm_max));
	multi_scale_enhancement_filter->SetSigmaMaximum(std::max(sigm_min, sigm_max));
	multi_scale_enhancement_filter->SetNumberOfSigmaSteps(std::min(1, num_levels));

	auto slice_filter = slice_by_slice_filter_type::New();
	slice_filter->SetInput(source);
	slice_filter->SetFilter(multi_scale_enhancement_filter);
	slice_filter->Update();

	return slice_filter->GetOutput();
}

template<class TInput, class TTissue, class TTarget>
void AutoTubeWidget::DoWorkNd(TInput* source, TTissue* tissues, TTarget* target)
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using input_type = TInput;
	using real_type = itk::Image<float, ImageDimension>;
	using mask_type = itk::Image<unsigned char, ImageDimension>;
	using labelfield_type = itk::Image<unsigned short, ImageDimension>;

	using threshold_filter_type = itk::BinaryThresholdImageFilter<real_type, mask_type>;
	using nonmax_filter_type = itk::NonMaxSuppressionImageFilter<real_type>;
	using thinnning_filter_type = BinaryThinningImageFilter<mask_type, mask_type, ImageDimension>;

	typename real_type::Pointer feature_image;
	std::vector<double> feature_params;
	feature_params.push_back(m_SigmaLow->text().toDouble());
	feature_params.push_back(m_SigmaHi->text().toDouble());
	feature_params.push_back(m_NumberSigmaLevels->text().toInt());
	feature_params.push_back(m_Metric2d->isChecked());
	if (!m_CachedFeatureImage.Get(feature_image, feature_params))
	{
		feature_image = m_Metric2d->isChecked()
												? ComputeFeatureImage2d<input_type, real_type>(source)
												: ComputeFeatureImage<input_type, real_type>(source);
		m_CachedFeatureImage.Store(feature_image, feature_params);
		m_CachedSkeleton.img = nullptr;
	}

	// initialize threshold to reasonable value
	bool ok = false;
	float lower = m_Threshold->text().toFloat(&ok);
	if (!ok)
	{
		auto calculator = itk::MinimumMaximumImageCalculator<real_type>::New();
		calculator->SetImage(feature_image);
		calculator->Compute();

		auto min_gm = calculator->GetMinimum();
		auto max_gm = calculator->GetMaximum();

		lower = min_gm + 0.8 * (max_gm - min_gm); // stupid way to guess threshold
		m_Threshold->setText(QString::number(lower));
	}

	// extract IDs if any were set
	std::vector<int> object_ids;
	if (!m_SelectedObjects->text().isEmpty())
	{
		std::vector<std::string> tokens;
		std::string selected_objects_text = m_SelectedObjects->text().toStdString();
		boost::algorithm::split(tokens, selected_objects_text, boost::algorithm::is_any_of(","));
		std::transform(tokens.begin(), tokens.end(), std::back_inserter(object_ids), [](std::string s) {
			boost::algorithm::trim(s);
			return stoi(s);
		});
	}

	// mask feature image before skeletonization
	if (!object_ids.empty())
	{
		using map_functor_type = iseg::Functor::MapLabels<unsigned short, unsigned char>;
		map_functor_type map;
		map.m_Map.assign(m_Handler3D->TissueNames().size() + 1, 0);
		for (size_t i = 0; i < object_ids.size(); i++)
		{
			map.m_Map.at(object_ids[i]) = 1;
		}

		auto map_filter = itk::UnaryFunctorImageFilter<TTissue, mask_type, map_functor_type>::New();
		map_filter->SetFunctor(map);
		map_filter->SetInput(tissues);

		auto masker = itk::MaskImageFilter<real_type, mask_type>::New();
		masker->SetInput(feature_image);
		masker->SetMaskImage(map_filter->GetOutput());
		SAFE_UPDATE(masker, return );
		feature_image = masker->GetOutput();
	}

	typename mask_type::Pointer skeleton;
	std::vector<double> skeleton_params(object_ids.begin(), object_ids.end());
	skeleton_params.push_back(lower);
	skeleton_params.push_back(m_NonMaxSuppression->isChecked());
	skeleton_params.push_back(m_Skeletonize->isChecked());
	skeleton_params.push_back(m_MinObjectSize->text().toInt());
	if (!m_CachedSkeleton.Get(skeleton, skeleton_params))
	{
		// disconnect bright tubes via non-maxi suppression
		if (m_NonMaxSuppression->isChecked())
		{
			auto masking = itk::ThresholdImageFilter<real_type>::New();
			masking->SetInput(feature_image);
			masking->ThresholdBelow(lower);
			masking->SetOutsideValue(std::min(lower, 0.f));

			// do thinning to get brightest pixels ("one" pixel wide)
			auto nonmax_filter = nonmax_filter_type::New();
			nonmax_filter->SetInput(masking->GetOutput());

			auto threshold = threshold_filter_type::New();
			threshold->SetInput(nonmax_filter->GetOutput());
			threshold->SetLowerThreshold(std::nextafter(std::min(lower, 0.f), 1.f));
			SAFE_UPDATE(threshold, return );
			skeleton = threshold->GetOutput();
		}
		else
		{
			auto threshold = threshold_filter_type::New();
			threshold->SetInput(feature_image);
			threshold->SetLowerThreshold(lower);
			SAFE_UPDATE(threshold, return );
			skeleton = threshold->GetOutput();
		}

		if (m_Skeletonize->isChecked())
		{
			// get centerline: either thresholding or non-max suppression must be done before this
			auto thinning = thinnning_filter_type::New();
			thinning->SetInput(skeleton);

			auto rescale = itk::RescaleIntensityImageFilter<mask_type>::New();
			rescale->SetInput(thinning->GetOutput());
			rescale->SetOutputMinimum(0);
			rescale->SetOutputMaximum(255);
			rescale->InPlaceOn();

			SAFE_UPDATE(rescale, return );
			skeleton = rescale->GetOutput();
		}
		m_CachedSkeleton.Store(skeleton, skeleton_params);
	}

	typename mask_type::Pointer output;
	if (skeleton && m_MinObjectSize->text().toInt() > 1)
	{
		auto connectivity = itk::ConnectedComponentImageFilter<mask_type, labelfield_type>::New();
		connectivity->SetInput(skeleton);
		connectivity->FullyConnectedOn();

		auto relabel = itk::RelabelComponentImageFilter<labelfield_type, labelfield_type>::New();
		relabel->SetInput(connectivity->GetOutput());
		relabel->SetMinimumObjectSize(m_MinObjectSize->text().toInt());

		auto threshold = itk::BinaryThresholdImageFilter<labelfield_type, mask_type>::New();
		threshold->SetInput(relabel->GetOutput());
		threshold->SetLowerThreshold(1);
		SAFE_UPDATE(threshold, return );
		output = threshold->GetOutput();
	}

	iseg::DataSelection data_selection;
	data_selection.allSlices = true; // all_slices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (output && iseg::Paste<mask_type, input_type>(output, target))
	{
		// good, else maybe output is not defined
	}
	else if (!iseg::Paste<mask_type, input_type>(skeleton, target))
	{
		std::cerr << "Error: could not set output because image regions don't match.\n";
	}

	emit EndDatachange(this);
}
