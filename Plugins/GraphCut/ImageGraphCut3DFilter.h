/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef __ImageGraphCut3DFilter_h_
#define __ImageGraphCut3DFilter_h_

// ITK
#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkHistogram.h>
#include <itkImage.h>
#include <itkImageDuplicator.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToImageFilter.h>
#include <itkListSample.h>
#include <itkMatrix.h>
#include <itkProgressReporter.h>
#include <itkRecursiveGaussianImageFilter.h>
#include <itkSampleToHistogramFilter.h>
#include <itkShapedNeighborhoodIterator.h>
#include <itkSymmetricEigenAnalysis.h>

// STL
#include <string>
#include <vector>

// Gc
#include "Flow/Grid/Kohli.h"
#include "Flow/Grid/PushRelabel/Fifo.h"
#include "Flow/Grid/PushRelabel/HighestLevel.h"

namespace itk {
template<typename TInput, typename TForeground, typename TBackground,
		 typename TOutput>
class ITK_EXPORT ImageGraphCut3DFilter
	: public ImageToImageFilter<TInput, TOutput>
{
public:
	// ITK related defaults
	typedef ImageGraphCut3DFilter Self;
	typedef ImageToImageFilter<TInput, TOutput> Superclass;
	typedef SmartPointer<Self> Pointer;
	typedef SmartPointer<const Self> ConstPointer;

	itkNewMacro(Self);

	itkTypeMacro(ImageGraphCut3DFilter, ImageToImageFilter);
	itkStaticConstMacro(NDimension, unsigned int, TInput::ImageDimension);

	// image types
	typedef TInput InputImageType;
	typedef TForeground ForegroundImageType;
	typedef TBackground BackgroundImageType;
	typedef TOutput OutputImageType;

	typedef itk::Statistics::Histogram<short,
									   itk::Statistics::DenseFrequencyContainer2>
		HistogramType;
	typedef std::vector<itk::Index<3>>
		IndexContainerType; // container for sinks / sources
	typedef enum { NoDirection,
				   BrightDark,
				   DarkBright } BoundaryDirectionType;

	enum eMaxFlowAlgorithm {
		kKohli = 0,
		kPushLabelFifo = 1,
		kPushLabelHighestLevel = 2,
	};

	using ProcessObject::SetNumberOfRequiredInputs;

	// parameter setters
	void SetSigma(double d) { m_Sigma = d; }

	void SetFB(bool b) { m_UseForegroundBackground = b; }

	void SetGM(bool b) { m_UseGradientMagnitude = b; }

	void SetIntensity(bool b) { m_UseIntensity = b; }

	void SetConnectivity(bool b) { m_6Connected = b; }

	void SetForeground(int b) { m_ForegroundValue = b; }

	void SetBackground(int b) { m_BackgroundValue = b; }

	void SetMaxFlowAlgorithm(eMaxFlowAlgorithm alg) { m_MaxFlowAlgorithm = alg; }

	void SetForegroundPixelValue(typename OutputImageType::PixelType v)
	{
		m_ForegroundPixelValue = v;
	}

	void SetBackgroundPixelValue(typename OutputImageType::PixelType v)
	{
		m_BackgroundPixelValue = v;
	}

	// image setters
	void SetInputImage(const InputImageType* image)
	{
		this->SetNthInput(0, const_cast<InputImageType*>(image));
	}

	void SetForegroundImage(const ForegroundImageType* image)
	{
		this->SetNthInput(1, const_cast<ForegroundImageType*>(image));
	}

	void SetBackgroundImage(const BackgroundImageType* image)
	{
		this->SetNthInput(2, const_cast<BackgroundImageType*>(image));
	}

	void SetVerboseOutput(bool b) { m_PrintTimer = b; }

private:
	ImageGraphCut3DFilter();
	virtual ~ImageGraphCut3DFilter();

	virtual void GenerateData() override;

private:
	struct ImageContainer
	{
		typename InputImageType::ConstPointer input;
		typename InputImageType::RegionType inputRegion;
		typename ForegroundImageType::ConstPointer foreground;
		typename BackgroundImageType::ConstPointer background;
		typename OutputImageType::Pointer output;
		typename InputImageType::RegionType outputRegion;
	};
	typedef itk::Vector<typename InputImageType::PixelType, 1>
		ListSampleMeasurementVectorType;
	typedef itk::Statistics::ListSample<ListSampleMeasurementVectorType>
		SampleType;
	typedef itk::Statistics::SampleToHistogramFilter<SampleType, HistogramType>
		SampleToHistogramFilterType;
	typedef Gc::Flow::IGridMaxFlow<3, Gc::Float32, Gc::Float32, Gc::Float32>
		GraphType;

	void InitializeGraph(GraphType*, ImageContainer, ProgressReporter& progress);

	void CutGraph(GraphType*, ImageContainer, ProgressReporter& progress);

	// convert 3d itk indices to a continously numbered indices
	unsigned int
		ConvertIndexToVertexDescriptor(const itk::Index<3>,
									   typename InputImageType::RegionType);

	// image getters
	const InputImageType* GetInputImage()
	{
		return static_cast<const InputImageType*>(
			this->ProcessObject::GetInput(0));
	}

	const ForegroundImageType* GetForegroundImage()
	{
		if (GetNumberOfRequiredInputs() == 1)
			return nullptr;
		return static_cast<const ForegroundImageType*>(
			this->ProcessObject::GetInput(1));
	}

	const BackgroundImageType* GetBackgroundImage()
	{
		if (GetNumberOfRequiredInputs() == 1)
			return nullptr;
		return static_cast<const BackgroundImageType*>(
			this->ProcessObject::GetInput(2));
	}

	// parameters
	bool m_UseForegroundBackground;
	bool m_UseIntensity;
	bool m_UseGradientMagnitude;
	eMaxFlowAlgorithm m_MaxFlowAlgorithm;
	bool m_6Connected;
	int m_ForegroundValue;
	int m_BackgroundValue;
	double m_Sigma;				 // noise in boundary term
	int m_NumberOfHistogramBins; // bins per dimension of histograms
	typename OutputImageType::PixelType m_ForegroundPixelValue;
	typename OutputImageType::PixelType m_BackgroundPixelValue;
	bool m_PrintTimer;

private:
	ImageGraphCut3DFilter(const Self&); // intentionally not implemented
	void operator=(const Self&);		// intentionally not implemented
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION

#	include "ImageGraphCut3DFilter.hxx"

#endif

#endif //__ImageGraphCut3DFilter_h_
