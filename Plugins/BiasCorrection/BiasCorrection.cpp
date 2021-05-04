/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "BiasCorrection.h"

#include "Data/SlicesHandlerITKInterface.h"
#include "Data/ItkUtils.h"

#include <itkBSplineControlPointImageFilter.h>
#include <itkCommand.h>
#include <itkDivideImageFilter.h>
#include <itkExpImageFilter.h>
#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <itkN4BiasFieldCorrectionImageFilter.h>
#include <itkShrinkImageFilter.h>
#include <itkTimeProbe.h>

#include <qlabel.h>
#include <qprogressdialog.h>
#include <QFormLayout>

#include <algorithm>
#include <functional>
#include <sstream>

namespace {
template<class TFilter>
class CommandIterationUpdate : public itk::Command
{
public:
	typedef CommandIterationUpdate Self;
	typedef itk::Command Superclass;
	typedef itk::SmartPointer<Self> Pointer;
	itkNewMacro(Self);

protected:
	CommandIterationUpdate(){};

public:
	void SetProgressObject(QProgressDialog* progress,
			const std::vector<unsigned int>& num_iterations)
	{
		m_NumberOfIterations = num_iterations;
		m_Progress = progress;
	}

	void Execute(itk::Object* caller, const itk::EventObject& event) override
	{
		Execute((const itk::Object*)caller, event);
	}

	void Execute(const itk::Object* object,
			const itk::EventObject& event) override
	{
		const TFilter* filter = dynamic_cast<const TFilter*>(object);

		if (typeid(event) != typeid(itk::IterationEvent))
		{
			return;
		}

		double current_level = filter->GetCurrentLevel();
		double current_iteration = filter->GetElapsedIterations();
		int percent = static_cast<int>(
				(current_level +
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
typename ImageType::Pointer AllocImage(
		const typename itk::ImageBase<ImageType::ImageDimension>* exemplar)
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

BiasCorrectionWidget::BiasCorrectionWidget(iseg::SlicesHandlerInterface* hand3D,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D),
			m_CurrentFilter(nullptr)
{
	setToolTip(Format(
			"Correct non-uniformity (especially in MRI) using the N4 Bias Correction "
			"algorithm by "
			"<br>"
			"Tustison et al., 'N4ITK: Improved N3 Bias Correction', IEEE "
			"Transactions on Medical Imaging, 29(6) : 1310 - 1320, June 2010"));

	activeslice = handler3D->active_slice();

	auto bias_header = new QLabel("N4 Bias Correction");

	number_levels = new QSpinBox(0, 50, 1, nullptr);
	number_levels->setValue(4);

	shrink_factor = new QSpinBox(1, 16, 1, nullptr);
	shrink_factor->setValue(4);

	number_iterations = new QSpinBox(1, 200, 5, nullptr);
	number_iterations->setValue(50);

	execute = new QPushButton("Execute");

	// layout
	auto layout = new QFormLayout;
	layout->addRow(bias_header);
	layout->addRow("Fitting Levels", number_levels);
	layout->addRow("Shrink Factor", shrink_factor);
	layout->addRow("Iterations", number_iterations);
	layout->addRow(execute);

	setLayout(layout);

	// connections
	connect(execute, SIGNAL(clicked()), this, SLOT(do_work()));
}

void BiasCorrectionWidget::do_work()
{
	typedef itk::Image<float, 3> InputImageType;

	iseg::SlicesHandlerITKInterface wrapper(handler3D);
	InputImageType::Pointer input = wrapper.GetImageDeprecated(iseg::SlicesHandlerITKInterface::kSource, true);

	//Ensure that it is a 3D image for the 3D image filter ! Else it does nothing
	if (input->GetLargestPossibleRegion().GetSize(2) > 1)
	{
		int num_levels = number_levels->value();
		int num_iterations = number_iterations->value();
		int factor = shrink_factor->value();
		double conv_threshold = 0.0;

		try
		{
			auto output = DoBiasCorrection<InputImageType::Pointer>(
					input, nullptr,
					std::vector<unsigned int>(num_levels, num_iterations), factor,
					conv_threshold);

			if (output)
			{
				auto source = wrapper.GetSource(true);

				iseg::DataSelection dataSelection;
				dataSelection.allSlices = true;
				dataSelection.bmp = true;
				emit begin_datachange(dataSelection, this);

				iseg::Paste<InputImageType, iseg::SlicesHandlerITKInterface::image_ref_type>(output, source);

				emit end_datachange(this);
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

void BiasCorrectionWidget::cancel()
{
	if (m_CurrentFilter)
	{
		m_CurrentFilter->SetAbortGenerateData(true);
		m_CurrentFilter = nullptr;
	}
}

BiasCorrectionWidget::~BiasCorrectionWidget() 
{

}

void BiasCorrectionWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
}

void BiasCorrectionWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void BiasCorrectionWidget::newloaded()
{
	activeslice = handler3D->active_slice();
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
ImagePointer BiasCorrectionWidget::DoBiasCorrection(
		ImagePointer inputImage, ImagePointer maskImage,
		const std::vector<unsigned int>& numIters, int shrinkFactor,
		double convergenceThreshold)
{
	bool verbose = false;

	using ImageType = typename ImagePointer::ObjectType;
	using MaskImageType = typename ImagePointer::ObjectType;

	using CorrectorType = itk::N4BiasFieldCorrectionImageFilter<ImageType, MaskImageType, ImageType>;
	auto corrector = CorrectorType::New();
	m_CurrentFilter = corrector;

	QProgressDialog progress("Performing bias correction...", "Cancel", 0, 101, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.show();
	progress.setValue(1);

	QObject::connect(&progress, SIGNAL(clicked()), this, SLOT(cancel()));

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
		maskImage = MaskImageType::New();
		maskImage->CopyInformation(inputImage);
		maskImage->SetRegions(inputImage->GetRequestedRegion());
		maskImage->Allocate(false);
		maskImage->FillBuffer(itk::NumericTraits<typename MaskImageType::PixelType>::OneValue());
	}

	/**
	* convergence options
	*/
	typename CorrectorType::VariableSizeArrayType max_number_iterations(numIters.size());
	for (unsigned int d = 0; d < numIters.size(); d++)
	{
		max_number_iterations[d] = numIters[d];
	}
	corrector->SetMaximumNumberOfIterations(max_number_iterations);

	typename CorrectorType::ArrayType number_fitting_levels;
	number_fitting_levels.Fill(numIters.size());
	corrector->SetNumberOfFittingLevels(number_fitting_levels);
	corrector->SetConvergenceThreshold(0.0);

	using ShrinkerType = itk::ShrinkImageFilter<ImageType, ImageType>;
	auto shrinker = ShrinkerType::New();
	shrinker->SetInput(inputImage);
	shrinker->SetShrinkFactors(shrinkFactor);

	using MaskShrinkerType = itk::ShrinkImageFilter<MaskImageType, MaskImageType>;
	auto mask_shrinker = MaskShrinkerType::New();
	mask_shrinker->SetInput(maskImage);
	mask_shrinker->SetShrinkFactors(shrinkFactor);

	shrinker->Update();
	mask_shrinker->Update();

	itk::TimeProbe timer;
	timer.Start();

	corrector->SetInput(shrinker->GetOutput());
	corrector->SetMaskImage(mask_shrinker->GetOutput());

	using CommandType = CommandIterationUpdate<CorrectorType>;
	auto observer = CommandType::New();
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
	using BSplinerType = itk::BSplineControlPointImageFilter<
			typename CorrectorType::BiasFieldControlPointLatticeType,
			typename CorrectorType::ScalarImageType>;
	typename BSplinerType::Pointer bspliner = BSplinerType::New();
	bspliner->SetInput(corrector->GetLogBiasFieldControlPointLattice());
	bspliner->SetSplineOrder(corrector->GetSplineOrder());
	bspliner->SetSize(inputImage->GetLargestPossibleRegion().GetSize());
	bspliner->SetOrigin(inputImage->GetOrigin());
	bspliner->SetDirection(inputImage->GetDirection());
	bspliner->SetSpacing(inputImage->GetSpacing());
	bspliner->Update();

	auto log_field = AllocImage<ImageType>(inputImage);

	itk::ImageRegionIterator<typename CorrectorType::ScalarImageType> ItB(bspliner->GetOutput(), bspliner->GetOutput()->GetLargestPossibleRegion());
	itk::ImageRegionIterator<ImageType> ItF(log_field,log_field->GetLargestPossibleRegion());
	for (ItB.GoToBegin(), ItF.GoToBegin(); !ItB.IsAtEnd(); ++ItB, ++ItF)
	{
		ItF.Set(ItB.Get()[0]);
	}

	using ExpFilterType = itk::ExpImageFilter<ImageType, ImageType>;
	auto exp_filter = ExpFilterType::New();
	exp_filter->SetInput(log_field);
	exp_filter->Update();

	using DividerType = itk::DivideImageFilter<ImageType, ImageType, ImageType>;
	auto divider = DividerType::New();
	divider->SetInput1(inputImage);
	divider->SetInput2(exp_filter->GetOutput());
	divider->Update();

	if (is_mask_specified)
	{
		itk::ImageRegionIteratorWithIndex<ImageType> ItD(divider->GetOutput(), divider->GetOutput()->GetLargestPossibleRegion());
		itk::ImageRegionIterator<ImageType> ItI(inputImage, inputImage->GetLargestPossibleRegion());
		for (ItD.GoToBegin(), ItI.GoToBegin(); !ItD.IsAtEnd(); ++ItD, ++ItI)
		{
			if (maskImage->GetPixel(ItD.GetIndex()) ==
					itk::NumericTraits<typename MaskImageType::PixelType>::ZeroValue())
			{
				ItD.Set(ItI.Get());
			}
		}
	}

	return divider->GetOutput();
}
