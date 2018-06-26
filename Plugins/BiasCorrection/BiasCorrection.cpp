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

#include "Data/SliceHandlerItkWrapper.h"
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

#include <qprogressdialog.h>

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

	void Execute(itk::Object* caller, const itk::EventObject& event) ITK_OVERRIDE
	{
		Execute((const itk::Object*)caller, event);
	}

	void Execute(const itk::Object* object,
			const itk::EventObject& event) ITK_OVERRIDE
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

BiasCorrectionWidget::BiasCorrectionWidget(iseg::SliceHandlerInterface* hand3D,
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

	vbox1 = new Q3VBox(this);
	bias_header = new QLabel("N4 Bias Correction: ", vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);

	txt_h2 = new QLabel("Fitting Levels: ", hbox2);
	edit_num_levels = new QSpinBox(0, 50, 1, hbox2);
	edit_num_levels->setValue(4);

	txt_h3 = new QLabel("Shrink Factor", hbox3);
	edit_shrink_factor = new QSpinBox(1, 16, 1, hbox3);
	edit_shrink_factor->setValue(4);

	txt_h4 = new QLabel("Iterations: ", hbox4);
	edit_num_iterations = new QSpinBox(1, 200, 5, hbox4);
	edit_num_iterations->setValue(50);

	bias_exec = new QPushButton("Execute", vbox1);

	vbox1->setMinimumWidth(std::max(300, vbox1->sizeHint().width()));

	QObject::connect(bias_exec, SIGNAL(clicked()), this, SLOT(do_work()));
}

void BiasCorrectionWidget::do_work()
{
	typedef itk::Image<float, 3> InputImageType;

	iseg::SliceHandlerItkWrapper wrapper(handler3D);
	InputImageType::Pointer input = wrapper.GetImageDeprecated(iseg::SliceHandlerItkWrapper::kSource, true);

	//Ensure that it is a 3D image for the 3D image filter ! Else it does nothing
	if (input->GetLargestPossibleRegion().GetSize(2) > 1)
	{
		int num_levels = edit_num_levels->value();
		int num_iterations = edit_num_iterations->value();
		int shrink_factor = edit_shrink_factor->value();
		double conv_threshold = 0.0;

		try
		{
			auto output = DoBiasCorrection<InputImageType::Pointer>(
					input, ITK_NULLPTR,
					std::vector<unsigned int>(num_levels, num_iterations), shrink_factor,
					conv_threshold);

			if (output)
			{
				auto source = wrapper.GetSource(true);

				iseg::DataSelection dataSelection;
				dataSelection.allSlices = true;
				dataSelection.bmp = true;
				emit begin_datachange(dataSelection, this);

				iseg::Paste<InputImageType, iseg::SliceHandlerItkWrapper::image_ref_type>(output, source);

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

QSize BiasCorrectionWidget::sizeHint() const { return vbox1->sizeHint(); }

BiasCorrectionWidget::~BiasCorrectionWidget() { delete vbox1; }

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

	typedef typename ImagePointer::ObjectType ImageType;
	typedef typename ImagePointer::ObjectType MaskImageType;
	static const int ImageDimension = ImageType::ImageDimension;

	typedef itk::N4BiasFieldCorrectionImageFilter<ImageType, MaskImageType,
			ImageType>
			CorrecterType;
	typename CorrecterType::Pointer correcter = CorrecterType::New();
	m_CurrentFilter = correcter;

	QProgressDialog progress("Performing bias correction...", "Cancel", 0, 101,
			this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.show();
	progress.setValue(1);

	QObject::connect(&progress, SIGNAL(clicked()), this, SLOT(cancel()));

	/**
	* handle the mask image
	*/
	bool isMaskImageSpecified = (maskImage.IsNotNull());
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
		maskImage->FillBuffer(
				itk::NumericTraits<typename MaskImageType::PixelType>::OneValue());
	}

	/**
	* convergence options
	*/
	typename CorrecterType::VariableSizeArrayType maximumNumberOfIterations(
			numIters.size());
	for (unsigned int d = 0; d < numIters.size(); d++)
	{
		maximumNumberOfIterations[d] = numIters[d];
	}
	correcter->SetMaximumNumberOfIterations(maximumNumberOfIterations);

	typename CorrecterType::ArrayType numberOfFittingLevels;
	numberOfFittingLevels.Fill(numIters.size());
	correcter->SetNumberOfFittingLevels(numberOfFittingLevels);
	correcter->SetConvergenceThreshold(0.0);

	typedef itk::ShrinkImageFilter<ImageType, ImageType> ShrinkerType;
	typename ShrinkerType::Pointer shrinker = ShrinkerType::New();
	shrinker->SetInput(inputImage);
	shrinker->SetShrinkFactors(shrinkFactor);

	typedef itk::ShrinkImageFilter<MaskImageType, MaskImageType> MaskShrinkerType;
	typename MaskShrinkerType::Pointer maskshrinker = MaskShrinkerType::New();
	maskshrinker->SetInput(maskImage);
	maskshrinker->SetShrinkFactors(shrinkFactor);

	shrinker->Update();
	maskshrinker->Update();

	itk::TimeProbe timer;
	timer.Start();

	correcter->SetInput(shrinker->GetOutput());
	correcter->SetMaskImage(maskshrinker->GetOutput());

	typedef CommandIterationUpdate<CorrecterType> CommandType;
	typename CommandType::Pointer observer = CommandType::New();
	correcter->AddObserver(itk::IterationEvent(), observer);
	observer->SetProgressObject(&progress, numIters);

	/**
	* histogram sharpening options
	*/
	//correcter->SetBiasFieldFullWidthAtHalfMaximum(0.15);
	//correcter->SetWienerFilterNoise(0.01);
	//correcter->SetNumberOfHistogramBins(200);

	try
	{
		// correcter->DebugOn();
		correcter->Update();
	}
	catch (itk::ExceptionObject& e)
	{
		if (verbose)
		{
			std::cerr << "Exception caught: " << e << std::endl;
		}
		m_CurrentFilter = ITK_NULLPTR;
		return ITK_NULLPTR;
	}

	m_CurrentFilter = ITK_NULLPTR;

	if (verbose)
	{
		correcter->Print(std::cout, 3);
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
	typedef itk::BSplineControlPointImageFilter<
			typename CorrecterType::BiasFieldControlPointLatticeType,
			typename CorrecterType::ScalarImageType>
			BSplinerType;
	typename BSplinerType::Pointer bspliner = BSplinerType::New();
	bspliner->SetInput(correcter->GetLogBiasFieldControlPointLattice());
	bspliner->SetSplineOrder(correcter->GetSplineOrder());
	bspliner->SetSize(inputImage->GetLargestPossibleRegion().GetSize());
	bspliner->SetOrigin(inputImage->GetOrigin());
	bspliner->SetDirection(inputImage->GetDirection());
	bspliner->SetSpacing(inputImage->GetSpacing());
	bspliner->Update();

	typename ImageType::Pointer logField = AllocImage<ImageType>(inputImage);

	itk::ImageRegionIterator<typename CorrecterType::ScalarImageType> ItB(
			bspliner->GetOutput(), bspliner->GetOutput()->GetLargestPossibleRegion());
	itk::ImageRegionIterator<ImageType> ItF(logField,
			logField->GetLargestPossibleRegion());
	for (ItB.GoToBegin(), ItF.GoToBegin(); !ItB.IsAtEnd(); ++ItB, ++ItF)
	{
		ItF.Set(ItB.Get()[0]);
	}

	typedef itk::ExpImageFilter<ImageType, ImageType> ExpFilterType;
	typename ExpFilterType::Pointer expFilter = ExpFilterType::New();
	expFilter->SetInput(logField);
	expFilter->Update();

	typedef itk::DivideImageFilter<ImageType, ImageType, ImageType> DividerType;
	typename DividerType::Pointer divider = DividerType::New();
	divider->SetInput1(inputImage);
	divider->SetInput2(expFilter->GetOutput());
	divider->Update();

	if (isMaskImageSpecified)
	{
		itk::ImageRegionIteratorWithIndex<ImageType> ItD(
				divider->GetOutput(), divider->GetOutput()->GetLargestPossibleRegion());
		itk::ImageRegionIterator<ImageType> ItI(
				inputImage, inputImage->GetLargestPossibleRegion());
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
