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

#include "Data/ItkUtils.h"
#include "Data/Types.h"

#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <itkTranslationTransform.h>

#include "itkSliceContiguousImage.h"

#include <vector>

namespace iseg {

class ConnectedShapeBasedInterpolation
{
public:
	using image_type = itk::Image<float, 2>;
	using image3_type = itk::Image<float, 3>;
	using mask_type = itk::Image<unsigned char, 2>;
	using mask3_type = itk::Image<unsigned char, 3>;
	using labefield_type = itk::Image<unsigned short, 2>;
	using image_stack_type = itk::SliceContiguousImage<float>;

	std::vector<image_type::Pointer> interpolate(const labefield_type* tissues1, const labefield_type* tissues2, tissues_size_t tissue_label, int number_of_slices, bool include_input_slices)
	{
		auto mask_tissues = [](const labefield_type* tissues, tissues_size_t tissuetype) {
			auto masker = itk::BinaryThresholdImageFilter<labefield_type, image_type>::New();
			masker->SetInput(tissues);
			masker->SetLowerThreshold(tissuetype);
			masker->SetUpperThreshold(tissuetype);
			masker->SetInsideValue(255.0);
			masker->SetOutsideValue(0.0);
			masker->Update();
			return image_type::Pointer(masker->GetOutput());
		};
		auto img1 = mask_tissues(tissues1, tissue_label);
		auto img2 = mask_tissues(tissues2, tissue_label);
		auto slices = interpolate(img1, img2, number_of_slices);
		if (include_input_slices)
		{
			slices.insert(slices.begin(), img1);
			slices.push_back(img2);
		}
		return slices;
	}

	std::vector<image_type::Pointer> interpolate(const image_type* slice1, const image_type* slice2, int number_of_slices)
	{
		int num_objects1 = 0;
		int num_objects2 = 0;
		const auto objects1 = get_components(slice1, num_objects1);
		const auto objects2 = get_components(slice2, num_objects2);
		if (num_objects1 != num_objects2 || num_objects2 == 0)
		{
			throw std::runtime_error("Slices don't have same number of foreground objects.");
		}

		const auto center_of_mass1 = get_center_of_mass(objects1, num_objects1);
		const auto center_of_mass2 = get_center_of_mass(objects2, num_objects2);

		auto get_closest = [](const std::vector<itk::Point<double, 2>>& pts1, const std::vector<itk::Point<double, 2>>& pts2) {
			std::map<size_t, size_t> corresponding_objects;
			for (size_t i = 0; i < pts1.size(); ++i)
			{
				double closest_dist = std::numeric_limits<double>::max();
				size_t closest_id = 0;
				for (size_t j = 0; j < pts2.size(); ++j)
				{
					double dist = pts1[i].SquaredEuclideanDistanceTo(pts2[j]);
					if (dist < closest_dist)
					{
						closest_dist = dist;
						closest_id = j;
					}
				}
				corresponding_objects[i] = closest_id;
			}
			return corresponding_objects;
		};

		auto map12 = get_closest(center_of_mass1, center_of_mass2);
		auto map21 = get_closest(center_of_mass2, center_of_mass1);
		for (auto p : map12)
		{
			if (map21.find(p.second) == map21.end() || map21.at(p.second) != p.first)
			{
				throw std::runtime_error("Foreground objects cannot be unambiguously assigned.");
			}
		}

		std::vector<image_type::Pointer> interpolated_slices(number_of_slices);
		for (auto p : map12)
		{
			auto object1 = p.first;
			auto object2 = p.second;

			auto x1 = center_of_mass1.at(object1);
			auto x2 = center_of_mass2.at(object2);

			itk::Vector<double, 2> translation;
			translation[0] = x1[0] - x2[0];
			translation[1] = x1[1] - x2[1];
			auto slice2_moved = move_image(slice2, -translation);

			auto sdf1 = compute_sdf<image_type>(slice1);
			auto sdf2 = compute_sdf<image_type>(slice2_moved);

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
			spacing[2] = 1.0; // unit spacing
			origin[2] = 0.0; // zero

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

			auto mask = resample(image3d, number_of_slices);

			for (int i = 0; i < number_of_slices; i++)
			{
				auto size = mask->GetBufferedRegion().GetSize();
				size[2] = 0;
				auto start = mask->GetBufferedRegion().GetIndex();
				start[2] = i;
				itk::ImageRegion<3> region(start, size);

				auto threed2twod = itk::ExtractImageFilter<mask3_type, image_type>::New();
				threed2twod->SetInput(mask);
				threed2twod->SetDirectionCollapseToIdentity();
				threed2twod->SetExtractionRegion(region);
				threed2twod->Update();

				double ratio = (i + 1) / (number_of_slices + 1.0);
				auto slice = move_image(threed2twod->GetOutput(), ratio * translation);

				if (interpolated_slices[i])
				{
					itk::ImageRegionIterator<image_type> sit(slice, slice->GetBufferedRegion());
					itk::ImageRegionIterator<image_type> dit(interpolated_slices[i], slice->GetBufferedRegion());

					for (sit.GoToBegin(), dit.GoToBegin(); !sit.IsAtEnd() && !dit.IsAtEnd(); ++sit, ++dit)
					{
						dit.Set(sit.Get() != 0 ? sit.Get() : dit.Get());
					}
				}
				else
				{
					interpolated_slices[i] = slice;
				}
			}
		}
		return interpolated_slices;
	}

private:
	/// find connected foreground regions (called "objects")
	mask_type::Pointer get_components(const image_type* img, int& num_objects) const
	{
		ScopedTimer timer("ConnectedComponents");

		auto caster = itk::CastImageFilter<image_type, mask_type>::New();
		caster->SetInput(img);
		auto connectivity = itk::ConnectedComponentImageFilter<mask_type, mask_type>::New();
		connectivity->SetInput(caster->GetOutput());
		connectivity->SetFullyConnected(true);
		connectivity->SetBackgroundValue(0.f);
		connectivity->Update();
		num_objects = connectivity->GetObjectCount();
		return connectivity->GetOutput();
	}

	/// compute center of mass of foreground object, assumes BG=0
	std::vector<itk::Point<double, 2>> get_center_of_mass(const mask_type* img, int num_objects) const
	{
		ScopedTimer timer("CenterOfMass");

		using idx_type = itk::ContinuousIndex<double, 2>;
		std::vector<double> n(num_objects, 0.0);
		std::vector<idx_type> idx(num_objects, idx_type({0, 0}));

		itk::ImageRegionConstIteratorWithIndex<mask_type> it(img, img->GetBufferedRegion());
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			int const id = static_cast<int>(it.Get()) - 1;
			if (id >= 0)
			{
				n[id]++;
				idx[id][0] += it.GetIndex()[0];
				idx[id][1] += it.GetIndex()[1];
			}
		}

		std::vector<itk::Point<double, 2>> pts(num_objects);
		for (int id = 0; id < num_objects; ++id)
		{
			if (n[id] > 0)
			{
				idx[id][0] /= n[id];
				idx[id][1] /= n[id];
			}
			img->TransformContinuousIndexToPhysicalPoint(idx[id], pts[id]);
		}
		return pts;
	}

	/// shift image by a specified amount
	template<class TImage>
	typename TImage::Pointer move_image(const TImage* img, const itk::Vector<double, 2>& translation) const
	{
		ScopedTimer timer("MoveImage");

		using resample_filter_type = itk::ResampleImageFilter<TImage, TImage>;
		using translation_type = itk::TranslationTransform<double, 2>;

		auto resample_filter = resample_filter_type::New();
		resample_filter->SetInput(img);

		auto nn_interpolator = itk::NearestNeighborInterpolateImageFunction<TImage>::New();
		resample_filter->SetInterpolator(nn_interpolator);

		auto transform = translation_type::New();
		transform->Translate(translation);
		resample_filter->SetTransform(transform.GetPointer());

		resample_filter->SetOutputParametersFromImage(img);
		resample_filter->Update();
		return resample_filter->GetOutput();
	}

	/// compute signed distance function, assumes BG=0
	template<class TImage>
	image_type::Pointer compute_sdf(const TImage* img) const
	{
		ScopedTimer timer("SignedDistance");

		using distance_filter_type = itk::SignedMaurerDistanceMapImageFilter<TImage, image_type>;
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
	mask3_type::Pointer resample(image_stack_type* image3d, size_t num_slices) const
	{
		ScopedTimer timer("Resample");

		auto id = itk::IdentityTransform<double, 3>::New();
		auto origin = image3d->GetOrigin();
		origin[2] = 1.0 / (num_slices + 1);
		auto size = image3d->GetBufferedRegion().GetSize();
		size[2] = num_slices;
		auto spacing = image3d->GetSpacing();
		spacing[2] = origin[2];

		auto resample_filter = itk::ResampleImageFilter<image_stack_type, image3_type>::New();
		resample_filter->SetTransform(id.GetPointer());
		resample_filter->SetInput(image3d);
		resample_filter->SetOutputParametersFromImage(image3d);
		resample_filter->SetOutputOrigin(origin);
		resample_filter->SetOutputSpacing(spacing);
		resample_filter->SetSize(size);

		auto threshold_filter = itk::BinaryThresholdImageFilter<image3_type, mask3_type>::New();
		threshold_filter->SetInput(resample_filter->GetOutput());
		threshold_filter->SetLowerThreshold(-std::numeric_limits<float>::max());
		threshold_filter->SetUpperThreshold(0.f);
		threshold_filter->SetOutsideValue(0);
		threshold_filter->SetInsideValue(255);
		threshold_filter->Update();
		return threshold_filter->GetOutput();
	}
};

} // namespace iseg
