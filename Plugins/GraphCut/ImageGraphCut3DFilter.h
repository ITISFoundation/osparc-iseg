/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
#include <itkImage.h>
#include <itkImageDuplicator.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToImageFilter.h>
#include <itkMatrix.h>
#include <itkProgressReporter.h>
#include <itkRecursiveGaussianImageFilter.h>
#include <itkShapedNeighborhoodIterator.h>
#include <itkSymmetricEigenAnalysis.h>
#include <itkTimeProbesCollectorBase.h>
//#include <itkMultiScaleHessianBasedMeasureImageFilter>

// STL
#include <fstream>
#include <string>
#include <vector>

// Gc
#include "Flow/Grid/Kohli.h"
#include "Flow/Grid/PushRelabel/Fifo.h"
#include "Flow/Grid/PushRelabel/HighestLevel.h"

namespace itk {

template<typename TInput, typename TForeground, typename TBackground, typename TOutput>
class ITK_EXPORT ImageGraphCutFilter : public ImageToImageFilter<TInput, TOutput>
{
public:
	// ITK related defaults
	using Self = ImageGraphCutFilter;
	using Superclass = ImageToImageFilter<TInput, TOutput>;
	using Pointer = SmartPointer<Self>;
	using ConstPointer = SmartPointer<const Self>;

	itkNewMacro(Self);

	itkTypeMacro(ImageGraphCutFilter, ImageToImageFilter);
	itkStaticConstMacro(NDimension, unsigned int, TInput::ImageDimension);

	// image types
	using InputImageType = TInput;
	using ForegroundImageType = TForeground;
	using BackgroundImageType = TBackground;
	using OutputImageType = TOutput;

	using IndexContainerType = std::vector<itk::Index<3>>; // container for sinks / sources

	enum BoundaryDirectionType {
		NoDirection,
		BrightDark,
		DarkBright
	} BoundaryDirectionType;

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
	ImageGraphCutFilter();
	~ImageGraphCutFilter() override {}

	void GenerateData() override;

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
	using GraphType = Gc::Flow::IGridMaxFlow<3, Gc::Float32, Gc::Float32, Gc::Float32>;

	void InitializeGraph(GraphType*, ImageContainer, ProgressReporter& progress);

	void CutGraph(GraphType*, ImageContainer, ProgressReporter& progress);

	// convert 3d itk indices to a continously numbered indices
	unsigned int ConvertIndexToVertexDescriptor(const itk::Index<3>, typename InputImageType::RegionType);

	// image getters
	const InputImageType* GetInputImage()
	{
		return dynamic_cast<const InputImageType*>(this->ProcessObject::GetInput(0));
	}

	// parameters
	bool m_UseForegroundBackground = false;
	bool m_UseIntensity = false;
	bool m_UseGradientMagnitude = false;
	eMaxFlowAlgorithm m_MaxFlowAlgorithm;
	bool m_6Connected = false;
	int m_ForegroundValue = 400;
	int m_BackgroundValue = -50;
	double m_Sigma = 0.2;				 // noise in boundary term
	int m_NumberOfHistogramBins; // bins per dimension of histograms
	typename OutputImageType::PixelType m_ForegroundPixelValue;
	typename OutputImageType::PixelType m_BackgroundPixelValue;
	bool m_PrintTimer = true;

private:
	ImageGraphCutFilter(const Self&); // intentionally not implemented
	void operator=(const Self&);			// intentionally not implemented
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION

#	include "ImageGraphCut3DFilter.hxx"

#endif

#endif //__ImageGraphCut3DFilter_h_
