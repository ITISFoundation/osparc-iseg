/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
#include "Data/SliceHandlerItkWrapper.h"

#include "itkBinaryThinningImageFilter3D.h"
#include "itkNonMaxSuppressionImageFilter.h"

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

AutoTubeWidget::AutoTubeWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), _handler3D(hand3D)
{
	setToolTip(Format("Sandbox Tool"));

	_metric2d = new QCheckBox;
	_metric2d->setChecked(false);
	_metric2d->setToolTip(Format("Use 2D pseudo vesselness (blob feature per slice), or use full 3D vesselness."));

	_non_max_suppression = new QCheckBox;
	_non_max_suppression->setChecked(true);
	_non_max_suppression->setToolTip(Format("Extract approx. one pixel wide paths based on non-maximum suppression."));

	_skeletonize = new QCheckBox;
	_skeletonize->setChecked(true);
	_skeletonize->setToolTip(Format("Compute 1-pixel wide centerlines (skeleton)."));

	_sigma_low = new QLineEdit(QString::number(0.3));
	_sigma_low->setValidator(new QDoubleValidator);

	_sigma_hi = new QLineEdit(QString::number(0.6));
	_sigma_hi->setValidator(new QDoubleValidator);

	_number_sigma_levels = new QLineEdit(QString::number(2));
	_number_sigma_levels->setValidator(new QIntValidator);

	_threshold = new QLineEdit;
	_threshold->setValidator(new QDoubleValidator);

	_max_radius = new QLineEdit(QString::number(1));
	_max_radius->setValidator(new QDoubleValidator);

	_min_object_size = new QLineEdit(QString::number(10));
	_min_object_size->setValidator(new QIntValidator);

	_select_objects_button = new QPushButton("Select Mask");
	_selected_objects = new QLineEdit;
	_selected_objects->setReadOnly(true);

	_execute_button = new QPushButton("Execute");

	auto layout = new QFormLayout;
	layout->addRow("Sigma Min", _sigma_low);
	layout->addRow("Sigma Max", _sigma_hi);
	layout->addRow("Number of Sigmas", _number_sigma_levels);
	layout->addRow("2D Vesselness", _metric2d);
	layout->addRow("Feature Threshold", _threshold);
	layout->addRow("Non-maximum Suppression", _non_max_suppression);
	layout->addRow("Centerlines", _skeletonize);
	//layout->addRow("Maximum radius", _max_radius);
	layout->addRow("Minimum object size", _min_object_size);
	layout->addRow(_select_objects_button, _selected_objects);
	layout->addRow(_execute_button);

	auto big_view = new QWidget;
	big_view->setLayout(layout);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidget(big_view);

	auto top_layout = new QGridLayout(1, 1);
	top_layout->addWidget(scroll_area, 0, 0);
	setLayout(top_layout);

	QObject::connect(_select_objects_button, SIGNAL(clicked()), this, SLOT(select_objects()));
	QObject::connect(_execute_button, SIGNAL(clicked()), this, SLOT(do_work()));
}

void AutoTubeWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void AutoTubeWidget::newloaded()
{
	on_slicenr_changed();
}

void AutoTubeWidget::on_slicenr_changed()
{
}

void AutoTubeWidget::cleanup()
{
	_selected_objects->setText("");
	_cached_feature_image.img = nullptr;
}

void AutoTubeWidget::on_mouse_clicked(iseg::Point p)
{
}

void AutoTubeWidget::select_objects()
{
	auto sel = _handler3D->tissue_selection();
	std::cout << "sel " << sel[0] << std::endl;
	std::string text = join(sel | transformed([](int d) { return std::to_string(d); }), ", ");
	_selected_objects->setText(QString::fromStdString(text));
}

void AutoTubeWidget::do_work()
{
	iseg::SliceHandlerItkWrapper itk_handler(_handler3D);
	try
	{
		if (true) //(all_slices->isChecked())
		{
			using input_type = itk::SliceContiguousImage<float>;
			using tissues_type = itk::SliceContiguousImage<iseg::tissues_size_t>;
			auto source = itk_handler.GetSource(true); // active_slices -> correct seed z-position
			auto target = itk_handler.GetTarget(true);
			auto tissues = itk_handler.GetTissues(true);
			do_work_nd<input_type, tissues_type, input_type>(source, tissues, target);
		}
		else
		{
			using input_type = itk::Image<float, 2>;
			using tissues_type = itk::Image<iseg::tissues_size_t, 2>;
			auto source = itk_handler.GetSourceSlice();
			auto target = itk_handler.GetTargetSlice();
			auto tissues = itk_handler.GetTissuesSlice();
			do_work_nd<input_type, tissues_type, input_type>(source, tissues, target);
		}
	}
	catch (const std::exception& e)
	{
		QMessageBox::warning(this, "iSeg", QString("Error: ") + e.what(), QMessageBox::Ok | QMessageBox::Default);
	}
}

template<class TInput, class TImage>
typename TImage::Pointer AutoTubeWidget::compute_feature_image(TInput* source) const
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<float, ImageDimension>;
	using hessian_image_type = itk::Image<hessian_pixel_type, ImageDimension>;
	using feature_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, TImage>;
	using multiscale_hessian_filter_type = itk::MultiScaleHessianBasedMeasureImageFilter<TInput, hessian_image_type, TImage>;

	double sigm_min = _sigma_low->text().toDouble();
	double sigm_max = _sigma_hi->text().toDouble();
	int num_levels = _number_sigma_levels->text().toInt();

	auto objectnessFilter = feature_filter_type::New();
	objectnessFilter->SetBrightObject(false);
	objectnessFilter->SetObjectDimension(1);
	objectnessFilter->SetScaleObjectnessMeasure(true);
	objectnessFilter->SetAlpha(0.5);
	objectnessFilter->SetBeta(0.5);
	objectnessFilter->SetGamma(5.0);

	auto multiScaleEnhancementFilter = multiscale_hessian_filter_type::New();
	multiScaleEnhancementFilter->SetInput(source);
	multiScaleEnhancementFilter->SetHessianToMeasureFilter(objectnessFilter);
	multiScaleEnhancementFilter->SetSigmaStepMethodToEquispaced();
	multiScaleEnhancementFilter->SetSigmaMinimum(std::min(sigm_min, sigm_max));
	multiScaleEnhancementFilter->SetSigmaMaximum(std::max(sigm_min, sigm_max));
	multiScaleEnhancementFilter->SetNumberOfSigmaSteps(std::min(1, num_levels));
	multiScaleEnhancementFilter->Update();
	return multiScaleEnhancementFilter->GetOutput();
}

template<class TInput, class TImage>
typename TImage::Pointer AutoTubeWidget::compute_feature_image_2d(TInput* source) const
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

	double sigm_min = _sigma_low->text().toDouble();
	double sigm_max = _sigma_hi->text().toDouble();
	int num_levels = _number_sigma_levels->text().toInt();

	auto objectnessFilter = feature_filter_type::New();
	objectnessFilter->SetBrightObject(false);
	objectnessFilter->SetObjectDimension(ImageDimension == 2 ? 1 : 0); // for 2D analysis we are looking for lines in a single slice
	objectnessFilter->SetScaleObjectnessMeasure(true);
	objectnessFilter->SetAlpha(0.5);
	objectnessFilter->SetBeta(0.5);
	objectnessFilter->SetGamma(5.0);

	auto multiScaleEnhancementFilter = multiscale_hessian_filter_type::New();
	multiScaleEnhancementFilter->SetHessianToMeasureFilter(objectnessFilter);
	multiScaleEnhancementFilter->SetSigmaStepMethodToEquispaced();
	multiScaleEnhancementFilter->SetSigmaMinimum(std::min(sigm_min, sigm_max));
	multiScaleEnhancementFilter->SetSigmaMaximum(std::max(sigm_min, sigm_max));
	multiScaleEnhancementFilter->SetNumberOfSigmaSteps(std::min(1, num_levels));

	auto slice_filter = slice_by_slice_filter_type::New();
	slice_filter->SetInput(source);
	slice_filter->SetFilter(multiScaleEnhancementFilter);
	slice_filter->Update();

	return slice_filter->GetOutput();
}

template<class TInput, class TTissue, class TTarget>
void AutoTubeWidget::do_work_nd(TInput* source, TTissue* tissues, TTarget* target)
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
	feature_params.push_back(_sigma_low->text().toDouble());
	feature_params.push_back(_sigma_hi->text().toDouble());
	feature_params.push_back(_number_sigma_levels->text().toInt());
	feature_params.push_back(_metric2d->isChecked());
	if (!_cached_feature_image.get(feature_image, feature_params))
	{
		feature_image = _metric2d->isChecked()
												? compute_feature_image_2d<input_type, real_type>(source)
												: compute_feature_image<input_type, real_type>(source);
		_cached_feature_image.store(feature_image, feature_params);
		_cached_skeleton.img = nullptr;
	}

	// initialize threshold to reasonable value
	bool ok = false;
	float lower = _threshold->text().toFloat(&ok);
	if (!ok)
	{
		auto calculator = itk::MinimumMaximumImageCalculator<real_type>::New();
		calculator->SetImage(feature_image);
		calculator->Compute();

		auto min_gm = calculator->GetMinimum();
		auto max_gm = calculator->GetMaximum();

		lower = min_gm + 0.8 * (max_gm - min_gm); // stupid way to guess threshold
		_threshold->setText(QString::number(lower));
	}

	// extract IDs if any were set
	std::vector<int> object_ids;
	if (!_selected_objects->text().isEmpty())
	{
		std::vector<std::string> tokens;
		std::string selected_objects_text = _selected_objects->text().toStdString();
		boost::algorithm::split(tokens, selected_objects_text, boost::algorithm::is_any_of(","));
		std::transform(tokens.begin(), tokens.end(), std::back_inserter(object_ids),
				[](std::string s) {
					boost::algorithm::trim(s);
					return stoi(s);
				});
	}

	// mask feature image before skeletonization
	if (!object_ids.empty())
	{
		using map_functor = iseg::Functor::MapLabels<unsigned short, unsigned char>;
		map_functor map;
		map.m_Map.assign(_handler3D->tissue_names().size() + 1, 0);
		for (size_t i = 0; i < object_ids.size(); i++)
		{
			map.m_Map.at(object_ids[i]) = 1;
		}

		auto map_filter = itk::UnaryFunctorImageFilter<TTissue, mask_type, map_functor>::New();
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
	skeleton_params.push_back(_non_max_suppression->isChecked());
	skeleton_params.push_back(_skeletonize->isChecked());
	skeleton_params.push_back(_min_object_size->text().toInt());
	if (!_cached_skeleton.get(skeleton, skeleton_params))
	{
		// disconnect bright tubes via non-maxi suppression
		if (_non_max_suppression->isChecked())
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

		if (_skeletonize->isChecked())
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
		_cached_skeleton.store(skeleton, skeleton_params);
	}

	typename mask_type::Pointer output;
	if (skeleton && _min_object_size->text().toInt() > 1)
	{
		auto connectivity = itk::ConnectedComponentImageFilter<mask_type, labelfield_type>::New();
		connectivity->SetInput(skeleton);
		connectivity->FullyConnectedOn();

		auto relabel = itk::RelabelComponentImageFilter<labelfield_type, labelfield_type>::New();
		relabel->SetInput(connectivity->GetOutput());
		relabel->SetMinimumObjectSize(_min_object_size->text().toInt());

		auto threshold = itk::BinaryThresholdImageFilter<labelfield_type, mask_type>::New();
		threshold->SetInput(relabel->GetOutput());
		threshold->SetLowerThreshold(1);
		SAFE_UPDATE(threshold, return );
		output = threshold->GetOutput();
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true; // all_slices->isChecked();
	dataSelection.sliceNr = _handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (output && iseg::Paste<mask_type, input_type>(output, target))
	{
		// good, else maybe output is not defined
	}
	else if (!iseg::Paste<mask_type, input_type>(skeleton, target))
	{
		std::cerr << "Error: could not set output because image regions don't match.\n";
	}

	emit end_datachange(this);
}
