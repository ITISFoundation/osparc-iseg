/*=========================================================================
*
*  Copyright Insight Software Consortium
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*         http://www.apache.org/licenses/LICENSE-2.0.txt
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*=========================================================================*/

// \author Bryn Lloyd
// \note Code modified from itkLabelVotingBinaryImageFilter.h/hxx

#ifndef itkLabelVotingBinaryImageFilter_hxx
#define itkLabelVotingBinaryImageFilter_hxx
#include "itkLabelVotingBinaryImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"

#include <vector>
#include <algorithm>

namespace itk {
template< typename TInputImage, typename TOutputImage >
LabelVotingBinaryImageFilter< TInputImage, TOutputImage >
::LabelVotingBinaryImageFilter()
{
	m_Radius.Fill(1);
	m_ForegroundValue = NumericTraits< InputPixelType >::max();
	m_BackgroundValue = NumericTraits< InputPixelType >::ZeroValue();
}

template< typename TInputImage, typename TOutputImage >
void
LabelVotingBinaryImageFilter< TInputImage, TOutputImage >
::GenerateInputRequestedRegion()
{
	// call the superclass' implementation of this method
	Superclass::GenerateInputRequestedRegion();

	// get pointers to the input and output
	typename Superclass::InputImagePointer inputPtr =
	    const_cast< TInputImage * >(this->GetInput());
	typename Superclass::OutputImagePointer outputPtr = this->GetOutput();

	if (!inputPtr || !outputPtr)
	{
		return;
	}

	// get a copy of the input requested region (should equal the output
	// requested region)
	typename TInputImage::RegionType inputRequestedRegion;
	inputRequestedRegion = inputPtr->GetRequestedRegion();

	// pad the input requested region by the operator radius
	inputRequestedRegion.PadByRadius(m_Radius);

	// crop the input requested region at the input's largest possible region
	if (inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()))
	{
		inputPtr->SetRequestedRegion(inputRequestedRegion);
		return;
	}
	else
	{
		// Couldn't crop the region (requested region is outside the largest
		// possible region).  Throw an exception.

		// store what we tried to request (prior to trying to crop)
		inputPtr->SetRequestedRegion(inputRequestedRegion);

		// build an exception
		InvalidRequestedRegionError e(__FILE__, __LINE__);
		e.SetLocation(ITK_LOCATION);
		e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
		e.SetDataObject(inputPtr);
		throw e;
	}
}

template< typename TInputImage, typename TOutputImage >
void
LabelVotingBinaryImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	m_NumberOfPixelsChanged = 0;

	m_Count.store(0);
}

template< typename TInputImage, typename TOutputImage >
void
LabelVotingBinaryImageFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{
	m_NumberOfPixelsChanged = m_Count.load();
}

template< typename TInputImage, typename TOutputImage >
void
LabelVotingBinaryImageFilter< TInputImage, TOutputImage >
::DynamicThreadedGenerateData(const OutputImageRegionType &outputRegionForThread)
{
	using PixelType = typename InputImageType::PixelType;

	class DenseMap
	{
	public:
		typedef PixelType                          K;
		typedef unsigned int                       V;
		typedef std::pair<K, V>                    value_type;
		typedef std::vector<value_type>            container_type;
		typedef typename container_type::iterator  iterator;

		inline size_t size() const { return m_Data.size(); }
		inline void clear(){ m_Data.clear(); }
		inline void reserve(size_t sz) { m_Data.reserve(sz); }

		inline iterator begin() { return m_Data.begin(); }
		inline iterator end() { return m_Data.end(); }

		inline V &operator[](const K &key)
		{
			auto it = std::find_if(m_Data.begin(), m_Data.end(),
				[&key](const value_type& e) { return (e.first == key); });
			if (it != end())
				return it->second;
			m_Data.push_back(std::make_pair(key, V(0)));
			return m_Data.back().second;
		}

	private:
		container_type m_Data;
	};

	ConstantBoundaryCondition< InputImageType > dbc;
	dbc.SetConstant(m_BackgroundValue);

	//ZeroFluxNeumannBoundaryCondition< InputImageType > nbc;

	ConstNeighborhoodIterator< InputImageType > bit;
	ImageRegionIterator< OutputImageType >      it;

	// Allocate output
	typename OutputImageType::Pointer output = this->GetOutput();
	typename InputImageType::ConstPointer input = this->GetInput();

	// Find the data-set boundary "faces"
	typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType faceList;
	NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType > bC;
	faceList = bC(input, outputRegionForThread, m_Radius);

	typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType::iterator fit;

	DenseMap histogram;
	histogram.reserve(8 * m_Radius[0] * m_Radius[1] * m_Radius[2]);

	auto more_often = [](const typename DenseMap::value_type & l, const typename DenseMap::value_type & r)
	{
		return (l.second > r.second);
	};

	unsigned int numberOfPixelsChanged = 0;

	// Process the internal region with no boundary and the boundaries.
	// -> the first 'face' is internal wo boundary
	for (fit = faceList.begin(); fit != faceList.end(); ++fit)
	{
		bit = ConstNeighborhoodIterator< InputImageType >(m_Radius, input, *fit);
		it = ImageRegionIterator< OutputImageType >(output, *fit);

		bit.OverrideBoundaryCondition(&dbc);
		bit.GoToBegin();

		unsigned int neighborhoodSize = bit.Size();

		while (!bit.IsAtEnd())
		{
			const InputPixelType inpixel = bit.GetCenterPixel();

			it.Set(static_cast<OutputPixelType>(inpixel));

			// only re-assign foreground pixels
			if (inpixel == m_ForegroundValue)
			{
				// count the pixels ON in the neighborhood
				histogram.clear();
				for (unsigned int i = 0; i < neighborhoodSize; ++i)
				{
					InputPixelType value = bit.GetPixel(i);
					if (value != m_BackgroundValue && value != m_ForegroundValue)
					{
						histogram[value]++;
					}
				}

				// skip trivial cases
				if (histogram.size() >= 2)
				{
					std::stable_sort(histogram.begin(), histogram.end(), more_often);

					auto first = histogram.begin();
					auto second = std::next(histogram.begin());

					// overwrite if first has 'clear' majority
					if (first->second >= second->second + m_MajorityThreshold)
					{
						it.Set(static_cast<OutputPixelType>(first->first));

						if (first->first != inpixel)
						{
							numberOfPixelsChanged++;
						}
					}
				}
				else if (histogram.size() == 1)
				{
					auto first = histogram.begin();

					// overwrite if has at least X neighbors of this type
					if (first->second >= m_VotingThreshold)
					{
						it.Set(static_cast<OutputPixelType>(first->first));

						numberOfPixelsChanged++;
					}
				}
			}

			++bit;
			++it;
		}
	}

	// Do algorithm without handling threadId
	m_Count.fetch_add(numberOfPixelsChanged, std::memory_order_relaxed);
}

/**
* Standard "PrintSelf" method
*/
template< typename TInputImage, typename TOutput >
void
LabelVotingBinaryImageFilter< TInputImage, TOutput >
::PrintSelf(
    std::ostream &os,
    Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "Radius: " << m_Radius << std::endl;
	os << indent << "Foreground value : "
	   << static_cast< typename NumericTraits< InputPixelType >::PrintType >(m_ForegroundValue) << std::endl;
	os << indent << "Background value : "
	   << static_cast< typename NumericTraits< InputPixelType >::PrintType >(m_BackgroundValue) << std::endl;
	os << indent << "Majority Threshold : " << m_MajorityThreshold << std::endl;
}
} // end namespace itk

#endif
