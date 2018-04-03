/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkSliceContiguousImageNeighborhoodAccessorFunctor.h,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkSliceContiguousImageNeighborhoodAccessorFunctor_h
#define __itkSliceContiguousImageNeighborhoodAccessorFunctor_h

#include "itkImageBoundaryCondition.h"
#include "itkNeighborhood.h"
#include "itkImageBase.h"

namespace itk
{

/** \class SliceContiguousImageNeighborhoodAccessorFunctor
 * \brief Provides accessor interfaces to Access pixels and is meant to be
 * used on pointers to pixels held by the Neighborhood class.
 *
 * A typical user should not need to use this class. The class is internally
 * used by the neighborhood iterators.
 *
 * \author Dan Mueller, Philips Healthcare, PII Development
 *
 * This implementation was taken from the Insight Journal paper:
 * http://hdl.handle.net/10380/3068
 *
 */
template< class TImage >
class SliceContiguousImageNeighborhoodAccessorFunctor
{
public:
  typedef TImage                                ImageType;
  typedef typename ImageType::PixelType         PixelType;
  typedef typename ImageType::InternalPixelType InternalPixelType;
  typedef unsigned int                          VectorLengthType;
  typedef typename ImageType::OffsetType        OffsetType;
  typedef typename ImageType::SizeType          SizeType;
  typedef std::vector< PixelType * >            SliceArrayType;

  itkStaticConstMacro(ImageDimension, unsigned int, 3);

  typedef Neighborhood< InternalPixelType *, ImageDimension > NeighborhoodType;

  typedef ImageBoundaryCondition< ImageType > const 
                          *ImageBoundaryConditionConstPointerType;

  SliceContiguousImageNeighborhoodAccessorFunctor( SliceArrayType* slices, SizeType size )
      : m_Slices( slices ), m_Size( size ), m_SizeOfSlice( size[0]*size[1] ) { };
  SliceContiguousImageNeighborhoodAccessorFunctor()
      : m_Slices( NULL ), m_SizeOfSlice( 1 ) {};

  /** Set the pointer index to the start of the buffer.
   * This must be set by the iterators to the starting location of the buffer.
   * Typically a neighborhood iterator iterating on a neighborhood of an Image,
   * say \c image will set this in its constructor. For instance:
   * 
   * \code
   * ConstNeighborhoodIterator( radius, image, )
   *   {
   *   ...
   *   m_NeighborhoodAccessorFunctor.SetBegin( image->GetBufferPointer() );
   *   }
   * \endcode
   */
  inline void SetBegin( const InternalPixelType * begin )  // NOTE: begin is always 0
    { this->m_Begin = const_cast< InternalPixelType * >( begin ); }

  /** Method to dereference a pixel pointer. This is used from the 
   * ConstNeighborhoodIterator as the equivalent operation to (*it).
   * This method should be preferred over the former (*it) notation. 
   * The reason is that dereferencing a pointer to a location of 
   * VectorImage pixel involves a different operation that simply
   * dereferencing the pointer. Here a PixelType (array of InternalPixelType s)
   * is created on the stack and returned.  */
  inline PixelType Get( const InternalPixelType *pixelPointer ) const
    {
    unsigned long offset = pixelPointer  - m_Begin; // NOTE: begin is always 0
    const unsigned long slice = offset / m_SizeOfSlice;
    const unsigned long sliceOffset = offset % m_SizeOfSlice;
    return *(m_Slices->operator[](slice) + sliceOffset);
    }

  /** Method to set the pixel value at a certain pixel pointer */
  inline void Set( InternalPixelType* &pixelPointer, const PixelType &p ) const
    {
    unsigned long offset = pixelPointer - m_Begin; // NOTE: begin is always 0
    const unsigned long slice = offset / m_SizeOfSlice;
    const unsigned long sliceOffset = offset % m_SizeOfSlice;
    InternalPixelType *truePixelPointer = (m_Slices->operator[](slice) + sliceOffset);
    *truePixelPointer = p;
    }

  inline PixelType BoundaryCondition(
      const OffsetType& point_index,
      const OffsetType &boundary_offset,
      const NeighborhoodType *data,
      const ImageBoundaryConditionConstPointerType boundaryCondition) const
    {
    return boundaryCondition->operator()(point_index, boundary_offset, data, *this);
    }

  /** Required for some filters to compile. */
  void SetVectorLength( VectorLengthType length )
    {
    }

  /** Required for some filters to compile. */
  VectorLengthType SetVectorLength()
    {
    return 0;
    }
private:
  SliceArrayType* m_Slices;
  SizeType m_Size;
  unsigned long m_SizeOfSlice; // Pre-computed for speed
  InternalPixelType *m_Begin;  // Begin of the buffer, always 0
};

} // end namespace itk
#endif
