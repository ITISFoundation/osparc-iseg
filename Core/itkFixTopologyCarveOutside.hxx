#ifndef itkFixTopologyCarveOutside_hxx
#define itkFixTopologyCarveOutside_hxx

#include "itkFixTopologyCarveOutside.h"
#include "TopologyInvariants.h"

#include "itkBinaryDilateImageFilter.h"
#include "itkConstantBoundaryCondition.h"
#include "itkFlatStructuringElement.h"
#include "itkImageLinearConstIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionRange.h"
#include "itkNeighborhoodIterator.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"

namespace itk
{

template <class TInputImage, class TOutputImage, class TMaskImage>
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::FixTopologyCarveOutside()
{
  this->SetNumberOfRequiredOutputs(1);
}

template <class TInputImage, class TOutputImage, class TMaskImage>
void
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::SetMaskImage(const TMaskImage * mask)
{
  this->ProcessObject::SetNthInput(1, const_cast<TMaskImage *>(mask));
}

template <class TInputImage, class TOutputImage, class TMaskImage>
const TMaskImage *
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::GetMaskImage() const
{
  return itkDynamicCastInDebugMode<MaskImageType *>(const_cast<DataObject *>(this->ProcessObject::GetInput(1)));
}

template <class TInputImage, class TOutputImage, class TMaskImage>
void
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::PrepareData(ProgressAccumulator * progress)
{
  InputImagePointer  input_image = dynamic_cast<const TInputImage *>(ProcessObject::GetInput(0));
  OutputImagePointer thin_image = this->GetOutput();
  thin_image->SetBufferedRegion(thin_image->GetRequestedRegion());
  thin_image->Allocate();

  // pad by 1 layer so we can get 1x1x1 neighborhood without checking if we are at boundary
  auto region = thin_image->GetRequestedRegion();
  auto padded_region = region;
  padded_region.PadByRadius(1);

  m_PaddedOutput = MaskImageType::New();
  m_PaddedOutput->SetRegions(padded_region);
  m_PaddedOutput->SetSpacing(thin_image->GetSpacing());
  m_PaddedOutput->Allocate();
  m_PaddedOutput->FillBuffer(ePixelState::kBackground);

  ImageRegionConstIterator<TInputImage> it(input_image, region);
  ImageRegionIterator<MaskImageType>    ot(m_PaddedOutput, region);

  // mark the input mask as kHardForeground
  for (it.GoToBegin(), ot.GoToBegin(); !ot.IsAtEnd(); ++it, ++ot)
  {
    if (it.Get() == m_InsideValue)
    {
      ot.Set(ePixelState::kHardForeground);
    }
  }

  // compute distance map: used for priority queue
  auto distance_filter = SignedMaurerDistanceMapImageFilter<MaskImageType, RealImageType>::New();
  progress->RegisterInternalFilter(distance_filter, 0.1);
  distance_filter->SetInput(m_PaddedOutput);
  distance_filter->SetUseImageSpacing(true);
  distance_filter->SetInsideIsPositive(false);
  distance_filter->SetSquaredDistance(false);
  distance_filter->SetBackgroundValue(0);
  distance_filter->Update();
  m_DistanceMap = distance_filter->GetOutput();

  typename MaskImageType::ConstPointer mask_image = GetMaskImage();
  if (!mask_image)
  {
    // if no mask is provided we dilate the input mask
    using kernel_type = itk::FlatStructuringElement<3>;
    auto ball = kernel_type::Ball({ m_Radius, m_Radius, m_Radius }, false);

    auto dilate = itk::BinaryDilateImageFilter<MaskImageType, MaskImageType, kernel_type>::New();
    progress->RegisterInternalFilter(dilate, 0.1);
    dilate->SetInput(m_PaddedOutput);
    dilate->SetKernel(ball);
    dilate->SetForegroundValue(ePixelState::kHardForeground);
    dilate->Update();
    mask_image = dilate->GetOutput();
  }

  // mark dilated as '2', but don't copy padding layer
  ImageRegionConstIterator<MaskImageType> mt(mask_image, region);
  for (mt.GoToBegin(), ot.GoToBegin(); !ot.IsAtEnd(); ++mt, ++ot)
  {
    if (mt.Get() != ot.Get())
    {
      ot.Set(ePixelState::kDilated);
    }
  }
}

template <class TInputImage, class TOutputImage, class TMaskImage>
void
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::ComputeThinImage(ProgressAccumulator * accumulator)
{
  using IndexType = typename InputImageType::IndexType;
  using NeighborhoodIteratorType = NeighborhoodIterator<TMaskImage, ConstantBoundaryCondition<TMaskImage>>;

  OutputImagePointer thin_image = this->GetOutput();
  // note output image not padded, all processing is done on m_PaddedOutput, i.e. padded image
  auto region = thin_image->GetRequestedRegion();

  std::vector<IndexType> seeds;
  for (int direction = 0; direction < ImageDimension; ++direction)
  {
    ImageLinearConstIteratorWithIndex<MaskImageType> it(m_PaddedOutput, region);
    it.SetDirection(direction);
    it.GoToBegin();
    for (; !it.IsAtEnd(); it.NextLine())
    {
      auto last_value = it.Get();
      auto last_idx = it.GetIndex();

      if (last_value) // mask at boundary
      {
        seeds.push_back({ last_idx });
      }

      for (; !it.IsAtEndOfLine(); ++it)
      {
        if (it.Get() != last_value)
        {
          if (last_value) // leaving mask
          {
            seeds.push_back({ last_idx });
          }
          else // entering mask
          {
            seeds.push_back({ it.GetIndex() });
          }
          last_value = it.Get();
        }
        last_idx = it.GetIndex();
      }

      if (last_value && last_idx != it.GetIndex()) // mask at boundary
      {
        seeds.push_back({ it.GetIndex() });
      }
    }
  }

  // for progress reporting
  size_t                                     mask_size = 0;
  itk::ImageRegionRange<const MaskImageType> image_range(*m_PaddedOutput, region);
  for (auto && pixel : image_range)
  {
    mask_size += (pixel == kDilated) ? 1 : 0;
  }

  // use max priority queue: process pixels further away from input mask first
  using node = std::pair<float, IndexType>;
  const auto cmp = [](const node & l, const node & r) { return l.first < r.first; };
  std::priority_queue<node, std::vector<node>, decltype(cmp)> queue(cmp);
  for (const auto & idx : seeds)
  {
    m_PaddedOutput->SetPixel(idx, kVisited);
    queue.push(std::make_pair(m_DistanceMap->GetPixel(idx), idx));
  }

  auto         neighbors = this->GetNeighborOffsets();
  const size_t num_neighbors = neighbors.size();

  NeighborhoodIteratorType n_it({ 1, 1, 1 }, m_PaddedOutput, region);

  const auto get_mask = [&n_it](const IndexType & idx, const int FG) {
    n_it.SetLocation(idx);
    auto n = n_it.GetNeighborhood();
    for (auto & v : n.GetBufferReference())
      v = (v != 0) ? FG : 1 - FG;
    return n;
  };

  // erode while topology does not change
  ProgressReporter progress(this, 0, mask_size, 100);
  while (!queue.empty())
  {
    auto idx = queue.top().second; // node
    queue.pop();

    if (m_PaddedOutput->GetPixel(idx) != kVisited)
      continue;

    auto vals = get_mask(idx, 1);

    // check if point is simple (deletion does not change connectivity in the 3x3x3 neighborhood)
    if (topology::EulerInvariant(vals, 1) && topology::CCInvariant(vals, 1) && topology::CCInvariant(vals, 0))
    {
      m_PaddedOutput->SetPixel(idx, kBackground);
    }

    // add unvisited neighbors to queue
    for (size_t k = 0; k < num_neighbors; ++k)
    {
      const IndexType n_id = idx + neighbors[k];
      const float     n_dist = m_DistanceMap->GetPixel(n_id);

      if (m_PaddedOutput->GetPixel(n_id) == kDilated)
      {
        // mark as visited
        m_PaddedOutput->SetPixel(n_id, kVisited);

        // add to queue
        queue.push(std::make_pair(n_dist, n_id));
      }
    }
    progress.CompletedPixel();
  }

  // copy to output
  InputImagePointer input_image = dynamic_cast<const TInputImage *>(ProcessObject::GetInput(0));
  itk::ImageRegionConstIterator<TInputImage> i_it(input_image, region);
  itk::ImageRegionConstIterator<TMaskImage>  s_it(m_PaddedOutput, region);
  itk::ImageRegionIterator<TOutputImage>     o_it(thin_image, region);

  for (i_it.GoToBegin(), s_it.GoToBegin(), o_it.GoToBegin(); !s_it.IsAtEnd(); ++i_it, ++s_it, ++o_it)
  {
    o_it.Set(s_it.Get() != ePixelState::kBackground ? m_InsideValue : i_it.Get());
  }
}

template <class TInputImage, class TOutputImage, class TMaskImage>
void
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::GenerateData()
{
  ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
  progress->SetMiniPipelineFilter(this);

  this->PrepareData(progress);

  this->ComputeThinImage(progress);
}

template <class TInputImage, class TOutputImage, class TMaskImage>
void
FixTopologyCarveOutside<TInputImage, TOutputImage, TMaskImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif // itkFixTopologyCarveOutside_hxx
