/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <boost/test/unit_test.hpp>

#include "itkRGBToLabColorSpacePixelAccessor.h"

#include <itkCastImageFilter.h>
#include <itkImageAdaptor.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkSLICImageFilter.h>

#include <itkImageIOFactory.h>
#include <itkMetaImageIOFactory.h>

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Data_suite);

BOOST_AUTO_TEST_CASE(SLIC)
{
	itk::MetaImageIOFactory::RegisterOneFactory();

	std::string inFileName = "/Users/lloyd/Models/iSeg_TestData/mr_T1/slice_004.mhd";
	std::string outFileName = "/Users/lloyd/Models/iSeg_TestData/mr_T1/slice_004_labels.mhd";

	using RGBPixelType = itk::RGBPixel<unsigned char>;
	using RGBImageType = itk::Image<RGBPixelType, 2>;
	using RGBToLabType = itk::Accessor::RGBToLabColorSpacePixelAccessor<unsigned char, float>;
	using RGBToLabAdaptorType = itk::ImageAdaptor<RGBImageType, RGBToLabType>;

	//using ImageType = itk::Image<itk::Vector<float,3>, 2>;	//itk::VectorImage<float, 2>;
	using ImageType = itk::Image<float, 2>;									//itk::VectorImage<float, 2>;
	using LabelImageType = itk::Image<unsigned int, 2>;							// int not supported by PNG writer
	using OutputImageType = itk::Image<float, 2>;									//itk::VectorImage<float, 2>;
	using SLICFilterType = itk::SLICImageFilter<ImageType, LabelImageType>;

	//using ReaderType = itk::ImageFileReader<RGBImageType>;
	using ReaderType = itk::ImageFileReader<ImageType>;
	using WriterType = itk::ImageFileWriter<OutputImageType>;

	auto reader = ReaderType::New();
	reader->SetFileName(inFileName);
	reader->Update();

	auto aniso = itk::GradientAnisotropicDiffusionImageFilter<ImageType, ImageType>::New();
	aniso->SetInput(reader->GetOutput());
	aniso->SetNumberOfIterations(5);
	aniso->SetTimeStep(0.125);
	aniso->SetConductanceParameter(1.0);

	auto rescale = itk::RescaleIntensityImageFilter<ImageType>::New();
	rescale->SetInput(reader->GetOutput());
	rescale->SetOutputMinimum(0);
	rescale->SetOutputMaximum(255.0);

	/*
	auto rgbToLabAdaptor = RGBToLabAdaptorType::New();
	rgbToLabAdaptor->SetImage(reader->GetOutput());

	using CastType = itk::CastImageFilter<RGBToLabAdaptorType, ImageType>;
	auto cast = CastType::New();
	cast->SetInput(rgbToLabAdaptor);
	*/
	auto filter = SLICFilterType::New();
	//filter->SetNumberOfWorkUnits(1);
	filter->SetSpatialProximityWeight(15.0);
	filter->SetMaximumNumberOfIterations(600);
	filter->SetSuperGridSize(30);
	filter->SetEnforceConnectivity(true);
	filter->SetInitializationPerturbation(true);
	filter->SetInput(rescale->GetOutput());
	filter->Update();

	using CastType = itk::CastImageFilter<LabelImageType, OutputImageType>;
	auto cast = CastType::New();
	cast->SetInput(filter->GetOutput());

	auto writer = WriterType::New();
	writer->SetFileName(outFileName);
	writer->SetInput(cast->GetOutput());
	writer->Update();
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();
