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
template <typename TInputImage,
          typename TOutputImage= TInputImage >
class ITK_EXPORT NonMaxSuppressionImageFilter:
    public ImageToImageFilter<TInputImage,TOutputImage>
{

public:
  /** Standard class typedefs. */
  typedef NonMaxSuppressionImageFilter  Self;
  typedef ImageToImageFilter<TInputImage,TOutputImage> Superclass;
  typedef SmartPointer<Self>                   Pointer;
  typedef SmartPointer<const Self>        ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(NonMaxSuppressionImageFilter, ImageToImageFilter);

  /** Pixel Type of the input image */
  typedef TInputImage                                    InputImageType;
  typedef TOutputImage                                   OutputImageType;
  typedef typename TInputImage::PixelType                PixelType;
  typedef typename TOutputImage::PixelType  OutputPixelType;

  /** Smart pointer typedef support.  */
  typedef typename TInputImage::Pointer  InputImagePointer;
  typedef typename TInputImage::ConstPointer  InputImageConstPointer;
  typedef typename TInputImage::SizeType    InputSizeType;
  typedef typename TOutputImage::SizeType   OutputSizeType;

  typedef typename OutputImageType::IndexType       OutputIndexType;

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
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  typedef typename OutputImageType::RegionType OutputImageRegionType;
#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimension,
                  (Concept::SameDimension<itkGetStaticConstMacro(InputImageDimension),itkGetStaticConstMacro(OutputImageDimension)>));

  itkConceptMacro(Comparable,
                  (Concept::Comparable<PixelType>));

  /** End concept checking */
#endif

protected:
  NonMaxSuppressionImageFilter();
  virtual ~NonMaxSuppressionImageFilter() {};
  void PrintSelf(std::ostream& os, Indent indent) const;
  
  /** Generate Data */
  void GenerateData( void );
  unsigned int SplitRequestedRegion(unsigned int i, unsigned int num,
    OutputImageRegionType & splitRegion);

  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, ThreadIdType threadId );

  void GenerateInputRequestedRegion() throw(InvalidRequestedRegionError);
  // Override since the filter produces the entire dataset.
  void EnlargeOutputRequestedRegion(DataObject *output);


private:
  NonMaxSuppressionImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented
  OutputPixelType m_EdgeValue;
  OutputPixelType m_ReqMax;
  int m_CurrentDimension;
  bool m_UseImageValue;
};


} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNonMaxSuppressionImageFilter.hxx"
#endif


#endif
