#ifndef itkFixTopologyCarveOutside_hxx
#define itkFixTopologyCarveOutside_hxx

#include "itkFixTopologyCarveOutside.h"

#include "Graph.h"
#include "TopologyInvariants.h"
#include "itkLabelRegionCalculator.h"

#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkNeighborhoodAlgorithm.h>
#include <itkNeighborhoodIterator.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>

#include <iostream>
#include <queue>
#include <vector>

namespace itk {

using namespace topology;

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
void FixTopologyCarveOutside<TInputImage, TOutputImage>::PrepareData()
{
	auto inputImage = dynamic_cast<const InputImageType*>(ProcessObject::GetInput(0));
	auto thinImage = GetThinning();

	// Get bounding box around voxels with inside value
	auto region = itk::GetLabelRegion<InputImageType>(inputImage, this->m_InsideValue);
	if (!thinImage->GetRequestedRegion().IsInside(region))
	{
		itkExceptionMacro(<< "Requested region does not contain inside region");
	}

	thinImage->SetBufferedRegion(thinImage->GetRequestedRegion());
	thinImage->Allocate();
	thinImage->FillBuffer(m_OutsideValue);

	ImageRegionIterator<TOutputImage> ot(thinImage, region);
	ot.GoToBegin();

	for (ot.GoToBegin(); !ot.IsAtEnd(); ++ot)
	{
		ot.Set(m_InsideValue);
	}
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::ComputeThinImage()
{
	auto inputImage = dynamic_cast<const InputImageType*>(ProcessObject::GetInput(0));
	auto thinImage = GetThinning();

	// Get bounding box around voxels with inside value
	auto region = itk::GetLabelRegion<InputImageType>(inputImage, this->m_InsideValue);

	ConstBoundaryConditionType boundaryCondition;
	boundaryCondition.SetConstant(m_OutsideValue);

	typename NeighborhoodIteratorType::RadiusType radius;
	radius.Fill(1);
	NeighborhoodIteratorType ot(radius, thinImage, region);
	ot.SetBoundaryCondition(boundaryCondition);

	// Compute distance - negative values inside
	using DistanceImageType = itk::Image<float, InputImageDimension>;
	auto distance_filter = itk::SignedMaurerDistanceMapImageFilter<TInputImage, DistanceImageType>::New();
	distance_filter->SetInput(inputImage);
	distance_filter->UseImageSpacingOn();
	distance_filter->SquaredDistanceOn();
	distance_filter->InsideIsPositiveOff();
	distance_filter->SetBackgroundValue(m_OutsideValue);
	distance_filter->GetOutput()->SetRequestedRegion(region);
	distance_filter->Update();
	auto distance_map = distance_filter->GetOutput();

	// '1' if pixel has been queued already
	using UCharImageType = itk::Image<unsigned char, 3>;
	auto visited = UCharImageType::New();
	visited->SetRegions(region);
	visited->Allocate();
	visited->FillBuffer(0);

	// Insert seed points in queue
	using index_t = IndexType; // or std::array<unsigned short,3>?
	using node = std::pair<float, index_t>;
	struct comp
	{
		bool operator()(const node& l, const node& r) const { return (l.first < r.first); }
	};
	std::priority_queue<node, std::vector<node>, comp> queue; // largest first

	// Find the data-set boundary "faces"
	NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<itk::Image<unsigned char, 3>> bC;
	auto faceList = bC(visited, region, radius);
	faceList.pop_front(); // face '0' is interior region
	for (const auto& f : faceList)
	{
		auto bit = itk::ImageRegionConstIterator<UCharImageType>(visited, f);

		for (bit.GoToBegin(); !bit.IsAtEnd(); ++bit)
		{
			auto dist = distance_map->GetPixel(bit.GetIndex());
			if (dist > 0.f)
			{
				visited->SetPixel(bit.GetIndex(), 1);
				queue.push(std::make_pair(dist, bit.GetIndex()));
			}
		}
	}

	// Define graph
	auto entireRegion = thinImage->GetLargestPossibleRegion();
	std::array<typename index_t::IndexValueType, 3> dims = {
			(typename index_t::IndexValueType)entireRegion.GetSize(0),
			(typename index_t::IndexValueType)entireRegion.GetSize(1),
			(typename index_t::IndexValueType)entireRegion.GetSize(2)};
	std::array<double, 3> spacing = {
			thinImage->GetSpacing()[0],
			thinImage->GetSpacing()[1],
			thinImage->GetSpacing()[2]};
	XCore::CUniformGridGraph<index_t, 3> graph(dims, spacing);

	// Carve from outside
	while (true)
	{
		std::priority_queue<node, std::vector<node>, comp> delayed_queue;
		int num_carved = 0;

		while (!queue.empty())
		{
			auto id = queue.top().second; // node
			auto d = queue.top().first; // node cost
			queue.pop();

			ot.SetLocation(id);

			// Check if point is Euler invariant
			bool can_carve = false;
			if (EulerInvariant(ot.GetNeighborhood(), m_InsideValue))
			{
				if (!m_EnforceManifold || !NonmanifoldRemove(ot.GetNeighborhood(), m_InsideValue))
				{
					// Check if point is simple (deletion does not change connectivity in the 3x3x3 neighborhood)
					if (CCInvariant(ot.GetNeighborhood(), m_InsideValue))
					{
						can_carve = true;
					}
				}
			}

			if (can_carve)
			{
				thinImage->SetPixel(id, m_OutsideValue);
				num_carved++;
			}
			else
			{
				delayed_queue.push(std::make_pair(d - 0.5f, id));
			}

			// update neighboring values
			auto neighbors = graph.Neighbors(id);
			for (const auto& neighbor : neighbors)
			{
				// skip pixels outside
				if (!region.IsInside(neighbor))
				{
					continue;
				}

				// skip if neighbor is inside object
				if (inputImage->GetPixel(neighbor) != m_InsideValue && visited->GetPixel(neighbor) == 0)
				{
					visited->SetPixel(neighbor, 1);
					queue.push(std::make_pair(distance_map->GetPixel(neighbor), neighbor));
				}
			}
		}

		if (num_carved == 0)
			break;

		queue = delayed_queue;
	}
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::GenerateData()
{
	this->PrepareData();

	this->ComputeThinImage();
}

template<class TInputImage, class TOutputImage>
void FixTopologyCarveOutside<TInputImage, TOutputImage>::PrintSelf(std::ostream& os, Indent indent) const
{
	Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif // itkFixTopologyCarveOutside_hxx
