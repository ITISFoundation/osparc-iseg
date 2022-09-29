/// \file itkFixTopologyCarveInside.h
///
/// \author Bryn Lloyd
/// \copyright 2020, IT'IS Foundation

#ifndef itkFixTopologyCarveOutside_h
#define itkFixTopologyCarveOutside_h

#include "itkImageToImageFilter.h"
#include "itkProgressAccumulator.h"

#include <vector>

namespace itk
{
/** \class FixTopologyCarveOutside
 *
 * \brief This filter does morphological closing with topology constraints
 *
 * It works by doing following steps:
 * 1. dilate the foreground
 * 2. erode/carve the dilated voxels from the "outside" while preserving the topology of the dilated region.
 *
 * The first step closes holes and the second returns as close as possible to the input mask, while ensuring that the
 * holes are not re-opened. Instead of using a dilation, a custom mask can be provided (e.g. a sphere around a hole).
 *
 * This filter implements ideas from: Vanderhyde, James. "Topology control of volumetric data.", PhD dissertation,
 * Georgia Institute of Technology, 2007..
 *
 * \author Bryn Lloyd
 * \ingroup TopologyControl
 */
template <class TInputImage, class TOutputImage, class TMaskImage = itk::Image<unsigned char, 3>>
class ITK_TEMPLATE_EXPORT FixTopologyCarveOutside : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Extract dimension from input and output image. */
  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

  static const unsigned int ImageDimension = InputImageDimension;

  /** Standard class typedefs. */
  using Self = FixTopologyCarveOutside;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(FixTopologyCarveOutside, ImageToImageFilter);

  /** Type for input image. */
  using InputImageType = TInputImage;

  /** Type for output image: Skeleton of the object.  */
  using OutputImageType = TOutputImage;

  /** Type for mask image  */
  using MaskImageType = TMaskImage;

  /** Type for the pixel type of the input image. */
  using InputImagePixelType = typename InputImageType::PixelType;

  /** Type for the pixel type of the input image. */
  using OutputImagePixelType = typename OutputImageType::PixelType;

  /** Pointer Type for input image. */
  using InputImagePointer = typename InputImageType::ConstPointer;

  /** Pointer Type for the output image. */
  using OutputImagePointer = typename OutputImageType::Pointer;

  /** Pointer Type for the mask image. */
  using MaskImageTypePointer = typename MaskImageType::Pointer;

  /** Optional mask (if none is provided the input mask is dilated by 'Radius') */
  void
  SetMaskImage(const TMaskImage * mask);
  const TMaskImage *
  GetMaskImage() const;

  itkSetMacro(Radius, SizeValueType);
  itkGetConstMacro(Radius, SizeValueType);

  itkSetMacro(InsideValue, InputImagePixelType);
  itkGetConstMacro(InsideValue, InputImagePixelType);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimensionCheck, (Concept::SameDimension<InputImageDimension, 3>));
  itkConceptMacro(SameTypeCheck, (Concept::SameType<InputImagePixelType, OutputImagePixelType>));
  itkConceptMacro(InputAdditiveOperatorsCheck, (Concept::AdditiveOperators<InputImagePixelType>));
  itkConceptMacro(InputConvertibleToIntCheck, (Concept::Convertible<InputImagePixelType, int>));
  itkConceptMacro(IntConvertibleToInputCheck, (Concept::Convertible<int, InputImagePixelType>));
  itkConceptMacro(InputIntComparableCheck, (Concept::Comparable<InputImagePixelType, int>));
  /** End concept checking */
#endif

protected:
  FixTopologyCarveOutside();
  ~FixTopologyCarveOutside() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  void
  GenerateData() override;

  void
  PrepareData(ProgressAccumulator * progress);

  void
  ComputeThinImage(ProgressAccumulator * progress);

  std::vector<typename InputImageType::OffsetType>
  GetNeighborOffsets()
  {
    // 18-connectivity
    using OffsetType = typename InputImageType::OffsetType;
    return { OffsetType{ -1, 0, 0 }, OffsetType{ 1, 0, 0 }, OffsetType{ 0, -1, 0 },  OffsetType{ 0, 1, 0 },
             OffsetType{ 0, 0, -1 }, OffsetType{ 0, 0, 1 }, OffsetType{ -1, -1, 0 }, OffsetType{ 1, -1, 0 },
             OffsetType{ -1, 1, 0 }, OffsetType{ 1, 1, 0 }, OffsetType{ 0, -1, -1 }, OffsetType{ 0, 1, -1 },
             OffsetType{ 0, -1, 1 }, OffsetType{ 0, 1, 1 }, OffsetType{ -1, 0, -1 }, OffsetType{ 1, 0, -1 },
             OffsetType{ -1, 0, 1 }, OffsetType{ 1, 0, 1 } };
  }

private:
  ITK_DISALLOW_COPY_AND_ASSIGN(FixTopologyCarveOutside);

  enum ePixelState : OutputImagePixelType
  {
    kBackground = 0,
    kHardForeground,
    kDilated,
    kVisited
  };

  MaskImageTypePointer m_PaddedOutput;

  using RealImageType = itk::Image<float, 3>;
  RealImageType::Pointer m_DistanceMap;

  SizeValueType       m_Radius = 1;
  InputImagePixelType m_InsideValue = 1;
}; // end of FixTopologyCarveOutside class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkFixTopologyCarveOutside.hxx"
#endif

#endif // itkFixTopologyCarveOutside
