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
// \note Code modified from itkVotingBinaryImageFilter.h/hxx

#ifndef itkLabelVotingBinaryImageFilter_h
#define itkLabelVotingBinaryImageFilter_h

#include "itkArray.h"
#include "itkConstantBoundaryCondition.h"
#include "itkImageToImageFilter.h"

namespace itk
{
/** \class LabelVotingBinaryImageFilter
 * \brief Applies a voting operation in a neighborhood of each pixel.
 *
 * \note Pixels which are not Foreground or Background will remain unchanged.
 *
 * \sa Image
 * \sa Neighborhood
 * \sa NeighborhoodOperator
 * \sa NeighborhoodIterator
 *
 * \ingroup IntensityImageFilters
 * \ingroup ITKLabelVoting
 */
template< typename TInputImage, typename TOutputImage >
class ITK_TEMPLATE_EXPORT LabelVotingBinaryImageFilter:
  public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
  /** Extract dimension from input and output image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  /** Convenient typedefs for simplifying declarations. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;

  /** Standard class typedefs. */
  typedef LabelVotingBinaryImageFilter                               Self;
  typedef ImageToImageFilter< InputImageType, OutputImageType > Superclass;
  typedef SmartPointer< Self >                                  Pointer;
  typedef SmartPointer< const Self >                            ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(LabelVotingBinaryImageFilter, ImageToImageFilter);

  /** Image typedef support. */
  typedef typename InputImageType::PixelType  InputPixelType;
  typedef typename OutputImageType::PixelType OutputPixelType;

  typedef typename InputImageType::RegionType  InputImageRegionType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;

  typedef typename InputImageType::SizeType InputSizeType;

  /** Set the radius of the neighborhood used to compute the median. */
  itkSetMacro(Radius, InputSizeType);

  /** Get the radius of the neighborhood used to compute the median */
  itkGetConstReferenceMacro(Radius, InputSizeType);

  /** Set the value associated with the Foreground (or the object) on
      the binary input image and the Background . */
  itkSetMacro(BackgroundValue, InputPixelType);
  itkSetMacro(ForegroundValue, InputPixelType);

  /** Get the value associated with the Foreground (or the object) on the
      binary input image and the Background . */
  itkGetConstReferenceMacro(BackgroundValue, InputPixelType);
  itkGetConstReferenceMacro(ForegroundValue, InputPixelType);

  /** Threshold above which a pixel can be assigned to max. voted neighbor */
  itkGetConstReferenceMacro(MajorityThreshold, unsigned int);
  itkSetMacro(MajorityThreshold, unsigned int);

  /** Threshold for minimum number of neighbors (of specific type) required for assignment */
  itkGetConstReferenceMacro(VotingThreshold, unsigned int);
  itkSetMacro(VotingThreshold, unsigned int);

  /** Returns the number of pixels that changed when the filter was executed. */
  itkGetConstReferenceMacro(NumberOfPixelsChanged, SizeValueType);

  /** LabelVotingBinaryImageFilter needs a larger input requested region than
   * the output requested region.  As such, LabelVotingBinaryImageFilter needs
   * to provide an implementation for GenerateInputRequestedRegion()
   * in order to inform the pipeline execution model.
   *
   * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
  virtual void GenerateInputRequestedRegion() override;

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( InputEqualityComparableCheck,
                   ( Concept::EqualityComparable< InputPixelType > ) );
  itkConceptMacro( IntConvertibleToInputCheck,
                   ( Concept::Convertible< int, InputPixelType > ) );
  itkConceptMacro( InputConvertibleToOutputCheck,
                   ( Concept::Convertible< InputPixelType, OutputPixelType > ) );
  itkConceptMacro( SameDimensionCheck,
                   ( Concept::SameDimension< InputImageDimension, OutputImageDimension > ) );
  itkConceptMacro( InputOStreamWritableCheck,
                   ( Concept::OStreamWritable< InputPixelType > ) );
  // End concept checking
#endif

protected:
  LabelVotingBinaryImageFilter();
  virtual ~LabelVotingBinaryImageFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const override;

	/** LabelVotingBinaryImageFilter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData()
   * routine which is called for each processing thread. The output
   * image data is allocated automatically by the superclass prior to
   * calling ThreadedGenerateData().  ThreadedGenerateData can only
   * write to the portion of the output image specified by the
   * parameter "outputRegionForThread"
   *
   * \sa ImageToImageFilter::DynamicThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData() */
	void DynamicThreadedGenerateData(const OutputImageRegionType& outputRegionForThread) override;

	/** Methods to be called before and after the invokation of
  * ThreadedGenerateData(). */
  void BeforeThreadedGenerateData() override;

  void AfterThreadedGenerateData() override;

private:
  ITK_DISALLOW_COPY_AND_ASSIGN(LabelVotingBinaryImageFilter);

  InputSizeType m_Radius;

  InputPixelType m_ForegroundValue;
  InputPixelType m_BackgroundValue;

  unsigned int m_MajorityThreshold = 1;
  unsigned int m_VotingThreshold = 1;

  SizeValueType m_NumberOfPixelsChanged = 0;

  // Auxiliary array for multi-threading
	std::atomic<SizeValueType> m_Count;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkLabelVotingBinaryImageFilter.hxx"
#endif

#endif
