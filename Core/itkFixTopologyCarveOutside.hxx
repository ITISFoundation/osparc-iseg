#ifndef itkFixTopologyCarveOutside_hxx
#define itkFixTopologyCarveOutside_hxx

#include "itkFixTopologyCarveOutside.h"
#include "TopologyInvariants.h"

#include "../Data/ItkUtils.h"
#include "../Data/Logger.h"

#include "itkBinaryDilateImageFilter.h"
#include "itkFlatStructuringElement.h"
#include "itkImageAlgorithm.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkNeighborhoodIterator.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"

#include <iostream>
#include <vector>

namespace itk {

template<class TInputImage, class TOutputImage>
FixTopologyCarveOutside<TInputImage, TOutputImage>::FixTopologyCarveOutside()
{
	this->SetNumberOfRequiredOutputs(1);
}

template<class TInputImage, class TOutputImage>
typename FixTopologyCarveOutside<TInputImage, TOutputImage>::OutputImageType*
		FixTopologyCarveOutside<TInputImage, TOutputImage>::GetThinning()
{
	return dynamic_cast<OutputImageType*>(this->ProcessObject::GetOutput(0));
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::PrepareData(ProgressAccumulator* progress)
{
	InputImagePointer input_image = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
	OutputImagePointer thin_image = GetThinning();
	thin_image->SetBufferedRegion(thin_image->GetRequestedRegion());
	thin_image->Allocate();

	// pad by 1 layer so we can get 1x1x1 neighborhood without checking if we are at boundary
	auto region = thin_image->GetRequestedRegion();
	auto padded_region = region;
	padded_region.PadByRadius(1);

	m_PaddedMask = OutputImageType::New();
	m_PaddedMask->SetRegions(padded_region);
	m_PaddedMask->SetSpacing(thin_image->GetSpacing());
	m_PaddedMask->Allocate();
	m_PaddedMask->FillBuffer(0);

	ImageRegionConstIterator<TInputImage> it(input_image, region);
	ImageRegionIterator<TOutputImage> ot(m_PaddedMask, region);

	it.GoToBegin();
	ot.GoToBegin();

	// Copy the input to the output, changing all foreground pixels to
	// have value 1 in the process.
	while (!ot.IsAtEnd())
	{
		if (it.Get() == m_InsideValue)
		{
			ot.Set(ePixelState::kHardForeground);
		}
		else
		{
			ot.Set(ePixelState::kBackground);
		}
		++it;
		++ot;
	}

	ISEG_INFO_MSG("Compute sdf");
	// Compute distance to object
	auto dmap = itk::SignedMaurerDistanceMapImageFilter<TOutputImage, itk::Image<float, 3>>::New();
	progress->RegisterInternalFilter(dmap, 0.1);
	dmap->SetInput(m_PaddedMask);
	dmap->UseImageSpacingOn();
	dmap->SquaredDistanceOn();
	dmap->InsideIsPositiveOff(); // inside is negative
	dmap->SetBackgroundValue(ePixelState::kBackground);
	dmap->Update();
	m_DistanceMap = dmap->GetOutput();

	// Dilate by specified thickness
	itk::Size<3> radius;
	radius.Fill(m_Radius);
	using kernel_type = itk::FlatStructuringElement<3>;
	auto ball = kernel_type::Ball(radius, false);

	ISEG_INFO_MSG("Dilate");
	auto dilate = itk::BinaryDilateImageFilter<TOutputImage, TOutputImage, kernel_type>::New();
	progress->RegisterInternalFilter(dmap, 0.1);
	dilate->SetInput(m_PaddedMask);
	dilate->SetKernel(ball);
	dilate->SetForegroundValue(NumericTraits<OutputImagePixelType>::One);
	dilate->Update();

	// mark dilated as '2', but don't copy padding layer
	ImageRegionConstIterator<TOutputImage> dt(dilate->GetOutput(), region);
	for (dt.GoToBegin(), ot.GoToBegin(); !ot.IsAtEnd(); ++dt, ++ot)
	{
		if (dt.Get() != ot.Get())
		{
			ot.Set(ePixelState::kDilated);
		}
	}
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::ComputeThinImage(ProgressAccumulator* accumulator)
{
	OutputImagePointer thin_image = GetThinning();
	// note output image not padded, all processing is done on m_PaddedMask, i.e. padded image
	auto region = thin_image->GetRequestedRegion();
	auto padded_region = m_PaddedMask->GetBufferedRegion();

	// buffers of mask/distance map
	OutputImagePixelType* mask_p = m_PaddedMask->GetPixelContainer()->GetImportPointer();

	// prepare progress progress 
	auto num_dilated = std::count(mask_p, mask_p + padded_region.GetNumberOfPixels(), kDilated);
	ProgressReporter progress(this, 0, num_dilated, 100, accumulator->GetAccumulatedProgress());

	ISEG_INFO_MSG("Mark boundary");
	// add seed points image faces where '2'
	// padded pixels are '0', get boundary of unpadded region
	itk::Size<3> radius = {1, 1, 1};
	auto r = itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<TOutputImage>::Compute(*thin_image, region, radius);
	for (const auto& face : r.GetBoundaryFaces())
	{
		auto bit = itk::ImageRegionIterator<TOutputImage>(m_PaddedMask, face);
		for (bit.GoToBegin(); !bit.IsAtEnd(); ++bit)
		{
			if (bit.Get() == kDilated)
			{
				bit.Set(kVisited);
			}
		}
	}

	// now add seed points at interface '2'-'0'
	const auto dims = padded_region.GetSize();
	const size_t dim2d[][2] = {{1, 2}, {0, 2}, {0, 1}};
	const std::ptrdiff_t strides[] = {1, static_cast<std::ptrdiff_t>(dims[0]), static_cast<std::ptrdiff_t>(dims[0] * dims[1])};

	const auto process_line = [mask_p](size_t pos, size_t stride, size_t N) {
		OutputImagePixelType prev_label = -1;

		size_t idx = pos;
		for (size_t i = 0; i < N; ++i, idx += stride)
		{
			// find interface
			if (mask_p[idx] != prev_label)
			{
				if (prev_label == kBackground && mask_p[idx] == kDilated)
				{
					mask_p[idx] = kVisited;
				}
				else if (prev_label == kDilated && mask_p[idx] == kBackground)
				{
					mask_p[idx - stride] = kVisited;
				}
				prev_label = mask_p[idx];
			}
		}
	};

	for (size_t d0 = 0; d0 < 3; ++d0)
	{
		size_t d1 = dim2d[d0][0];
		size_t d2 = dim2d[d0][1];

		for (size_t i1 = 0; i1 < dims[d1]; ++i1)
		{
			for (size_t i2 = 0; i2 < dims[d2]; ++i2)
			{
				// test along direction 'd'
				process_line(i1 * strides[d1] + i2 * strides[d2], strides[d0], dims[d0]);
			}
		}
	}

	// get seeds
	using node_t = std::pair<float, IndexType>;
	struct comp
	{
		bool operator()(const node_t& l, const node_t& r) const { return (l.first < r.first); }
	};
	std::priority_queue<node_t, std::vector<node_t>, comp> queue;
	auto bit = itk::ImageRegionConstIteratorWithIndex<TOutputImage>(m_PaddedMask, region);
	for (bit.GoToBegin(); !bit.IsAtEnd(); ++bit)
	{
		if (bit.Get() == kVisited)
		{
			auto idx = bit.GetIndex();
			auto sdf = m_DistanceMap->GetPixel(idx);
			queue.push(node_t(sdf, idx));
		}
	}

	// neighborhood iterator
	NeighborhoodIteratorType n_it(radius, m_PaddedMask, region);
	using offset_type = typename NeighborhoodIteratorType::OffsetType;
	std::array<offset_type, 6> neighbors = {
			offset_type{-1, 0, 0},
			offset_type{1, 0, 0},
			offset_type{0, -1, 0},
			offset_type{0, 1, 0},
			offset_type{0, 0, -1},
			offset_type{0, 0, 1}
	};

	const auto mask = [](NeighborhoodType& n) { for (auto& v : n.GetBufferReference()) v = (v != 0) ? 1 : 0; };

	// erode while topology does not change
	ISEG_INFO_MSG("Erode from boundary");
	while (!queue.empty())
	{
		progress.CompletedPixel();

		auto node = queue.top();
		auto id = node.second;
		queue.pop();

		if (m_PaddedMask->GetPixel(id) != kVisited)
		{
			continue;
		}

		n_it.SetLocation(id);
		auto vals = n_it.GetNeighborhood();
		mask(vals);

		// check if pixel can be eroded
		// check if point is simple (deletion does not change connectivity in the 3x3x3 neighborhood)
		if (topology::EulerInvariant(vals, 1) &&
				topology::CCInvariant(vals, 1) &&
				topology::CCInvariant(vals, 0))
		{
			// We cannot delete current point, so reset
			m_PaddedMask->SetPixel(id, kBackground);
		}

		// add unvisited neighbors to queue
		for (const auto& offset : neighbors)
		{
			const auto nid = id + offset;
			if (m_PaddedMask->GetPixel(nid) == kDilated)
			{
				// mark as visited
				m_PaddedMask->SetPixel(nid, kVisited);

				// add to queue
				queue.push(node_t(m_DistanceMap->GetPixel(nid), nid));
			}
		}
	}

	// copy to output
	InputImagePointer input_image = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
	itk::ImageRegionConstIterator<TInputImage> i_it(input_image, region);
	itk::ImageRegionConstIterator<TOutputImage> s_it(m_PaddedMask, region);
	itk::ImageRegionIterator<TOutputImage> o_it(thin_image, region);
	
	for (i_it.GoToBegin(), s_it.GoToBegin(), o_it.GoToBegin(); !s_it.IsAtEnd(); ++i_it, ++s_it, ++o_it)
	{
		o_it.Set(s_it.Get() != 0 ? m_InsideValue : i_it.Get());
	}
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::GenerateData()
{
	ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
	progress->SetMiniPipelineFilter(this);

	this->PrepareData(progress);

	this->ComputeThinImage(progress);
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::PrintSelf(std::ostream& os, Indent indent) const
{
	Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif // itkFixTopologyCarveOutside_hxx
