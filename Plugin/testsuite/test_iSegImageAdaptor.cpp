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

#include "itkSliceContiguousImage.h"

// example ITK filters to run using adaptor
#include <itkExtractImageFilter.h>
#include <itkRecursiveGaussianImageFilter.h>

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Addon_suite);

BOOST_AUTO_TEST_CASE(isegImageAdaptor)
{
	size_t slice_shape[2] = {12, 24};
	std::vector<float*> slices(12, nullptr);

	typedef itk::SliceContiguousImage<float> ImageType;

	ImageType::Pointer image = ImageType::New();
	ImageType::IndexType start;
	start.Fill(0);
	ImageType::SizeType size;
	size[0] = slice_shape[0];
	size[1] = slice_shape[1];
	size[2] = slices.size();
	ImageType::RegionType region(start, size);
	image->SetRegions(region);
	// NOT required, unless we want to allocate memory here:
	// image->Allocate();

	// Set slice pointers
	bool container_manage_memory = false;
	ImageType::PixelContainerPointer container = ImageType::PixelContainer::New();
	container->SetImportPointersForSlices(slices, size[0] * size[1],
										  container_manage_memory);
	image->SetPixelContainer(container);

	{
		typedef itk::Image<float, 3> OutputImageType;
		typedef itk::RecursiveGaussianImageFilter<ImageType, OutputImageType>
			GaussianFilterType;
		auto gaussian = GaussianFilterType::New();
		gaussian->SetSigma(1.0);
		gaussian->SetInput(image);
		BOOST_REQUIRE_NO_THROW(gaussian->Update());
	}
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();
