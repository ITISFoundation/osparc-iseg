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

#include "itkOpenThinning.h"

namespace itk {

template<class TInputImage, class TOutputImage>
class MedialAxisImageFilter : public OpenThinning<TInputImage, TOutputImage>
{
public:
	/** Standard class typedefs. */
	using Self = MedialAxisImageFilter;
	using Superclass = OpenThinning<TInputImage, TOutputImage>;
	using Pointer = SmartPointer<Self>;
	using ConstPointer = SmartPointer<const Self>;

	/** Method for creation through the object factory */
	itkNewMacro(Self);

	/** Run-time type information (and related methods). */
	itkTypeMacro(MedialAxisImageFilter, OpenThinning);

protected:
	MedialAxisImageFilter() {}
	virtual ~MedialAxisImageFilter(){};

	/** Compute thinning Image. */
	void GenerateData() override;

private:
	MedialAxisImageFilter(const Self&); //purposely not implemented
	void operator=(const Self&);				//purposely not implemented
};

} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#	include "itkMedialAxisImageFilter.hxx"
#endif
