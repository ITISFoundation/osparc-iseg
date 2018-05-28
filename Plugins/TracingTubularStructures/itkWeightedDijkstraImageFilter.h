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

#include <itkConstNeighborhoodIterator.h>
#include <itkImageToPathFilter.h>
#include <itkNeighborhoodAlgorithm.h>
#include <itkPolyLineParametricPath.h>

#include <queue>
#include <utility>
#include <vector>

namespace itk {
template<typename ImageType>
class MyMetric
{
public:
	typedef typename ImageType::PixelType ValueType;
	typedef typename ImageType::SpacingValueType SpacingValueType;

	MyMetric() : m_StartValue(0), m_EndValue(0) {}

	ValueType m_IntensityWeight = 1;
	ValueType m_AngleWeight = 1;
	ValueType m_LengthWeight = 1;

	void Initialize(const ImageType* image, typename ImageType::IndexType start, typename ImageType::IndexType end)
	{
		m_Image = image;
		m_Spacing = m_Image->GetSpacing();
		m_StartValue = m_Image->GetPixel(start);
		m_EndValue = m_Image->GetPixel(end);
	}

	ValueType GetEdgeWeight(const typename ImageType::IndexType& i, const typename ImageType::IndexType& j, const typename ImageType::IndexType& iprev) const
	{
		auto value_i = this->m_Image->GetPixel(i);
		auto value_j = this->m_Image->GetPixel(j);

		auto iDifference = fabs(value_j - value_i);
		auto sIDifference = fabs(value_j - m_StartValue) + fabs(value_j - m_EndValue);

		using bit64 = long long;
		bit64 dir1[] = {j[0] - (bit64)i[0], j[1] - (bit64)i[1], j[2] - (bit64)i[2]};
		bit64 dir0[] = {(bit64)i[0] - iprev[0], (bit64)i[1] - iprev[1], (bit64)i[2] - iprev[2]};
		auto pLength = ComputeLength(dir1[0] * m_Spacing[0], dir1[1] * m_Spacing[1], dir1[2] * m_Spacing[2]);
		auto pSmoothness = ComputeAngle(
				dir0[0] * m_Spacing[0], dir0[1] * m_Spacing[1], dir0[2] * m_Spacing[2],
				dir1[0] * m_Spacing[0], dir1[1] * m_Spacing[1], dir1[2] * m_Spacing[2]);

		return m_IntensityWeight * (iDifference + sIDifference) + m_LengthWeight * pLength + m_AngleWeight * pSmoothness;
	}

	inline SpacingValueType ComputeLength(SpacingValueType x, SpacingValueType y, SpacingValueType z) const
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	// computes max(0, 1 - cos( Theta )), not the actual angle, i.e. value is in range[0,2]
	inline SpacingValueType ComputeAngle(
			SpacingValueType x1, SpacingValueType y1, SpacingValueType z1,
			SpacingValueType x2, SpacingValueType y2, SpacingValueType z2) const
	{
		// same direction:     1 - 1    = 0
		// opposite direction: 1 - (-1) = 2
		auto num = x1 * x2 + y1 * y2 + z1 * z2;
		return (num == 0) ? 1 : (1 - num / (ComputeLength(x1, y1, z1) * ComputeLength(x2, y2, z2)));
	}

private:
	typename ImageType::SpacingType m_Spacing;
	typename ImageType::ConstPointer m_Image;
	ValueType m_StartValue;
	ValueType m_EndValue;
};

template<typename TInputImageType, typename TMetric = MyMetric<TInputImageType>>
class WeightedDijkstraImageFilter : public ImageToPathFilter<TInputImageType, PolyLineParametricPath<3>>
{
public:
	typedef TInputImageType ImageType;
	typedef PolyLineParametricPath<3> PathType;
	typedef typename TInputImageType::IndexType IndexType;
	typedef typename TInputImageType::PixelType PixelType;

	typedef WeightedDijkstraImageFilter Self;
	typedef ImageToPathFilter<ImageType, PathType> Superclass;
	typedef SmartPointer<Self> Pointer;
	typedef SmartPointer<const Self> ConstPointer;

	itkNewMacro(Self);
	itkTypeMacro(WeightedDijkstraImageFilter, ImageToPathFilter);

	void SetStartIndex(const typename TInputImageType::IndexType& StartIndex);
	void SetEndIndex(const typename TInputImageType::IndexType& EndIndex);

	void SetRegion(const typename TInputImageType::RegionType region)
	{
		m_Region = region;
	}

	TMetric& GetMetric()
	{
		return m_Metric;
	}

protected:
	WeightedDijkstraImageFilter();
	virtual ~WeightedDijkstraImageFilter() {}
	void PrintSelf(std::ostream& os, Indent indent) const override;

	void GenerateData() override;

private:
	ITK_DISALLOW_COPY_AND_ASSIGN(WeightedDijkstraImageFilter);

	TMetric m_Metric;
	typename TInputImageType::IndexType m_StartIndex, m_EndIndex;
	typename TInputImageType::RegionType m_Region;
};

} // end of namespace itk

#include "itkWeightedDijkstraImageFilter.hxx"
