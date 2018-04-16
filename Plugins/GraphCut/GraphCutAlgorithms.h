/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

// ITK
#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>
#include <itkImage.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkProgressReporter.h>
#include <itkShapedNeighborhoodIterator.h>
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

enum eGcMaxFlowAlgorithm {
	kKohli = 0,
	kPushLabelFifo = 1,
	kPushLabelHighestLevel = 2,
};
enum eGcConnectivity {
	kFaceNeighbors,
	kNodeNeighbors
};

template<typename TInput, typename TOutput, typename TInputIntensityImage = TInput>
class ITK_EXPORT GraphCutLabelSeparator : public ImageToImageFilter<TInput, TOutput>
{
public:
	// ITK related defaults
	using Self = GraphCutLabelSeparator;
	using Superclass = ImageToImageFilter<TInput, TOutput>;
	using Pointer = SmartPointer<Self>;
	using ConstPointer = SmartPointer<const Self>;

	itkNewMacro(Self);
	itkTypeMacro(GraphCutLabelSeparator, ImageToImageFilter);
	itkStaticConstMacro(NDimension, unsigned int, TInput::ImageDimension);

	// image types
	using InputImageType = TInput;
	using IntensityImageType = TInputIntensityImage;
	using OutputImageType = TOutput;
	//using IndexContainerType = std::vector<itk::Index<3>>; // container for sinks / sources

	using ProcessObject::SetNumberOfRequiredInputs;

	// parameter setters
	void SetSigma(double d) { m_Sigma = d; }

	void SetConnectivity(eGcConnectivity type) { m_Connectivity = type; }

	void SetMaxFlowAlgorithm(eGcMaxFlowAlgorithm alg) { m_MaxFlowAlgorithm = alg; }

	void SetObject1Value(typename InputImageType::PixelType v) { m_Object1Value = v; }

	void SetObject2Value(typename InputImageType::PixelType v) { m_Object2Value = v; }

	void SetBackgroundValue(typename InputImageType::PixelType v) { m_BackgroundValue = v; }

	void SetUseGradientMagnitude(bool b) { m_UseGradientMagnitude = b; }

	// image setters
	void SetMaskInput(InputImageType* image)
	{
		this->ProcessObject::SetNthInput(0, image);
	}

	void SetIntensityInput(IntensityImageType* image)
	{
		this->ProcessObject::SetNthInput(1, image);
	}

	InputImageType* GetMaskInput()
	{
		return static_cast<InputImageType*>(this->ProcessObject::GetInput(0));
	}

	IntensityImageType* GetIntensityInput()
	{
		return static_cast<IntensityImageType*>(this->ProcessObject::GetInput(1));
	}

	void SetVerboseOutput(bool b) { m_PrintTimer = b; }

private:
	GraphCutLabelSeparator();
	virtual ~GraphCutLabelSeparator() {}
	virtual void GenerateData() override;

private:
	typedef Gc::Flow::IGridMaxFlow<3, Gc::Float32, Gc::Float32, Gc::Float32> GraphType;

	void InitializeGraph(GraphType*, ProgressReporter& progress);

	void CutGraph(GraphType*, ProgressReporter& progress);

	// convert 3d itk indices to a continuously numbered indices
	unsigned int ConvertIndexToVertexDescriptor(const itk::Index<3>, typename InputImageType::RegionType);

	double m_Sigma = 1.0;
	bool m_UseGradientMagnitude = false;
	eGcConnectivity m_Connectivity = eGcConnectivity::kNodeNeighbors;
	eGcMaxFlowAlgorithm m_MaxFlowAlgorithm = eGcMaxFlowAlgorithm::kKohli;

	typename InputImageType::PixelType m_BackgroundValue = 0;
	typename InputImageType::PixelType m_Object1Value = 127;
	typename InputImageType::PixelType m_Object2Value = 255;
	bool m_PrintTimer = false;

private:
	GraphCutLabelSeparator(const Self&); // intentionally not implemented
	void operator=(const Self&);				 // intentionally not implemented
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION

#	include "GraphCutAlgorithms.hxx"

#endif
