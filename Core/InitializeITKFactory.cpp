/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include <boost/thread/once.hpp>

namespace iseg {

void initializeITKFactory()
{
	static boost::once_flag once_flag = BOOST_ONCE_INIT;
	boost::call_once(once_flag, []() {
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
} _instance;

} // namespace iseg
