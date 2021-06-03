#ifndef __itkNonMaxSuppressionImageFilter_txx
#define __itkNonMaxSuppressionImageFilter_txx

#include "itkNonMaxSuppressionImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"

#include "itkImageLinearIteratorWithIndex.h"
#include "itkImageLinearConstIteratorWithIndex.h"

namespace itk
{

template <typename TInputImage, typename TOutputImage>
NonMaxSuppressionImageFilter<TInputImage, TOutputImage>
::NonMaxSuppressionImageFilter()
{
  this->SetNumberOfRequiredOutputs( 1 );
  this->SetNumberOfRequiredInputs( 1 );
// needs to be selected according to erosion/dilation

  m_EdgeValue = 1;
  m_ReqMax = ImageDimension;
  m_UseImageValue = false;
}

template <typename TInputImage, typename TOutputImage>
unsigned int
NonMaxSuppressionImageFilter<TInputImage, TOutputImage>
::SplitRequestedRegion(unsigned int i, unsigned int num,
  OutputImageRegionType & splitRegion)
{
  // Get the output pointer
  OutputImageType *outputPtr = this->GetOutput();

  // Initialize the splitRegion to the output requested region
  splitRegion = outputPtr->GetRequestedRegion();

  const OutputSizeType & requestedRegionSize = splitRegion.GetSize();

  OutputIndexType splitIndex = splitRegion.GetIndex();
  OutputSizeType  splitSize  = splitRegion.GetSize();

  // split on the outermost dimension available
  // and avoid the current dimension
  int splitAxis = static_cast< int >( outputPtr->GetImageDimension() ) - 1;
  while ( ( requestedRegionSize[splitAxis] == 1 ) ||
          ( splitAxis == static_cast< int >( m_CurrentDimension ) ) )
    {
    --splitAxis;
    if ( splitAxis < 0 )
      { // cannot split
      itkDebugMacro("Cannot Split");
      return 1;
      }
    }

  // determine the actual number of pieces that will be generated
  double range = static_cast< double >( requestedRegionSize[splitAxis] );

  unsigned int valuesPerThread =
    static_cast< unsigned int >( std::ceil( range / static_cast< double >( num ) ) );
  unsigned int maxThreadIdUsed =
    static_cast< unsigned int >( std::ceil( range / static_cast< double >( valuesPerThread ) ) ) - 1;

  // Split the region
  if ( i < maxThreadIdUsed )
    {
    splitIndex[splitAxis] += i * valuesPerThread;
    splitSize[splitAxis] = valuesPerThread;
    }
  if ( i == maxThreadIdUsed )
    {
    splitIndex[splitAxis] += i * valuesPerThread;
    // last thread needs to process the "rest" dimension being split
    splitSize[splitAxis] = splitSize[splitAxis] - i * valuesPerThread;
    }

  // set the split region ivars
  splitRegion.SetIndex(splitIndex);
  splitRegion.SetSize(splitSize);

  itkDebugMacro("Split Piece: " << splitRegion);

  return maxThreadIdUsed + 1;
}



template <typename TInputImage, typename TOutputImage>
void
NonMaxSuppressionImageFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method. this should
  // copy the output requested region to the input requested region
  Superclass::GenerateInputRequestedRegion();

  // This filter needs all of the input
  InputImagePointer image = const_cast<InputImageType *>( this->GetInput() );
  if( image )
    {
    image->SetRequestedRegion( this->GetInput()->GetLargestPossibleRegion() );
    }
}
template <typename TInputImage, typename TOutputImage>
void
NonMaxSuppressionImageFilter<TInputImage, TOutputImage>
::EnlargeOutputRequestedRegion(DataObject *output)
{
  TOutputImage *out = dynamic_cast<TOutputImage*>(output);

  if (out)
    {
    out->SetRequestedRegion( out->GetLargestPossibleRegion() );
    }
}

template <typename TInputImage, typename TOutputImage >
void
NonMaxSuppressionImageFilter<TInputImage, TOutputImage >
::GenerateData()
{
	auto nbthreads = this->GetNumberOfWorkUnits();

	typename TInputImage::ConstPointer   inputImage(    this->GetInput ()   );
  typename TOutputImage::Pointer       outputImage(   this->GetOutput()        );

  //const unsigned int imageDimension = inputImage->GetImageDimension();
  outputImage->SetBufferedRegion( outputImage->GetRequestedRegion() );
  outputImage->Allocate();
  outputImage->FillBuffer(0);
  // Set up the multithreaded processing
  typename ImageSource< OutputImageType >::ThreadStruct str;
  str.Filter = this;

  auto multithreader = this->GetMultiThreader();
	multithreader->SetNumberOfWorkUnits(nbthreads);
	multithreader->SetSingleMethod(this->ThreaderCallback, &str);

  // multithread the execution
  for( unsigned int d=0; d<ImageDimension; d++ )
    {
    m_CurrentDimension = d;
    multithreader->SingleMethodExecute();
    }

}

template <typename TInputImage, typename TOutputImage>
void
NonMaxSuppressionImageFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, ThreadIdType threadId )
{
  // compute the number of rows first, so we can setup a progress reporter
  typename std::vector< unsigned int > NumberOfRows;
  InputSizeType   size   = outputRegionForThread.GetSize();

  for (unsigned int i = 0; i < InputImageDimension; i++)
    {
    NumberOfRows.push_back( 1 );
    for (unsigned int d = 0; d < InputImageDimension; d++)
      {
      if( d != i )
        {
        NumberOfRows[i] *= size[ d ];
        }
      }
    }
  float progressPerDimension = 1.0/ImageDimension;

  ProgressReporter * progress = new ProgressReporter(this, threadId, NumberOfRows[m_CurrentDimension], 30, m_CurrentDimension * progressPerDimension, progressPerDimension);


  using InputConstIteratorType = ImageLinearConstIteratorWithIndex< TInputImage  >  ;
  using OutputIteratorType = ImageLinearIteratorWithIndex<TOutputImage>;

  // for stages after the first
  using OutputConstIteratorType = ImageLinearConstIteratorWithIndex<TOutputImage>;


  using RegionType = ImageRegion<TInputImage::ImageDimension>;

  typename TInputImage::ConstPointer   inputImage(    this->GetInput ()   );
  typename TOutputImage::Pointer       outputImage(   this->GetOutput()        );

  
  RegionType region = outputRegionForThread;

  //InputConstIteratorType  inputIterator(  inputImage,  region );
  InputConstIteratorType  inputIteratorNext(  inputImage,  region );
  OutputIteratorType      outputIterator( outputImage, region );
  OutputConstIteratorType inputIteratorStage2( outputImage, region );
  //inputIterator.SetDirection(m_CurrentDimension);
  inputIteratorNext.SetDirection(m_CurrentDimension);
  outputIterator.SetDirection(m_CurrentDimension);
  //inputIterator.GoToBegin();
  inputIteratorNext.GoToBegin();
  outputIterator.GoToBegin();


  if ( m_CurrentDimension != (InputImageDimension - 1) )
    {
    while( !inputIteratorNext.IsAtEnd() && !outputIterator.IsAtEnd() )
      {
      PixelType prev, centre, next;
      prev = itk::NumericTraits<PixelType>::max();
      next = itk::NumericTraits<PixelType>::max();
      centre = itk::NumericTraits<PixelType>::max();
      ++inputIteratorNext;
      for (; !inputIteratorNext.IsAtEndOfLine(); ++inputIteratorNext, ++outputIterator )
	{
	next = inputIteratorNext.Get();
	if ((centre > prev) && (centre > next))
	  {
	  outputIterator.Set(outputIterator.Get() + 1);
	  }
	prev=centre;
	centre=next;
	}

      // now onto the next line
      // inputIterator.NextLine();
      inputIteratorNext.NextLine();
      outputIterator.NextLine();
      inputIteratorNext.GoToBeginOfLine();
      outputIterator.GoToBeginOfLine();

      progress->CompletedPixel();
      }
    }
  else
    {
    // special case for the last dimension, as we need to set the output
    // to the specified value
    if (m_UseImageValue)
      {
      while( !inputIteratorNext.IsAtEnd() && !outputIterator.IsAtEnd() )
	{
	PixelType prev, centre, next;
	prev = itk::NumericTraits<PixelType>::max();
	next = itk::NumericTraits<PixelType>::max();
	centre = itk::NumericTraits<PixelType>::max();
	++inputIteratorNext;
	for (; !inputIteratorNext.IsAtEndOfLine(); ++inputIteratorNext, ++outputIterator )
	  {
	  next = inputIteratorNext.Get();
	  if ((centre > prev) && (centre > next))
	    {
	    OutputPixelType val = outputIterator.Get() + 1;
	    if (val >= m_ReqMax)
	      {
	      outputIterator.Set(centre);
	      }
	    else 
	      {
	      outputIterator.Set(0);
	      }
	    }
	  prev=centre;
	  centre=next;
	  }

	// now onto the next line
	//inputIterator.NextLine();
	inputIteratorNext.NextLine();
	outputIterator.NextLine();
	inputIteratorNext.GoToBeginOfLine();
	outputIterator.GoToBeginOfLine();
	progress->CompletedPixel();
	}
      }
    else
      {
      while( !inputIteratorNext.IsAtEnd() && !outputIterator.IsAtEnd() )
	{
	PixelType prev, centre, next;
	prev = itk::NumericTraits<PixelType>::max();
	next = itk::NumericTraits<PixelType>::max();
	centre = itk::NumericTraits<PixelType>::max();
	++inputIteratorNext;
	for (; !inputIteratorNext.IsAtEndOfLine(); ++inputIteratorNext, ++outputIterator )
	  {
	  next = inputIteratorNext.Get();
	  if ((centre > prev) && (centre > next))
	    {
	    OutputPixelType val = outputIterator.Get() + 1;
	    if (val >= m_ReqMax)
	      {
	      outputIterator.Set(m_EdgeValue);
	      }
	    else 
	      {
	      outputIterator.Set(0);
	      }
	    }
	  prev=centre;
	  centre=next;
	  }

	// now onto the next line
	inputIteratorNext.NextLine();
	outputIterator.NextLine();
	inputIteratorNext.GoToBeginOfLine();
	outputIterator.GoToBeginOfLine();
	progress->CompletedPixel();
	}
      }

    }

}


template <typename TInputImage, typename TOutputImage>
void
NonMaxSuppressionImageFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
  os << "Edge value: " << m_EdgeValue << std::endl;
  os << "MaxReq: " << m_ReqMax << std::endl;
}


} // namespace itk
#endif
