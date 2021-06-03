#ifndef __itkNonMaxSuppressionImageFilter_h
#define __itkNonMaxSuppressionImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "itkProgressReporter.h"
namespace itk
{
/**
 * \class NonMaxSuppressionImageFilter
 * \brief Suppress voxels that aren't a maxima. For thinning edge
 * images
 * This implementation is line based, rather than kernels. It is
 * multithreaded.
 * Only checks neighbouring voxels, so don't smooth too much.
 * Also, doesn't do any thresholding internally, so do this yourself
 * first.

 * \author Richard Beare, Department of Medicine, Monash University,
 * Australia.  <Richard.Beare@monash.edu>
 *
**/
template <typename TInputImage, typename TOutputImage = TInputImage>
class ITK_EXPORT NonMaxSuppressionImageFilter : public ImageToImageFilter<TInputImage, TOutputImage>
{

public:
  /** Standard class type aliases. */
  using Self = NonMaxSuppressionImageFilter;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(NonMaxSuppressionImageFilter, ImageToImageFilter);

  /** Pixel Type of the input image */
  using InputImageType = TInputImage;
  using OutputImageType = TOutputImage;
  using PixelType = typename TInputImage::PixelType;
  using OutputPixelType = typename TOutputImage::PixelType;

  /** Smart pointer type support.  */
  using InputImagePointer = typename TInputImage::Pointer;
  using InputImageConstPointer = typename TInputImage::ConstPointer;
  using InputSizeType = typename TInputImage::SizeType;
  using OutputSizeType = typename TOutputImage::SizeType;

  using OutputIndexType = typename OutputImageType::IndexType;

  itkSetMacro(EdgeValue, OutputPixelType);
  itkGetConstReferenceMacro(EdgeValue, OutputPixelType);

  // pass image values through - overrids edgevalue
  itkSetMacro(UseImageValue, bool);
  itkGetConstReferenceMacro(UseImageValue, bool);

  // how many directions does it need to be maximal in?
  // default = dimensions
  itkSetMacro(ReqMax, OutputPixelType);
  itkGetConstReferenceMacro(ReqMax, OutputPixelType);

  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int, TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
  using OutputImageRegionType = typename OutputImageType::RegionType;
#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimension,
                  (Concept::SameDimension<itkGetStaticConstMacro(InputImageDimension),
                                          itkGetStaticConstMacro(OutputImageDimension)>));

  itkConceptMacro(Comparable, (Concept::Comparable<PixelType>));

  /** End concept checking */
#endif

protected:
  NonMaxSuppressionImageFilter();
  ~NonMaxSuppressionImageFilter() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  void
  GenerateData() override;
  unsigned int
  SplitRequestedRegion(unsigned int i, unsigned int num, OutputImageRegionType & splitRegion) override;
  void
  ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId) override;
  void
  GenerateInputRequestedRegion() override;
  void
  EnlargeOutputRequestedRegion(DataObject * output) override;

private:
  NonMaxSuppressionImageFilter(const Self &) = delete;
  void
                  operator=(const Self &) = delete;
  OutputPixelType m_EdgeValue;
  OutputPixelType m_ReqMax;
  int             m_CurrentDimension;
  bool            m_UseImageValue;
};


} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkNonMaxSuppressionImageFilter.hxx"
#endif


#endif
