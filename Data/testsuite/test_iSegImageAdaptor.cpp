/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
BOOST_AUTO_TEST_SUITE(Data_suite);

BOOST_AUTO_TEST_CASE(isegImageAdaptor)
{
	size_t slice_shape[2] = {12, 24};
	std::vector<float> data(slice_shape[0] * slice_shape[1], 1.0);
	std::vector<float*> slices(12, data.data());

	using image_type = itk::SliceContiguousImage<float>;

	auto image = image_type::New();
	image_type::IndexType start;
	start.Fill(0);
	image_type::SizeType size;
	size[0] = slice_shape[0];
	size[1] = slice_shape[1];
	size[2] = slices.size();
	image_type::RegionType region(start, size);
	image->SetRegions(region);
	// NOT required, unless we want to allocate memory here:
	// image->Allocate();

	// Set slice pointers
	bool container_manage_memory = false;
	auto container = image_type::PixelContainer::New();
	container->SetImportPointersForSlices(slices, size[0] * size[1], container_manage_memory);
	image->SetPixelContainer(container);

	{
		using output_image_type = itk::Image<float, 3>;
		using gaussian_filter_type = itk::RecursiveGaussianImageFilter<image_type, output_image_type>;
		auto gaussian = gaussian_filter_type::New();
		gaussian->SetSigma(1.0);
		gaussian->SetInput(image);
		BOOST_REQUIRE_NO_THROW(gaussian->Update());
	}
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();
