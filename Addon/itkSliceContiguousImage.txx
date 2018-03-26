/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkSliceContiguousImage.txx,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkSliceContiguousImage_txx
#define _itkSliceContiguousImage_txx

#include "itkSliceContiguousImage.h"
#include "itkProcessObject.h"

namespace itk
{

template<class TPixel>
SliceContiguousImage<TPixel>
::SliceContiguousImage()
{
  m_Container = PixelContainer::New();
}


template<class TPixel>
void
SliceContiguousImage<TPixel>
::Allocate()
{
  ComputeOffsetTable();
  SizeType size = GetLargestPossibleRegion().GetSize();
  m_Container->Reserve( size[2], size[0]*size[1] );
}


template<class TPixel>
void 
SliceContiguousImage<TPixel>
::Initialize()
{
  //
  // We don't modify ourselves because the "ReleaseData" methods depend upon
  // no modification when initialized.
  //

  // Call the superclass which should initialize the BufferedRegion ivar.
  Superclass::Initialize();

  // Replace the handle to the container. This is the safest thing to do,
  // since the same container can be shared by multiple images (e.g.
  // Grafted outputs and in place filters).
  m_Container = PixelContainer::New();
}


template<class TPixel>
void 
SliceContiguousImage<TPixel>
::FillBuffer(const PixelType& value)
{
  typename PixelContainer::SliceArrayType *slices = m_Container->GetSlices();
  SizeType size = this->GetLargestPossibleRegion().GetSize();
  unsigned long sizeOfSlice = size[0] * size[1];
  for ( unsigned int i=0; i<size[2]; i++ )
    {
    for ( unsigned int j=0; j<sizeOfSlice; j++ )
      {
      slices->operator[](i)[j] = value;
      }
    }
}


template<class TPixel>
void
SliceContiguousImage<TPixel>
::SetPixelContainer(PixelContainer *container)
{
   if (m_Container != container)
     {
     m_Container = container;
     this->Modified();
     }
}


template<class TPixel>
void 
SliceContiguousImage<TPixel>
::Graft(const DataObject *data)
{
  // call the superclass' implementation
  Superclass::Graft( data );

  if ( data )
    {
    // Attempt to cast data to an Image
    const Self *imgData;

    try
      {
      imgData = dynamic_cast< const Self *>( data );
      }
    catch( ... )
      {
      return;
      }

    // Copy from SliceContiguousImage< TPixel, dim >
    if ( imgData )
      {
      // Now copy anything remaining that is needed
      this->SetPixelContainer( const_cast< PixelContainer *>
                                    (imgData->GetPixelContainer()) );
      }
    else 
      {
      // pointer could not be cast back down
      itkExceptionMacro( << "itk::SliceContiguousImage::Graft() cannot cast "
                         << typeid(data).name() << " to "
                         << typeid(const Self *).name() );
      }
    }
}


template<class TPixel>
void 
SliceContiguousImage<TPixel>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
  os << indent << "PixelContainer: " << std::endl;
  m_Container->Print(os, indent.GetNextIndent());
// m_Origin and m_Spacing are printed in the Superclass
}

} // end namespace itk

#endif
