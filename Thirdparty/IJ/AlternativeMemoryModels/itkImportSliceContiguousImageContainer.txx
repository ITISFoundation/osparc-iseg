/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkImportSliceContiguousImageContainer.txx,v $
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
#ifndef _itkImportSliceContiguousImageContainer_txx
#define _itkImportSliceContiguousImageContainer_txx

#include "itkImportSliceContiguousImageContainer.h"

namespace itk
{

template <typename TElementIdentifier, typename TElement>
ImportSliceContiguousImageContainer<TElementIdentifier , TElement>
::ImportSliceContiguousImageContainer()
{
  m_ContainerManageMemory = true;
  m_Capacity = 0;
  m_SizeOfSlice = 0;
  m_NumberOfSlices = 0;
}


template <typename TElementIdentifier, typename TElement>
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::~ImportSliceContiguousImageContainer()
{
  if( m_ContainerManageMemory )
    {
    typename SliceArrayType::iterator itr = m_SlicesArray.begin();
    typename SliceArrayType::iterator end = m_SlicesArray.end();
    while( itr != end )
      {
      TElement * sliceToBeReleased = *itr;
      delete [] sliceToBeReleased;
      ++itr;
      }
    m_SlicesArray.clear();
    }
}


/**
 * Tell the container to allocate enough memory to allow at least
 * as many elements as the size given to be stored.
 */
template <typename TElementIdentifier, typename TElement>
void
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::Reserve(ElementIdentifier numberOfSlices, ElementIdentifier sizeOfSlice)
{
  if ( m_SlicesArray.size() > 0 )
    {
    // Ensure the existing size is the same as new size to reserve
    if ( sizeOfSlice != m_SizeOfSlice )
      {
      itkExceptionMacro(<< "Reserve size must be the same as the existing size." );
      return;
      }

    if ( numberOfSlices > m_NumberOfSlices)
      {
      // Allocate new slices
      SliceArrayType temp = AllocateSlices( sizeOfSlice, numberOfSlices );

      // Copy the old into the new
      typename SliceArrayType::iterator itNew = temp.begin();
      typename SliceArrayType::iterator it = m_SlicesArray.begin();
      typename SliceArrayType::iterator end = m_SlicesArray.end();
      while( it != end )
        {
        TElement * srcSlice = *it;
        TElement * destSlice = *itNew;
        memcpy( destSlice, srcSlice, sizeOfSlice );
        ++it;
        ++itNew;
        }

      // Release the old slices
      if( m_ContainerManageMemory )
        {
        it = m_SlicesArray.begin();
        end = m_SlicesArray.end();
        while( it != end )
          {
          TElement * sliceToBeReleased = *it;
          delete [] sliceToBeReleased;
          ++it;
          }
        m_SlicesArray.clear();
        }
      m_SlicesArray = temp;
      m_ContainerManageMemory = true;
      m_SizeOfSlice = sizeOfSlice;
      m_NumberOfSlices = numberOfSlices;
      m_Capacity = numberOfSlices;
      this->Modified();
      }
    else
      {
      m_NumberOfSlices = numberOfSlices;
      this->Modified();
      }
    }
  else
    {
    m_SlicesArray = AllocateSlices( numberOfSlices, sizeOfSlice );
    m_ContainerManageMemory = true;
    m_SizeOfSlice = sizeOfSlice;
    m_NumberOfSlices = numberOfSlices;
    m_Capacity = numberOfSlices;
    this->Modified();
    }
}


/**
 * Tell the container to try to minimize its memory usage for storage of
 * the current number of elements.
 */
template <typename TElementIdentifier, typename TElement>
void
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::Squeeze(void)
{
  if ( m_SlicesArray.size() > 0 )
    {
    if (m_NumberOfSlices < m_Capacity)
      {
      // Release unused slices
      if( m_ContainerManageMemory )
        {
        while ( m_NumberOfSlices < m_Capacity )
          {
          TElement * sliceToBeReleased = m_SlicesArray[m_Capacity--];
          delete [] sliceToBeReleased;
          m_SlicesArray.pop_back();
          }
        }
      m_ContainerManageMemory = true;
      m_Capacity = m_NumberOfSlices;
      this->Modified();
      }
    }
}


/**
 * Tell the container to release any of its allocated memory.
 */
template <typename TElementIdentifier, typename TElement>
void
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::Initialize(void)
{
  if ( m_SlicesArray.size() > 0 )
    {
    if( m_ContainerManageMemory )
      {
      typename SliceArrayType::iterator itr = m_SlicesArray.begin();
      typename SliceArrayType::iterator end = m_SlicesArray.end();
      while( itr != end )
        {
        TElement * sliceToBeReleased = *itr;
        delete [] sliceToBeReleased;
        ++itr;
        }
      m_SlicesArray.clear();
      }
    m_ContainerManageMemory = true;
    m_Capacity = 0;
    m_SizeOfSlice = 0;
    m_NumberOfSlices = 0;
    this->Modified();
    }
}


template <typename TElementIdentifier, typename TElement>
void
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::SetImportPointersForSlices(SliceArrayType& slices,
                             TElementIdentifier sizeOfSlice,
                             bool LetContainerManageMemory)
{
  // Free existing slices
  if( m_ContainerManageMemory )
    {
    typename SliceArrayType::iterator itr = m_SlicesArray.begin();
    typename SliceArrayType::iterator end = m_SlicesArray.end();
    while( itr != end )
      {
      TElement * sliceToBeReleased = *itr;
      delete [] sliceToBeReleased;
      ++itr;
      }
    m_SlicesArray.clear();
    }

  // Set ivars
  m_ContainerManageMemory = LetContainerManageMemory;
  m_SizeOfSlice = sizeOfSlice;
  m_NumberOfSlices = slices.size();
  m_Capacity = m_NumberOfSlices;

  // Take a copy of the slice pointers
  m_SlicesArray.resize( m_NumberOfSlices );
  std::copy( slices.begin(), slices.end(), m_SlicesArray.begin() );

  this->Modified();
}


template <typename TElementIdentifier, typename TElement>
typename ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::SliceArrayType
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::AllocateSlices(ElementIdentifier numberOfSlices, ElementIdentifier sizeOfSlice) const
{
  // Encapsulate all image memory allocation here to throw an
  // exception when memory allocation fails even when the compiler
  // does not do this by default.
  bool success = false;
  TElement* data;
  SliceArrayType slices;
  try
    {
    for (unsigned int i=0; i<numberOfSlices; i++)
      {
      data = new TElement[sizeOfSlice];
      slices.push_back( data );
      }
    success = true;
    }
  catch(...)
    {
    success = false;
    }
  if(!success)
    {
    // We cannot construct an error string here because we may be out
    // of memory.  Do not use the exception macro.
    throw MemoryAllocationError(__FILE__, __LINE__,
                                "Failed to allocate memory for image.",
                                ITK_LOCATION);
    }
  return slices;
}


template <typename TElementIdentifier, typename TElement>
void
ImportSliceContiguousImageContainer< TElementIdentifier , TElement >
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Container manages memory: "
     << (m_ContainerManageMemory ? "true" : "false") << std::endl;
  os << indent << "SizeOfSlice: " << m_SizeOfSlice << std::endl;
  os << indent << "NumberOfSlices: " << m_NumberOfSlices << std::endl;
  os << indent << "Capacity: " << m_Capacity << std::endl;
}

} // end namespace itk

#endif
