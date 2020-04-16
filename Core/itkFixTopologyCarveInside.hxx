#ifndef itkFixTopologyCarveInside_hxx
#define itkFixTopologyCarveInside_hxx

#include "itkFixTopologyCarveInside.h"

#include "Graph.h"
#include "TopologyInvariants.h"
#include "itkLabelRegionCalculator.h"

#include <itkConnectedComponentImageFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkNeighborhoodAlgorithm.h>
#include <itkNeighborhoodIterator.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>

#include <iostream>
#include <queue>
#include <vector>

namespace itk {

template<class TInputImage, class TOutputImage, class TConnectedComponentImage>
FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::FixTopologyCarveInside()
{
	this->SetNumberOfRequiredOutputs(1);
}

template<class TInputImage, class TOutputImage, class TConnectedComponentImage>
typename FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::OutputImageType*
		FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::GetThinning()
{
	return dynamic_cast<OutputImageType*>(this->ProcessObject::GetOutput(0));
}

template<class TInputImage, class TOutputImage, class TConnectedComponentImage>
void FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::PrepareData()
{
	auto thinImage = GetThinning();
	thinImage->SetBufferedRegion(thinImage->GetRequestedRegion());
	thinImage->Allocate();
	thinImage->FillBuffer(m_OutsideValue);
}

template<class TInputImage, class TOutputImage, class TConnectedComponentImage>
void FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::ComputeThinImage()
{
	using namespace topology;

	auto inputImage = dynamic_cast<const InputImageType*>(ProcessObject::GetInput(0));
	auto thinImage = GetThinning();

	// Get bounding box around voxels with inside value
	auto region = itk::GetLabelRegion<InputImageType>(inputImage, this->m_InsideValue);
	if (!thinImage->GetRequestedRegion().IsInside(region))
	{
		itkExceptionMacro(<< "Requested region does not contain inside region");
	}

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
	distance_filter->InsideIsPositiveOn(); // inside is positive
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

	// Add seeds
	{
		auto ccfilter = itk::ConnectedComponentImageFilter<InputImageType, TConnectedComponentImage>::New();
		ccfilter->SetInput(inputImage);
		ccfilter->SetBackgroundValue(m_OutsideValue);
		ccfilter->FullyConnectedOff(); // face-connected off
		ccfilter->Update();
		auto components_image = ccfilter->GetOutput();

		// Find most interior (positive) seed point for each component
		std::vector<node> best_seeds(ccfilter->GetObjectCount(), std::make_pair(-1.f, index_t()));

		auto cit = itk::ImageRegionConstIterator<TConnectedComponentImage>(components_image, region);
		auto dit = itk::ImageRegionConstIterator<DistanceImageType>(distance_map, region);
		for (cit.GoToBegin(), dit.GoToBegin(); !cit.IsAtEnd(); ++cit, ++dit)
		{
			if (cit.Get() > 0)
			{
				// map to zero-based index
				int c = cit.Get() - 1;

				// inside distances are positive
				if (dit.Get() > best_seeds.at(c).first)
				{
					best_seeds[c].first = dit.Get();
					best_seeds[c].second = dit.GetIndex();
				}
			}
		}

		// Add seeds to queue
		for (const auto& seed : best_seeds)
		{
			thinImage->SetPixel(seed.second, m_InsideValue);
			visited->SetPixel(seed.second, 1);
			queue.push(seed);
		}

		/*
		auto dx = thinImage->GetSpacing();
		auto inside_threshold = 2.5*(dx[0] + dx[1] + dx[2]);
		for (dit.GoToBegin(); !dit.IsAtEnd(); ++dit)
		{
			if (dit.Get() > inside_threshold)
			{
				node seed{dit.Get(), dit.GetIndex()};
				thinImage->SetPixel(seed.second, m_InsideValue);
				visited->SetPixel(seed.second, 1);
				queue.push(seed);
			}
		}
		*/
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
			// Original seed points
			if (thinImage->GetPixel(id) == m_InsideValue)
			{
				can_carve = true;
			}
			else if (EulerInvariant(ot.GetNeighborhood(), m_OutsideValue))
			{
				// Check if point is simple (deletion does not change connectivity in the 3x3x3 neighborhood)
				if (CCInvariant(ot.GetNeighborhood(), m_OutsideValue))
				{
					can_carve = true;
				}
			}

			if (can_carve)
			{
				thinImage->SetPixel(id, m_InsideValue);
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
				// skip if neighbor is not inside object
				if (inputImage->GetPixel(neighbor) == m_InsideValue && visited->GetPixel(neighbor) == 0)
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

template<class TInputImage, class TOutputImage, class TConnectedComponentImage>
void FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::GenerateData()
{
	this->PrepareData();

	this->ComputeThinImage();
}

template<class TInputImage, class TOutputImage, class TConnectedComponentImage>
void FixTopologyCarveInside<TInputImage, TOutputImage, TConnectedComponentImage>::PrintSelf(std::ostream& os, Indent indent) const
{
	Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif // itkFixTopologyCarveInside_hxx
