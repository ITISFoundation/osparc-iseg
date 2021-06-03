/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "BiasCorrection.h"

#include "Data/ItkUtils.h"
#include "Data/SlicesHandlerITKInterface.h"

#include <itkBSplineControlPointImageFilter.h>
#include <itkCommand.h>
#include <itkDivideImageFilter.h>
#include <itkExpImageFilter.h>
#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <itkN4BiasFieldCorrectionImageFilter.h>
#include <itkShrinkImageFilter.h>
#include <itkTimeProbe.h>

#include <QFormLayout>
#include <qlabel.h>
#include <qprogressdialog.h>

#include <algorithm>
#include <functional>
#include <sstream>

namespace {
template<class TFilter>
class CommandIterationUpdate : public itk::Command
{
public:
	using Self = CommandIterationUpdate; // NOLINT
	using Superclass = itk::Command;		 // NOLINT
	using Pointer = itk::SmartPointer<Self>;
	itkNewMacro(Self);

protected:
	CommandIterationUpdate() = default;

public:
	void SetProgressObject(QProgressDialog* progress, const std::vector<unsigned int>& num_iterations)
	{
		m_NumberOfIterations = num_iterations;
		m_Progress = progress;
	}

	void Execute(itk::Object* caller, const itk::EventObject& event) override
	{
		Execute((const itk::Object*)caller, event);
	}

	void Execute(const itk::Object* object, const itk::EventObject& event) override
	{
		const TFilter* filter = dynamic_cast<const TFilter*>(object);

		if (typeid(event) != typeid(itk::IterationEvent))
		{
			return;
		}

		double current_level = filter->GetCurrentLevel();
		double current_iteration = filter->GetElapsedIterations();
		int percent = static_cast<int>((current_level +
																			 current_iteration / m_NumberOfIterations.at(current_level)) *
																	 100.0 / m_NumberOfIterations.size());
		m_Progress->setValue(std::min(percent, 100));

		if (m_Progress->wasCanceled())
		{
			// hack to stop filter from executing
			throw itk::ProcessAborted();
		}
	}

private:
	std::vector<unsigned int> m_NumberOfIterations;
	QProgressDialog* m_Progress;
};

template<typename ImageType>
typename ImageType::Pointer AllocImage(const typename itk::ImageBase<ImageType::ImageDimension>* exemplar)
{
	typename ImageType::Pointer rval = ImageType::New();
	// it may be the case that the output image might have a different
	// number of PixelComponents than the exemplar, so only copy this
	// information.
	// rval->CopyInformation(exemplar);
	rval->SetLargestPossibleRegion(exemplar->GetLargestPossibleRegion());
	rval->SetBufferedRegion(exemplar->GetBufferedRegion());
	rval->SetRequestedRegion(exemplar->GetRequestedRegion());
	rval->SetSpacing(exemplar->GetSpacing());
	rval->SetOrigin(exemplar->GetOrigin());
	rval->SetDirection(exemplar->GetDirection());
	rval->Allocate();
	return rval;
}

} // namespace

BiasCorrectionWidget::BiasCorrectionWidget(iseg::SlicesHandlerInterface* hand3D)
		: m_Handler3D(hand3D), m_CurrentFilter(nullptr)
{
	setToolTip(Format("Correct non-uniformity (especially in MRI) using the N4 Bias Correction "
										"algorithm by "
										"<br>"
										"Tustison et al., 'N4ITK: Improved N3 Bias Correction', IEEE "
										"Transactions on Medical Imaging, 29(6) : 1310 - 1320, June 2010"));

	m_Activeslice = m_Handler3D->ActiveSlice();

	auto bias_header = new QLabel("N4 Bias Correction");

	m_NumberLevels = new QSpinBox(0, 50, 1, nullptr);
	m_NumberLevels->setValue(4);

	m_ShrinkFactor = new QSpinBox(1, 16, 1, nullptr);
	m_ShrinkFactor->setValue(4);

	m_NumberIterations = new QSpinBox(1, 200, 5, nullptr);
	m_NumberIterations->setValue(50);

	m_Execute = new QPushButton("Execute");

	// layout
	auto layout = new QFormLayout;
	layout->addRow(bias_header);
	layout->addRow("Fitting Levels", m_NumberLevels);
	layout->addRow("Shrink Factor", m_ShrinkFactor);
	layout->addRow("Iterations", m_NumberIterations);
	layout->addRow(m_Execute);

	setLayout(layout);

	// connections
	QObject_connect(m_Execute, SIGNAL(clicked()), this, SLOT(DoWork()));
}

void BiasCorrectionWidget::DoWork()
{
	using input_image_type = itk::Image<float, 3>;

	iseg::SlicesHandlerITKInterface wrapper(m_Handler3D);
	input_image_type::Pointer input = wrapper.GetImageDeprecated(iseg::SlicesHandlerITKInterface::kSource, true);

	//Ensure that it is a 3D image for the 3D image filter ! Else it does nothing
	if (input->GetLargestPossibleRegion().GetSize(2) > 1)
	{
		int num_levels = m_NumberLevels->value();
		int num_iterations = m_NumberIterations->value();
		int factor = m_ShrinkFactor->value();
		double conv_threshold = 0.0;

		try
		{
			auto output = DoBiasCorrection<input_image_type::Pointer>(input, nullptr, std::vector<unsigned int>(num_levels, num_iterations), factor, conv_threshold);

			if (output)
			{
				auto source = wrapper.GetSource(true);

				iseg::DataSelection data_selection;
				data_selection.allSlices = true;
				data_selection.bmp = true;
				emit BeginDatachange(data_selection, this);

				iseg::Paste<input_image_type, iseg::SlicesHandlerITKInterface::image_ref_type>(output, source);

				emit EndDatachange(this);
			}
		}
		catch (itk::ExceptionObject&)
		{
		}
		catch (...)
		{
			// itk calls 'throw' with no exception
		}
	}
}

void BiasCorrectionWidget::Cancel()
{
	if (m_CurrentFilter)
	{
		m_CurrentFilter->SetAbortGenerateData(true);
		m_CurrentFilter = nullptr;
	}
}

BiasCorrectionWidget::~BiasCorrectionWidget() = default;

void BiasCorrectionWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
}

void BiasCorrectionWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void BiasCorrectionWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
}

std::string BiasCorrectionWidget::GetName()
{
	return std::string("MRI Bias Correction");
}

QIcon BiasCorrectionWidget::GetIcon(QDir picdir)
{
	return QIcon(picdir.absFilePath(QString("Bias.png")).ascii());
}

template<typename ImagePointer>
ImagePointer BiasCorrectionWidget::DoBiasCorrection(ImagePointer inputImage, ImagePointer maskImage, const std::vector<unsigned int>& numIters, int shrinkFactor, double convergenceThreshold)
{
	bool verbose = false;

	using image_type = typename ImagePointer::ObjectType;
	using mask_image_type = typename ImagePointer::ObjectType;

	using corrector_type = itk::N4BiasFieldCorrectionImageFilter<image_type, mask_image_type, image_type>;
	auto corrector = corrector_type::New();
	m_CurrentFilter = corrector;

	QProgressDialog progress("Performing bias correction...", "Cancel", 0, 101, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.show();
	progress.setValue(1);

	QObject_connect(&progress, SIGNAL(clicked()), this, SLOT(Cancel()));

	/**
	* handle the mask image
	*/
	bool is_mask_specified = (maskImage.IsNotNull());
	if (!maskImage)
	{
		if (verbose)
		{
			std::cout << "Mask not read.  Using the entire image as the mask."
								<< std::endl
								<< std::endl;
		}
		maskImage = mask_image_type::New();
		maskImage->CopyInformation(inputImage);
		maskImage->SetRegions(inputImage->GetRequestedRegion());
		maskImage->Allocate(false);
		maskImage->FillBuffer(itk::NumericTraits<typename mask_image_type::PixelType>::OneValue());
	}

	/**
	* convergence options
	*/
	typename corrector_type::VariableSizeArrayType max_number_iterations(numIters.size());
	for (unsigned int d = 0; d < numIters.size(); d++)
	{
		max_number_iterations[d] = numIters[d];
	}
	corrector->SetMaximumNumberOfIterations(max_number_iterations);

	typename corrector_type::ArrayType number_fitting_levels;
	number_fitting_levels.Fill(numIters.size());
	corrector->SetNumberOfFittingLevels(number_fitting_levels);
	corrector->SetConvergenceThreshold(0.0);

	using shrinker_type = itk::ShrinkImageFilter<image_type, image_type>;
	auto shrinker = shrinker_type::New();
	shrinker->SetInput(inputImage);
	shrinker->SetShrinkFactors(shrinkFactor);

	using mask_shrinker_type = itk::ShrinkImageFilter<mask_image_type, mask_image_type>;
	auto mask_shrinker = mask_shrinker_type::New();
	mask_shrinker->SetInput(maskImage);
	mask_shrinker->SetShrinkFactors(shrinkFactor);

	shrinker->Update();
	mask_shrinker->Update();

	itk::TimeProbe timer;
	timer.Start();

	corrector->SetInput(shrinker->GetOutput());
	corrector->SetMaskImage(mask_shrinker->GetOutput());

	using command_type = CommandIterationUpdate<corrector_type>;
	auto observer = command_type::New();
	corrector->AddObserver(itk::IterationEvent(), observer);
	observer->SetProgressObject(&progress, numIters);

	/**
	* histogram sharpening options
	*/
	//corrector->SetBiasFieldFullWidthAtHalfMaximum(0.15);
	//corrector->SetWienerFilterNoise(0.01);
	//corrector->SetNumberOfHistogramBins(200);

	try
	{
		// corrector->DebugOn();
		corrector->Update();
	}
	catch (itk::ExceptionObject& e)
	{
		if (verbose)
		{
			std::cerr << "Exception caught: " << e << std::endl;
		}
		m_CurrentFilter = nullptr;
		return nullptr;
	}

	m_CurrentFilter = nullptr;

	if (verbose)
	{
		corrector->Print(std::cout, 3);
	}

	timer.Stop();
	if (verbose)
	{
		std::cout << "Elapsed time: " << timer.GetMean() << std::endl;
	}

	/**
	* output
	*
	* Reconstruct the bias field at full image resolution.  Divide
	* the original input image by the bias field to get the final
	* corrected image.
	*/
	using b_spliner_type = itk::BSplineControlPointImageFilter<
			typename corrector_type::BiasFieldControlPointLatticeType, typename corrector_type::ScalarImageType>;
	auto bspliner = b_spliner_type::New();
	bspliner->SetInput(corrector->GetLogBiasFieldControlPointLattice());
	bspliner->SetSplineOrder(corrector->GetSplineOrder());
	bspliner->SetSize(inputImage->GetLargestPossibleRegion().GetSize());
	bspliner->SetOrigin(inputImage->GetOrigin());
	bspliner->SetDirection(inputImage->GetDirection());
	bspliner->SetSpacing(inputImage->GetSpacing());
	bspliner->Update();

	auto log_field = AllocImage<image_type>(inputImage);

	itk::ImageRegionIterator<typename corrector_type::ScalarImageType> it_b(bspliner->GetOutput(), bspliner->GetOutput()->GetLargestPossibleRegion());
	itk::ImageRegionIterator<image_type> it_f(log_field, log_field->GetLargestPossibleRegion());
	for (it_b.GoToBegin(), it_f.GoToBegin(); !it_b.IsAtEnd(); ++it_b, ++it_f)
	{
		it_f.Set(it_b.Get()[0]);
	}

	using exp_filter_type = itk::ExpImageFilter<image_type, image_type>;
	auto exp_filter = exp_filter_type::New();
	exp_filter->SetInput(log_field);
	exp_filter->Update();

	using divider_type = itk::DivideImageFilter<image_type, image_type, image_type>;
	auto divider = divider_type::New();
	divider->SetInput1(inputImage);
	divider->SetInput2(exp_filter->GetOutput());
	divider->Update();

	if (is_mask_specified)
	{
		itk::ImageRegionIteratorWithIndex<image_type> it_d(divider->GetOutput(), divider->GetOutput()->GetLargestPossibleRegion());
		itk::ImageRegionIterator<image_type> it_i(inputImage, inputImage->GetLargestPossibleRegion());
		for (it_d.GoToBegin(), it_i.GoToBegin(); !it_d.IsAtEnd(); ++it_d, ++it_i)
		{
			if (maskImage->GetPixel(it_d.GetIndex()) ==
					itk::NumericTraits<typename mask_image_type::PixelType>::ZeroValue())
			{
				it_d.Set(it_i.Get());
			}
		}
	}

	return divider->GetOutput();
}
