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

#include "LookupTable.h"

#include <itkImageRegionIterator.h>
#include <itkImageToImageFilter.h>

namespace itk {

template<class TInputImage, class TOutputImage>
class OpenThinning : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
	/** Standard class typedefs. */
	using Self = OpenThinning;
	using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
	using Pointer = SmartPointer<Self>;
	using ConstPointer = SmartPointer<const Self>;

	/** Method for creation through the object factory */
	itkNewMacro(Self);

	/** Run-time type information (and related methods). */
	itkTypeMacro(OpenThinning, ImageToImageFilter);

	/** ImageDimension enumeration   */
	itkStaticConstMacro(ImageDimension, unsigned int, TInputImage::ImageDimension);

	/** Type for input image. */
	using InputImageType = TInputImage;

	/** Type for output image. */
	using OutputImageType = TOutputImage;

	/** Type for the pixel type of the input image. */
	using InputImagePixelType = typename InputImageType::PixelType;

	/** Type for the pixel type of the input image. */
	using OutputImagePixelType = typename OutputImageType::PixelType;

	void SetLookupTable(const LookupTable& lut);

	/**  Compute thinning Image. */
	static void ComputeThinImage(OutputImageType* output, const LookupTable& lookupTable);

#ifdef ITK_USE_CONCEPT_CHECKING
	/** Begin concept checking */
	itkConceptMacro(SameDimensionCheck, (Concept::SameDimension<ImageDimension, 3>));
	itkConceptMacro(OutputConvertibleToIntCheck, (Concept::Convertible<OutputImagePixelType, int>));
	/** End concept checking */
#endif

protected:
	OpenThinning();
	virtual ~OpenThinning(){};
	void PrintSelf(std::ostream& os, Indent indent) const override;

	/** Compute thinning Image. */
	void GenerateData() override;

private:
	OpenThinning(const Self&);	 //purposely not implemented
	void operator=(const Self&); //purposely not implemented

	LookupTable m_LookupTable;
};

} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#	include "itkOpenThinning.hxx"
#endif
