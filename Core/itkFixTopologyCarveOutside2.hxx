/*=========================================================================
 *
 *  Copyright NumFOCUS
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
// clang-format off
#ifndef itkFixTopologyCarveOutside2_hxx
#define itkFixTopologyCarveOutside2_hxx

#include "itkBinaryErodeImageFilter.h"
#include "itkFixTopologyCarveOutside2.h"
#include "itkFlatStructuringElement.h"
#include "itkImageAlgorithm.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkNeighborhoodIterator.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"

#include <iostream>
#include <vector>

namespace itk
{

template <class TInputImage, class TOutputImage>
FixTopologyCarveOutside2<TInputImage, TOutputImage>::FixTopologyCarveOutside2()
{
  this->SetNumberOfRequiredOutputs(1);

  // OutputImagePointer thinImage = OutputImageType::New();
  // this->SetNthOutput(0, thinImage.GetPointer());
}

template <class TInputImage, class TOutputImage>
typename FixTopologyCarveOutside2<TInputImage, TOutputImage>::OutputImageType *
FixTopologyCarveOutside2<TInputImage, TOutputImage>::GetThinning()
{
  return dynamic_cast<OutputImageType *>(this->ProcessObject::GetOutput(0));
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::PrepareData()
{
  // Copy the input image to the output image, changing from the input
  // type to the output type.

  OutputImagePointer thinImage = GetThinning();
  InputImagePointer inputImage = dynamic_cast<const TInputImage *>(ProcessObject::GetInput(0));

  thinImage->SetBufferedRegion(thinImage->GetRequestedRegion());
  thinImage->Allocate();

  typename OutputImageType::RegionType region = thinImage->GetRequestedRegion();

  ImageRegionConstIterator<TInputImage> it(inputImage, region);
  ImageRegionIterator<TOutputImage>     ot(thinImage, region);

  it.GoToBegin();
  ot.GoToBegin();

  // Copy the input to the output, changing all foreground pixels to
  // have value 1 in the process.
  while (!ot.IsAtEnd())
  {
    if (it.Get())
    {
      ot.Set(NumericTraits<OutputImagePixelType>::One);
    }
    else
    {
      ot.Set(NumericTraits<OutputImagePixelType>::Zero);
    }
    ++it;
    ++ot;
  }

  // Compute distance to object
  auto dmap = itk::SignedMaurerDistanceMapImageFilter<TOutputImage, itk::Image<float, 3>>::New();
  dmap->SetInput(thinImage);
  dmap->UseImageSpacingOn();
  dmap->SquaredDistanceOn();
  dmap->InsideIsPositiveOff(); // inside is negative
  dmap->SetBackgroundValue(0);
  dmap->Update();
  m_DistanceMap = dmap->GetOutput();

  // Dilate by specified thickness
  itk::Size<3> radius;
  radius.Fill(m_Radius);
  using kernel_type = itk::FlatStructuringElement<3>;
  auto ball = kernel_type::Ball(radius, false);

  auto dilate = itk::BinaryDilateImageFilter<TOutputImage, TOutputImage, kernel_type>::New();
  dilate->SetInput(thinImage);
  dilate->SetKernel(ball);
  dilate->SetDilateValue(2 * NumericTraits<OutputImagePixelType>::One);
  dilate->Update();

  itk::ImageAlgorithm::Copy<TOutputImage, TOutputImage>(dilate->GetOutput(), thinImage, region, region);
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::ComputeThinImage2()
{
  class NeighborAccessor
  {
  public:
    using value_type = OutputImagePixelType;
    using id_type = size_t;

    NeighborAccessor(value_type* buffer, const itk::Size<3>& dims)
        : m_Data(buffer)
    {
      std::ptrdiff_t m_StrideJ = dims[0], m_StrideK = dims[0]*dims[1];
      // assume non-boundary 'id'
      m_Offsets[0] = - 1 - m_StrideJ - m_StrideK;
      m_Offsets[1] = - m_StrideJ - m_StrideK;
      m_Offsets[2] = + 1 - m_StrideJ - m_StrideK;
      m_Offsets[3] = - 1 - m_StrideK;
      m_Offsets[4] = - m_StrideK;
      m_Offsets[5] = + 1 - m_StrideK;
      m_Offsets[6] = - 1 + m_StrideJ - m_StrideK;
      m_Offsets[7] = + m_StrideJ - m_StrideK;
      m_Offsets[8] = + 1 + m_StrideJ - m_StrideK;

      m_Offsets[9]  = - 1 - m_StrideJ;
      m_Offsets[10] = - m_StrideJ;
      m_Offsets[11] = + 1 - m_StrideJ;
      m_Offsets[12] = - 1;
      //m_Offsets[0] = 0; center is not neighbor
      m_Offsets[13] = + 1;
      m_Offsets[14] = - 1 + m_StrideJ;
      m_Offsets[15] = + m_StrideJ;
      m_Offsets[16] = + 1 + m_StrideJ;

      m_Offsets[17] = - 1 - m_StrideJ + m_StrideK;
      m_Offsets[18] = - m_StrideJ + m_StrideK;
      m_Offsets[19] = + 1 - m_StrideJ + m_StrideK;
      m_Offsets[20] = - 1 + m_StrideK;
      m_Offsets[21] = + m_StrideK;
      m_Offsets[22] = + 1 + m_StrideK;
      m_Offsets[23] = - 1 + m_StrideJ + m_StrideK;
      m_Offsets[24] = + m_StrideJ + m_StrideK;
      m_Offsets[25] = + 1 + m_StrideJ + m_StrideK;
    }

    void GetNeighbors(size_t id, std::array<value_type, 26>& vals) const
    {
      ;
    }

    const std::array<std::ptrdiff_t, 26>& Offsets() { return m_Offsets; }

  private:
    std::array<std::ptrdiff_t, 26> m_Offsets;
    std::ptrdiff_t m_StrideJ, m_StrideK;
    value_type* m_Data;
  };

  InputImagePointer inputImage = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
  OutputImagePointer thinImage = GetThinning();

  auto region = thinImage->GetRequestedRegion();

  // region id convention
  enum eValueType
  {
    kBackground = 0,
    kHardForeground,
    kDilated,
    kVisited
  };

  // boundary('2'): image faces where '2', or interface '2'-'0'
  itk::Size<3> radius = {1,1,1};
  itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<TOutputImage> face_calculator;
  auto faces = face_calculator(thinImage, region, radius);
  if (faces.size() == 7)
  {
    faces.pop_front();
  }

  // add seed points image faces where '2'
  for (const auto& face: faces)
  {
    auto bit = itk::ImageRegionIterator<TOutputImage>(thinImage, face);
    for (bit.GoToBegin(); !bit.IsAtEnd(); ++bit)
    {
      if (bit.Get() == kDilated)
      {
        bit.Set(kVisited);
      }
    }
  }

  // now add seed points at interface '2'-'0'
  const auto dims = region.Size();
  const size_t dim2d[][2] = {{1, 2}, {0, 2}, {0, 1}};
  const std::ptrdiff_t strides[] = {1, dims[0], dims[0] * dims[1]};

  const auto process_line = [](size_t pos, size_t stride, size_t N) {
    OutputImagePixelType prev_label = -1;
    OutputImagePixelType* tissues;

    size_t idx = pos;
    for (size_t i = 0; i < N; ++i, idx += stride)
    {
      // find interface
      if (tissues[idx] != prev_label)
      {
        if (prev_label == kBackground && tissues[idx] == kDilated)
        {
          tissues[idx] = kVisited;
        }
        else if (prev_label == kDilated && tissues[idx] == kBackground)
        {
          tissues[idx-stride] = kVisited;
        }
        prev_label = tissues[idx];
      }
    }
  };

  for (size_t d0 = 0; d0 < 3; ++d0)
  {
    size_t d1 = dim2d[d0][0], d2 = dim2d[d0][1];
    const size_t Nd = dims[d0];
    const size_t N1 = dims[d1];
    const size_t N2 = dims[d2];

    for (size_t i1 = 0; i1 < N1; ++i1)
    {
      for (size_t i2 = 0; i2 < N2; ++i2)
      {
        // test along direction 'd'
        process_line(i1 * strides[d1] + i2 * strides[d2], strides[d0], Nd);
      }
    }
  }

  // get seeds
  using node_t = std::pair<float, size_t>;
  std::priority_queue<node_t, std::vector<node_t>, std::greater<node_t>> queue;
  auto bit = itk::ImageConstIteratorWithIndex<TOutputImage>(thinImage, region);
  for (bit.GoToBegin(); !bit.IsAtEnd(); ++bit)
  {
    if (bit.Get() == kVisited)
    {
      auto idx = bit.GetIndex();
      auto sdf = m_DistanceMap->GetPixel(idx);
      queue.push(node_t(sdf, idx[0] + idx[1] * strides[1] + idx[2] * strides[2]));
    }
  }

  OutputImagePixelType* thin_image_p = thinImage->GetPixelContainer()->GetImportPointer();
  const float* distance_p = m_DistanceMap->GetPixelContainer()->GetImportPointer();

  NeighborAccessor na(thin_image_p, dims);
  std::array<OutputImagePixelType, 26> vals;
  const auto& offsets = na.Offsets();

  // erode while topology does not change
  while (!queue.empty())
  {
    auto id = queue.top().second;
    queue.pop();

    // check if pixel can be eroded
    if (thin_image_p[id] == kVisited)
    {
      na.GetNeighbors(id, vals);
    }

    // add unvisited neighbors to queue
    for (auto offset: offsets)
    {
      const size_t nid = id + offset;
      if (thin_image_p[nid] == kDilated)
      {
        thin_image_p[nid] = kVisited;
        queue.push(node_t(distance_p[nid], nid));
      }
    }
  }
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::ComputeThinImage()
{
  InputImagePointer inputImage = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
  OutputImagePointer thinImage = GetThinning();

  auto region = thinImage->GetRequestedRegion();

  ConstBoundaryConditionType boundaryCondition;
  boundaryCondition.SetConstant(0);

  typename NeighborhoodIteratorType::RadiusType radius;
  radius.Fill(1);
  NeighborhoodIteratorType ot(radius, thinImage, region);
  ot.SetBoundaryCondition(boundaryCondition);
  ImageRegionConstIterator<TInputImage> it(inputImage, region);

  using id_dist_t = std::pair<IndexType, float>;
  std::vector<id_dist_t> simpleBorderPoints;

  // Define offsets
  using OffsetType = typename NeighborhoodIteratorType::OffsetType;
  OffsetType N = { { 0, -1, 0 } }; // north
  OffsetType S = { { 0, 1, 0 } };  // south
  OffsetType E = { { 1, 0, 0 } };  // east
  OffsetType W = { { -1, 0, 0 } }; // west
  OffsetType U = { { 0, 0, 1 } };  // up
  OffsetType B = { { 0, 0, -1 } }; // bottom
  OffsetType borders[] = {N, S, E, W, U, B};

  // Prepare Euler LUT [Lee94]
  int eulerLUT[256];
  this->FillEulerLUT(eulerLUT);

  // Loop through the image several times until there is no change.
  int unchangedBorders = 0;
  while (unchangedBorders < 6) // loop until no change for all the six border types
  {
    unchangedBorders = 0;
    for (unsigned int currentBorder = 0; currentBorder < 6; ++currentBorder)
    {
      // Loop through the image.
      for (ot.GoToBegin(), it.GoToBegin(); !ot.IsAtEnd(); ++ot, ++it)
      {
        // Check if point is foreground in input
        if (it.Get())
        {
          continue;
        }
        // Check if point is foreground
        if (ot.GetCenterPixel() != 1)
        {
          continue; // current point is already background
        }
        // Check 6-neighbors if point is a border point of type currentBorder
        bool isBorderPoint = false;
        if (ot.GetPixel(borders[currentBorder]) <= 0)
        {
          isBorderPoint = true;
        }
        if (!isBorderPoint)
        {
          continue; // current point is not deletable
        }
        // Check if point is the end of an arc
        //int numberOfNeighbors = -1;           // -1 and not 0 because the center pixel will be counted as well
        //for (unsigned int i = 0; i < 27; ++i) // i =  0..26
        //{
        //  if (ot.GetPixel(i) == 1)
        //  {
        //    numberOfNeighbors++;
        //  }
        //}

        //if (numberOfNeighbors == 1)
        //{
        //  continue; // current point is not deletable
        //}

        // Check if point is Euler invariant
        if (!this->IsEulerInvariant(ot.GetNeighborhood(), eulerLUT))
        {
          continue; // current point is not deletable
        }

        // Check if point is simple (deletion does not change connectivity in the 3x3x3 neighborhood)
        if (!this->IsSimplePoint(ot.GetNeighborhood(), true))
        {
          continue; // current point is not deletable
        }

        if (!this->IsSimplePoint(ot.GetNeighborhood(), false))
        {
          continue; // current point is not deletable
        }

        // Add all simple border points to a list for sequential re-checking
        simpleBorderPoints.push_back(std::make_pair(ot.GetIndex(), m_DistanceMap->GetPixel(ot.GetIndex())));
      }

      // sort from largest to smallest (SDF has is negative inside object)
      std::sort(simpleBorderPoints.begin(), simpleBorderPoints.end(),
          [](const id_dist_t& a, const id_dist_t& b) { return a.second > b.second; });

std::cerr << "Distances: " << simpleBorderPoints.front().second << " >= " << simpleBorderPoints.back().second << "\n";

      // Sequential re-checking to preserve connectivity when
      // deleting in a parallel way
      bool noChange = true;
      for (const auto& pt: simpleBorderPoints)
      {
        // 1. Set simple border point to 0
        thinImage->SetPixel(pt.first, NumericTraits<OutputImagePixelType>::Zero);
        // 2. Check if neighborhood is still connected
        ot.SetLocation(pt.first);
        if (!this->IsSimplePoint(ot.GetNeighborhood(), true))
        {
          // We cannot delete current point, so reset
          thinImage->SetPixel(pt.first, NumericTraits<OutputImagePixelType>::One);
        }
        else
        {
          noChange = false;
        }
      }
      if (noChange)
      {
        unchangedBorders++;
      }
      simpleBorderPoints.clear();
    }
  }
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::GenerateData()
{
  this->PrepareData();

  this->ComputeThinImage();
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::FillEulerLUT(int * LUT)
{
  LUT[1] = 1;
  LUT[3] = -1;
  LUT[5] = -1;
  LUT[7] = 1;
  LUT[9] = -3;
  LUT[11] = -1;
  LUT[13] = -1;
  LUT[15] = 1;
  LUT[17] = -1;
  LUT[19] = 1;
  LUT[21] = 1;
  LUT[23] = -1;
  LUT[25] = 3;
  LUT[27] = 1;
  LUT[29] = 1;
  LUT[31] = -1;
  LUT[33] = -3;
  LUT[35] = -1;
  LUT[37] = 3;
  LUT[39] = 1;
  LUT[41] = 1;
  LUT[43] = -1;
  LUT[45] = 3;
  LUT[47] = 1;
  LUT[49] = -1;
  LUT[51] = 1;

  LUT[53] = 1;
  LUT[55] = -1;
  LUT[57] = 3;
  LUT[59] = 1;
  LUT[61] = 1;
  LUT[63] = -1;
  LUT[65] = -3;
  LUT[67] = 3;
  LUT[69] = -1;
  LUT[71] = 1;
  LUT[73] = 1;
  LUT[75] = 3;
  LUT[77] = -1;
  LUT[79] = 1;
  LUT[81] = -1;
  LUT[83] = 1;
  LUT[85] = 1;
  LUT[87] = -1;
  LUT[89] = 3;
  LUT[91] = 1;
  LUT[93] = 1;
  LUT[95] = -1;
  LUT[97] = 1;
  LUT[99] = 3;
  LUT[101] = 3;
  LUT[103] = 1;

  LUT[105] = 5;
  LUT[107] = 3;
  LUT[109] = 3;
  LUT[111] = 1;
  LUT[113] = -1;
  LUT[115] = 1;
  LUT[117] = 1;
  LUT[119] = -1;
  LUT[121] = 3;
  LUT[123] = 1;
  LUT[125] = 1;
  LUT[127] = -1;
  LUT[129] = -7;
  LUT[131] = -1;
  LUT[133] = -1;
  LUT[135] = 1;
  LUT[137] = -3;
  LUT[139] = -1;
  LUT[141] = -1;
  LUT[143] = 1;
  LUT[145] = -1;
  LUT[147] = 1;
  LUT[149] = 1;
  LUT[151] = -1;
  LUT[153] = 3;
  LUT[155] = 1;

  LUT[157] = 1;
  LUT[159] = -1;
  LUT[161] = -3;
  LUT[163] = -1;
  LUT[165] = 3;
  LUT[167] = 1;
  LUT[169] = 1;
  LUT[171] = -1;
  LUT[173] = 3;
  LUT[175] = 1;
  LUT[177] = -1;
  LUT[179] = 1;
  LUT[181] = 1;
  LUT[183] = -1;
  LUT[185] = 3;
  LUT[187] = 1;
  LUT[189] = 1;
  LUT[191] = -1;
  LUT[193] = -3;
  LUT[195] = 3;
  LUT[197] = -1;
  LUT[199] = 1;
  LUT[201] = 1;
  LUT[203] = 3;
  LUT[205] = -1;
  LUT[207] = 1;

  LUT[209] = -1;
  LUT[211] = 1;
  LUT[213] = 1;
  LUT[215] = -1;
  LUT[217] = 3;
  LUT[219] = 1;
  LUT[221] = 1;
  LUT[223] = -1;
  LUT[225] = 1;
  LUT[227] = 3;
  LUT[229] = 3;
  LUT[231] = 1;
  LUT[233] = 5;
  LUT[235] = 3;
  LUT[237] = 3;
  LUT[239] = 1;
  LUT[241] = -1;
  LUT[243] = 1;
  LUT[245] = 1;
  LUT[247] = -1;
  LUT[249] = 3;
  LUT[251] = 1;
  LUT[253] = 1;
  LUT[255] = -1;
}

template <class TInputImage, class TOutputImage>
bool
FixTopologyCarveOutside2<TInputImage, TOutputImage>::IsEulerInvariant(NeighborhoodType neighbors, const int * LUT)
{
  // Calculate Euler characteristic for each octant and sum up
  int           EulerChar = 0;
  unsigned char n;

  // Octant SWU
  n = 1;
  if (neighbors[24] == 1)
  {
    n |= 128;
  }
  if (neighbors[25] == 1)
  {
    n |= 64;
  }
  if (neighbors[15] == 1)
  {
    n |= 32;
  }
  if (neighbors[16] == 1)
  {
    n |= 16;
  }
  if (neighbors[21] == 1)
  {
    n |= 8;
  }
  if (neighbors[22] == 1)
  {
    n |= 4;
  }
  if (neighbors[12] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant SEU
  n = 1;
  if (neighbors[26] == 1)
  {
    n |= 128;
  }
  if (neighbors[23] == 1)
  {
    n |= 64;
  }
  if (neighbors[17] == 1)
  {
    n |= 32;
  }
  if (neighbors[14] == 1)
  {
    n |= 16;
  }
  if (neighbors[25] == 1)
  {
    n |= 8;
  }
  if (neighbors[22] == 1)
  {
    n |= 4;
  }
  if (neighbors[16] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant NWU
  n = 1;
  if (neighbors[18] == 1)
  {
    n |= 128;
  }
  if (neighbors[21] == 1)
  {
    n |= 64;
  }
  if (neighbors[9] == 1)
  {
    n |= 32;
  }
  if (neighbors[12] == 1)
  {
    n |= 16;
  }
  if (neighbors[19] == 1)
  {
    n |= 8;
  }
  if (neighbors[22] == 1)
  {
    n |= 4;
  }
  if (neighbors[10] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant NEU
  n = 1;
  if (neighbors[20] == 1)
  {
    n |= 128;
  }
  if (neighbors[23] == 1)
  {
    n |= 64;
  }
  if (neighbors[19] == 1)
  {
    n |= 32;
  }
  if (neighbors[22] == 1)
  {
    n |= 16;
  }
  if (neighbors[11] == 1)
  {
    n |= 8;
  }
  if (neighbors[14] == 1)
  {
    n |= 4;
  }
  if (neighbors[10] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant SWB
  n = 1;
  if (neighbors[6] == 1)
  {
    n |= 128;
  }
  if (neighbors[15] == 1)
  {
    n |= 64;
  }
  if (neighbors[7] == 1)
  {
    n |= 32;
  }
  if (neighbors[16] == 1)
  {
    n |= 16;
  }
  if (neighbors[3] == 1)
  {
    n |= 8;
  }
  if (neighbors[12] == 1)
  {
    n |= 4;
  }
  if (neighbors[4] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant SEB
  n = 1;
  if (neighbors[8] == 1)
  {
    n |= 128;
  }
  if (neighbors[7] == 1)
  {
    n |= 64;
  }
  if (neighbors[17] == 1)
  {
    n |= 32;
  }
  if (neighbors[16] == 1)
  {
    n |= 16;
  }
  if (neighbors[5] == 1)
  {
    n |= 8;
  }
  if (neighbors[4] == 1)
  {
    n |= 4;
  }
  if (neighbors[14] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant NWB
  n = 1;
  if (neighbors[0] == 1)
  {
    n |= 128;
  }
  if (neighbors[9] == 1)
  {
    n |= 64;
  }
  if (neighbors[3] == 1)
  {
    n |= 32;
  }
  if (neighbors[12] == 1)
  {
    n |= 16;
  }
  if (neighbors[1] == 1)
  {
    n |= 8;
  }
  if (neighbors[10] == 1)
  {
    n |= 4;
  }
  if (neighbors[4] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  // Octant NEB
  n = 1;
  if (neighbors[2] == 1)
  {
    n |= 128;
  }
  if (neighbors[1] == 1)
  {
    n |= 64;
  }
  if (neighbors[11] == 1)
  {
    n |= 32;
  }
  if (neighbors[10] == 1)
  {
    n |= 16;
  }
  if (neighbors[5] == 1)
  {
    n |= 8;
  }
  if (neighbors[4] == 1)
  {
    n |= 4;
  }
  if (neighbors[14] == 1)
  {
    n |= 2;
  }

  EulerChar += LUT[n];

  if (EulerChar == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

template <class TInputImage, class TOutputImage>
bool
FixTopologyCarveOutside2<TInputImage, TOutputImage>::IsSimplePoint(NeighborhoodType neighbors, bool FG)
{
  // Copy neighbors for labeling
  int cube[26];
  for (unsigned int i = 0; i < 13; ++i) // i =  0..12 -> cube[0..12]
  {
    cube[i] = FG ? neighbors[i] : neighbors[i] - 1;
  }
  // i != 13 : ignore center pixel when counting (see [Lee94])
  for (unsigned int i = 14; i < 27; ++i) // i = 14..26 -> cube[13..25]
  {
    cube[i - 1] = FG ? neighbors[i] : neighbors[i] - 1;
  }
  // Set initial label
  int label = 2;
  // Loop over all points in the neighborhood
  for (unsigned int i = 0; i < 26; ++i)
  {
    if (cube[i] == 1) // voxel has not been labelled yet
    {
      // Start recursion with any octant that contains the point i
      switch (i)
      {
        case 0:
        case 1:
        case 3:
        case 4:
        case 9:
        case 10:
        case 12:
          this->OctreeLabeling(1, label, cube);
          break;
        case 2:
        case 5:
        case 11:
        case 13:
          this->OctreeLabeling(2, label, cube);
          break;
        case 6:
        case 7:
        case 14:
        case 15:
          this->OctreeLabeling(3, label, cube);
          break;
        case 8:
        case 16:
          this->OctreeLabeling(4, label, cube);
          break;
        case 17:
        case 18:
        case 20:
        case 21:
          this->OctreeLabeling(5, label, cube);
          break;
        case 19:
        case 22:
          this->OctreeLabeling(6, label, cube);
          break;
        case 23:
        case 24:
          this->OctreeLabeling(7, label, cube);
          break;
        case 25:
          this->OctreeLabeling(8, label, cube);
          break;
      }
      label++;
      if (label - 2 >= 2)
      {
        return false;
      }
    }
  }
  // return label-2; in [Lee94] if the number of connected compontents would be needed
  return true;
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::OctreeLabeling(int octant, int label, int * cube)
{
  // Check if there are points in the octant with value 1
  if (octant == 1)
  {
    // Set points in this octant to current label
    // and recurseive labeling of adjacent octants
    if (cube[0] == 1)
    {
      cube[0] = label;
    }
    if (cube[1] == 1)
    {
      cube[1] = label;
      this->OctreeLabeling(2, label, cube);
    }
    if (cube[3] == 1)
    {
      cube[3] = label;
      this->OctreeLabeling(3, label, cube);
    }
    if (cube[4] == 1)
    {
      cube[4] = label;
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(4, label, cube);
    }
    if (cube[9] == 1)
    {
      cube[9] = label;
      this->OctreeLabeling(5, label, cube);
    }
    if (cube[10] == 1)
    {
      cube[10] = label;
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[12] == 1)
    {
      cube[12] = label;
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(7, label, cube);
    }
  }

  if (octant == 2)
  {
    if (cube[1] == 1)
    {
      cube[1] = label;
      this->OctreeLabeling(1, label, cube);
    }
    if (cube[4] == 1)
    {
      cube[4] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(4, label, cube);
    }
    if (cube[10] == 1)
    {
      cube[10] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[2] == 1)
    {
      cube[2] = label;
    }
    if (cube[5] == 1)
    {
      cube[5] = label;
      this->OctreeLabeling(4, label, cube);
    }
    if (cube[11] == 1)
    {
      cube[11] = label;
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[13] == 1)
    {
      cube[13] = label;
      this->OctreeLabeling(4, label, cube);
      this->OctreeLabeling(6, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
  }

  if (octant == 3)
  {
    if (cube[3] == 1)
    {
      cube[3] = label;
      this->OctreeLabeling(1, label, cube);
    }
    if (cube[4] == 1)
    {
      cube[4] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(4, label, cube);
    }
    if (cube[12] == 1)
    {
      cube[12] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[6] == 1)
    {
      cube[6] = label;
    }
    if (cube[7] == 1)
    {
      cube[7] = label;
      this->OctreeLabeling(4, label, cube);
    }
    if (cube[14] == 1)
    {
      cube[14] = label;
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[15] == 1)
    {
      cube[15] = label;
      this->OctreeLabeling(4, label, cube);
      this->OctreeLabeling(7, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
  }

  if (octant == 4)
  {
    if (cube[4] == 1)
    {
      cube[4] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(3, label, cube);
    }
    if (cube[5] == 1)
    {
      cube[5] = label;
      this->OctreeLabeling(2, label, cube);
    }
    if (cube[13] == 1)
    {
      cube[13] = label;
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(6, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
    if (cube[7] == 1)
    {
      cube[7] = label;
      this->OctreeLabeling(3, label, cube);
    }
    if (cube[15] == 1)
    {
      cube[15] = label;
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(7, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
    if (cube[8] == 1)
    {
      cube[8] = label;
    }
    if (cube[16] == 1)
    {
      cube[16] = label;
      this->OctreeLabeling(8, label, cube);
    }
  }

  if (octant == 5)
  {
    if (cube[9] == 1)
    {
      cube[9] = label;
      this->OctreeLabeling(1, label, cube);
    }
    if (cube[10] == 1)
    {
      cube[10] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[12] == 1)
    {
      cube[12] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[17] == 1)
    {
      cube[17] = label;
    }
    if (cube[18] == 1)
    {
      cube[18] = label;
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[20] == 1)
    {
      cube[20] = label;
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[21] == 1)
    {
      cube[21] = label;
      this->OctreeLabeling(6, label, cube);
      this->OctreeLabeling(7, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
  }

  if (octant == 6)
  {
    if (cube[10] == 1)
    {
      cube[10] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(5, label, cube);
    }
    if (cube[11] == 1)
    {
      cube[11] = label;
      this->OctreeLabeling(2, label, cube);
    }
    if (cube[13] == 1)
    {
      cube[13] = label;
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(4, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
    if (cube[18] == 1)
    {
      cube[18] = label;
      this->OctreeLabeling(5, label, cube);
    }
    if (cube[21] == 1)
    {
      cube[21] = label;
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(7, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
    if (cube[19] == 1)
    {
      cube[19] = label;
    }
    if (cube[22] == 1)
    {
      cube[22] = label;
      this->OctreeLabeling(8, label, cube);
    }
  }

  if (octant == 7)
  {
    if (cube[12] == 1)
    {
      cube[12] = label;
      this->OctreeLabeling(1, label, cube);
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(5, label, cube);
    }
    if (cube[14] == 1)
    {
      cube[14] = label;
      this->OctreeLabeling(3, label, cube);
    }
    if (cube[15] == 1)
    {
      cube[15] = label;
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(4, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
    if (cube[20] == 1)
    {
      cube[20] = label;
      this->OctreeLabeling(5, label, cube);
    }
    if (cube[21] == 1)
    {
      cube[21] = label;
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(6, label, cube);
      this->OctreeLabeling(8, label, cube);
    }
    if (cube[23] == 1)
    {
      cube[23] = label;
    }
    if (cube[24] == 1)
    {
      cube[24] = label;
      this->OctreeLabeling(8, label, cube);
    }
  }

  if (octant == 8)
  {
    if (cube[13] == 1)
    {
      cube[13] = label;
      this->OctreeLabeling(2, label, cube);
      this->OctreeLabeling(4, label, cube);
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[15] == 1)
    {
      cube[15] = label;
      this->OctreeLabeling(3, label, cube);
      this->OctreeLabeling(4, label, cube);
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[16] == 1)
    {
      cube[16] = label;
      this->OctreeLabeling(4, label, cube);
    }
    if (cube[21] == 1)
    {
      cube[21] = label;
      this->OctreeLabeling(5, label, cube);
      this->OctreeLabeling(6, label, cube);
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[22] == 1)
    {
      cube[22] = label;
      this->OctreeLabeling(6, label, cube);
    }
    if (cube[24] == 1)
    {
      cube[24] = label;
      this->OctreeLabeling(7, label, cube);
    }
    if (cube[25] == 1)
    {
      cube[25] = label;
    }
  }
}

template <class TInputImage, class TOutputImage>
void
FixTopologyCarveOutside2<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif // itkFixTopologyCarveOutside2_hxx
// clang-format on