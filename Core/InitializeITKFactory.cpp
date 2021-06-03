/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "InitializeITKFactory.h"

#include <itkImageIOFactory.h>

#include <itkMetaImageIOFactory.h>
#include <itkNiftiImageIOFactory.h>
#include <itkNrrdImageIOFactory.h>
#include <itkObjectFactoryBase.h>
#include <itkVTKImageIOFactory.h>

#include <itkGDCMImageIOFactory.h>

#include <itkBMPImageIOFactory.h>
#include <itkJPEGImageIOFactory.h>
#include <itkPNGImageIOFactory.h>
#include <itkTIFFImageIOFactory.h>

#include <mutex>

namespace iseg {

void initializeITKFactory()
{
	static std::once_flag once_flag;
	std::call_once(once_flag, []() {
		itk::ObjectFactoryBase::RegisterFactory(itk::MetaImageIOFactory::New());
		itk::ObjectFactoryBase::RegisterFactory(itk::NiftiImageIOFactory::New());
		itk::ObjectFactoryBase::RegisterFactory(itk::NrrdImageIOFactory::New());
		itk::ObjectFactoryBase::RegisterFactory(itk::VTKImageIOFactory::New());

		itk::ObjectFactoryBase::RegisterFactory(itk::GDCMImageIOFactory::New());

		itk::ObjectFactoryBase::RegisterFactory(itk::BMPImageIOFactory::New());
		itk::ObjectFactoryBase::RegisterFactory(itk::JPEGImageIOFactory::New());
		itk::ObjectFactoryBase::RegisterFactory(itk::PNGImageIOFactory::New());
		itk::ObjectFactoryBase::RegisterFactory(itk::TIFFImageIOFactory::New());
	});
}

class AutoInit
{
public:
	AutoInit()
	{
		initializeITKFactory();
	}
} instance;

} // namespace iseg
