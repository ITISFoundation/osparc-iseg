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

#include <itkCastImageFilter.h>
#include <itkTranslationTransform.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkResampleImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConnectedComponentImageFilter.h>

#include "itkSliceContiguousImage.h"

#include <vector>

namespace iseg {

class ConnectedShapeBasedInterpolation
{
public:
	using image_type = itk::Image<float, 2>;
	using image_stack_type = itk::SliceContiguousImage<float>;

	std::vector<image_type::Pointer> interpolate(const image_type* slice1, const image_type* slice2, int number_of_slices)
	{
		if (!check_exactly_one_object(slice1) || !check_exactly_one_object(slice2))
		{
			throw std::runtime_error("Detected multiple foreground objects");
		}
		auto x1 = get_center_of_mass(slice1);
		auto x2 = get_center_of_mass(slice2);

		itk::Vector<double, 2> translation;
		translation[0] = x1[0] - x2[0];
		translation[1] = x1[1] - x2[1];
		auto slice2_moved = move_image(slice2, -translation);

		auto sdf1 = compute_sdf(slice1);
		auto sdf2 = compute_sdf(slice2_moved);

		// slack to 3d image
		using image_stack_type = itk::SliceContiguousImage<float>;
		image_stack_type::IndexType start;
		image_stack_type::SizeType size;
		image_stack_type::SpacingType spacing;
		image_stack_type::PointType origin;
		for (int i = 0; i < 2; i++)
		{
			start[i] = slice1->GetBufferedRegion().GetIndex()[i];
			size[i] = slice1->GetBufferedRegion().GetSize()[i];
			spacing[i] = slice1->GetSpacing()[i];
			origin[i] = slice1->GetOrigin()[i];
		}
		start[2] = 0;
		size[2] = 2;
		spacing[2] = 1.0; // TODO
		origin[2] = 0.0;

		auto image3d = image_stack_type::New();
		image3d->SetRegions(image_stack_type::RegionType(start, size));
		image3d->SetSpacing(spacing);
		image3d->SetOrigin(origin);

		std::vector<float*> slices;
		slices.push_back(sdf1->GetPixelContainer()->GetImportPointer());
		slices.push_back(sdf2->GetPixelContainer()->GetImportPointer());

		auto container = image_stack_type::PixelContainer::New();
		container->SetImportPointersForSlices(slices, size[0] * size[1], false);
		image3d->SetPixelContainer(container);

		std::vector<image_type::Pointer> interpolated_slices;
		for (int i = 1; i <= number_of_slices; i++)
		{
			auto slice = interpolate(image3d, i / (number_of_slices + 1.0), translation);
			interpolated_slices.push_back(slice);
		}
		return interpolated_slices;
	}

private:
	bool check_exactly_one_object(const image_type* img) const
	{
		using mask_type = itk::Image<unsigned char, 2>;
		auto caster = itk::CastImageFilter<image_type, mask_type>::New();
		caster->SetInput(img);
		auto connectivity = itk::ConnectedComponentImageFilter<mask_type, mask_type>::New();
		connectivity->SetInput(caster->GetOutput());
		connectivity->SetFullyConnected(true);
		connectivity->SetBackgroundValue(0.f);
		connectivity->Update();
		return connectivity->GetObjectCount() == 1;
	}

	/// compute center of mass of foreground object, assumes BG=0
	itk::Point<double, 2> get_center_of_mass(const image_type* img) const
	{
		double n = 0;
		itk::ContinuousIndex<double, 2> idx({ 0,0 });
		itk::ImageRegionConstIteratorWithIndex<image_type> it(img, img->GetBufferedRegion());
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			if (it.Get() > 0.f)
			{
				n++;
				idx[0] += it.GetIndex()[0];
				idx[1] += it.GetIndex()[1];
			}
		}
		if (n > 0)
		{
			idx[0] /= n;
			idx[1] /= n;
		}
		itk::Point<double, 2> pt;
		img->TransformContinuousIndexToPhysicalPoint(idx, pt);
		return pt;
	}

	/// shift image by a specified amount
	image_type::Pointer move_image(const image_type* img, const itk::Vector<double,2>& translation) const
	{
		using resample_filter_type = itk::ResampleImageFilter<image_type, image_type>;
		using translation_type = itk::TranslationTransform<double, 2>;
		
		auto resample_filter = resample_filter_type::New();
		resample_filter->SetInput(img);
		
		auto nn_interpolator = itk::NearestNeighborInterpolateImageFunction<image_type>::New();
		resample_filter->SetInterpolator(nn_interpolator);

		auto transform = translation_type::New();
		transform->Translate(translation);
		resample_filter->SetTransform(transform.GetPointer());

		resample_filter->SetOutputParametersFromImage(img);
		resample_filter->Update();
		return resample_filter->GetOutput();
	}

	/// compute signed distance function, assumes BG=0
	image_type::Pointer compute_sdf(const image_type* img) const
	{
		using distance_filter_type = itk::SignedMaurerDistanceMapImageFilter<image_type, image_type>;
		auto dt_filter1 = distance_filter_type::New();
		dt_filter1->SetInput(img);
		dt_filter1->SetBackgroundValue(0); // ?
		dt_filter1->SetInsideIsPositive(false);
		dt_filter1->SetSquaredDistance(false);
		dt_filter1->SetUseImageSpacing(true);
		dt_filter1->Update();
		return dt_filter1->GetOutput();
	}

	/// interpolate between known slices
	image_type::Pointer interpolate(image_stack_type* image3d, double ratio_12, const itk::Vector<double, 2>& translation_12) const
	{
		auto id = itk::IdentityTransform<double, 3>::New();
		auto origin = image3d->GetOrigin();
		origin[2] = ratio_12;
		auto size = image3d->GetBufferedRegion().GetSize();
		size[2] = 1;

		using image_3d_type = itk::Image<float, 3>;
		auto resample_filter = itk::ResampleImageFilter<image_stack_type, image_3d_type>::New();
		resample_filter->SetTransform(id.GetPointer());
		resample_filter->SetInput(image3d);
		resample_filter->SetOutputParametersFromImage(image3d);
		resample_filter->SetOutputOrigin(origin);
		resample_filter->SetSize(size);
		itk::ImageRegion<3> region;
		region.SetSize(0, size[0]);
		region.SetSize(1, size[1]);

		auto threed2twod = itk::ExtractImageFilter<image_3d_type, image_type>::New();
		threed2twod->SetInput(resample_filter->GetOutput());
		threed2twod->SetDirectionCollapseToIdentity();
		threed2twod->SetExtractionRegion(region);
		threed2twod->Update();

		auto threshold_filter = itk::BinaryThresholdImageFilter<image_type, image_type>::New();
		threshold_filter->SetInput(threed2twod->GetOutput());
		threshold_filter->SetLowerThreshold(-std::numeric_limits<float>::max());
		threshold_filter->SetUpperThreshold(0.f);
		threshold_filter->SetOutsideValue(0);
		threshold_filter->SetInsideValue(255);
		threshold_filter->Update();
		auto mask = threshold_filter->GetOutput();

		return move_image(mask, ratio_12 * translation_12);
	};

};

} // namespace iseg