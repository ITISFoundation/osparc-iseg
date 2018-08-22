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

#include <itkImageToImageFilter.h>
#include <itkImageRegionIterator.h>

namespace itk {

template<class TInputImage>
class PalagyiKubaThinningImageFilter : public ImageToImageFilter<TInputImage, itk::Image<unsigned char,3> >
{
public:
	using OutputImage = itk::Image<unsigned char,3>;
	
	/** Standard class typedefs. */
	using Self = PalagyiKubaThinningImageFilter;
	using Superclass = ImageToImageFilter<TInputImage, OutputImage>;
	using Pointer = SmartPointer<Self>;
	using ConstPointer = SmartPointer<const Self>;

	/** Method for creation through the object factory */
	itkNewMacro(Self);

	/** Run-time type information (and related methods). */
	itkTypeMacro(PalagyiKubaThinningImageFilter, ImageToImageFilter);

protected:
	PalagyiKubaThinningImageFilter();
	virtual ~PalagyiKubaThinningImageFilter(){};

	/** Compute thinning Image. */
	void GenerateData() override;

private:
	PalagyiKubaThinningImageFilter(const Self&); //purposely not implemented
	void operator=(const Self&);				 //purposely not implemented
};

} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#	include "itkPalagyiKubaThinningImageFilter.hxx"
#endif
