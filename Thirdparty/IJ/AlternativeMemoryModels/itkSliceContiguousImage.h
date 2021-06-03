/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkSliceContiguousImage.h,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkSliceContiguousImage_h
#define __itkSliceContiguousImage_h

#include "itkImageBase.h"
#include "itkImageRegion.h"
#include "itkSliceContiguousImageContainer.h"
#include "itkSliceContiguousImagePixelAccessor.h"
#include "itkSliceContiguousImagePixelAccessorFunctor.h"
#include "itkSliceContiguousImageNeighborhoodAccessorFunctor.h"
#include "itkNeighborhoodAccessorFunctor.h"
#include "itkPoint.h"
#include "itkContinuousIndex.h"
#include "itkWeakPointer.h"

namespace itk
{

/** \class SliceContiguousImage
 *  \brief A 3-dimensional image with slice contiguous memory model.
 *
 * The elements for a normal itk::Image are stored in a single, contiguous
 * 1-D array. The elements for this image are stored in a slice contiguous
 * manner ie. elements within a slice are stored in a contiguous 1D array,
 * but slices are not necessarily adjacent in memory. This memory model
 * fits well with the DICOM standard for images.
 *
 * The class is templated over the pixel type. A slice contiguous memory
 * model only makes sense for 3D images, so this image is not templated
 * over the dimension (it is always 3).
 *
 * \author Dan Mueller, Philips Healthcare, PII Development
 *
 * This implementation was taken from the Insight Journal paper:
 * http://hdl.handle.net/10380/3068
 *
 */
template <class TPixel>
class ITK_TEMPLATE_EXPORT SliceContiguousImage : public ImageBase<3>
{
public:
  /** Standard class type aliases */
  using Self = SliceContiguousImage;
  using Superclass = ImageBase<3>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;
  using ConstWeakPointer = WeakPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(SliceContiguousImage, ImageBase);

  /** Pixel type. */
  using PixelType = TPixel;

  /** This is the actual pixel type contained in the buffer. */
  using InternalPixelType = TPixel;

  /** Typedef alias for PixelType */
  using ValueType = TPixel;

  using IOPixelType = InternalPixelType;

  /** Dimension of the image.  This constant is used by functions that are
   * templated over image type (as opposed to being templated over pixel type
   * and dimension) when they need compile time access to the dimension of
   * the image. */
  itkStaticConstMacro(ImageDimension, unsigned int, Superclass::ImageDimension);

  /** Container used to store pixels in the image. */
  using PixelContainer = SliceContiguousImageContainer<unsigned long, InternalPixelType>;

  /** Index typedef support. An index is used to access pixel values. */
  using IndexType = typename Superclass::IndexType;

  /** Offset typedef support. An offset is used to access pixel values. */
  using OffsetType = typename Superclass::OffsetType;

  /** Size typedef support. A size is used to define region bounds. */
  using SizeType = typename Superclass::SizeType;

  /** Direction typedef support. A matrix of direction cosines. */
  using DirectionType = typename Superclass::DirectionType;

  /** Region typedef support. A region is used to specify a subset of an image. */
  using RegionType = typename Superclass::RegionType;

  /** Spacing typedef support.  Spacing holds the size of a pixel.  The
   * spacing is the geometric distance between image samples. */
  using SpacingType = typename Superclass::SpacingType;

  /** Origin typedef support.  The origin is the geometric coordinates
   * of the index (0,0). */
  using PointType = typename Superclass::PointType;

  /** A pointer to the pixel container. */
  using PixelContainerPointer = typename PixelContainer::Pointer;
  using PixelContainerConstPointer = typename PixelContainer::ConstPointer;

  /** Offset typedef (relative position between indices) */
  using OffsetValueType = typename Superclass::OffsetValueType;

  /** Accessor type that convert data between internal and external
   *  representations. */
  using AccessorType = SliceContiguousImagePixelAccessor<InternalPixelType, SizeType>;

  /** Tyepdef for the functor used to access pixels.*/
  using AccessorFunctorType = SliceContiguousImagePixelAccessorFunctor<Self>;

  /** Tyepdef for the functor used to access a neighborhood of pixel pointers.*/
  using NeighborhoodAccessorFunctorType = SliceContiguousImageNeighborhoodAccessorFunctor<Self>;

  /** Allocate the image memory. The size of the image must
   * already be set, e.g. by calling SetRegions(). */
  void
  Allocate(bool initialize = false) override;

  /** Restore the data object to its initial state. This means releasing
   * memory. */
  void
  Initialize() override;

  /** Fill the image buffer with a value.  Be sure to call Allocate()
   * first. */
  void
  FillBuffer(const PixelType & value);

  /** \brief Set a pixel value.
   *
   * Allocate() needs to have been called first -- for efficiency,
   * this function does not check that the image has actually been
   * allocated yet. */
  void
  SetPixel(const IndexType & index, const PixelType & value)
  {
    typename PixelContainer::SliceArrayType * slices = m_Container->GetSlices();
    OffsetValueType                           offset = this->ComputeOffset(index);
    SizeType                                  size = this->GetLargestPossibleRegion().GetSize();
    unsigned long                             sizeOfSlice = size[0] * size[1];
    const unsigned long                       sliceIndex = offset / sizeOfSlice;
    const unsigned long                       sliceOffset = offset % sizeOfSlice;
    slices->                                  operator[](sliceIndex)[sliceOffset] = value;
  }

  /** \brief Get a pixel (read only version).
   *
   * For efficiency, this function does not check that the
   * image has actually been allocated yet. Note that the method returns a
   * pixel on the stack. */
  const PixelType &
  GetPixel(const IndexType & index) const
  {
    typename PixelContainer::SliceArrayType * slices = m_Container->GetSlices();
    OffsetValueType                           offset = this->ComputeOffset(index);
    SizeType                                  size = this->GetLargestPossibleRegion().GetSize();
    unsigned long                             sizeOfSlice = size[0] * size[1];
    const unsigned long                       sliceIndex = offset / sizeOfSlice;
    const unsigned long                       sliceOffset = offset % sizeOfSlice;
    return (slices->operator[](sliceIndex)[sliceOffset]);
  }

  /** \brief Get a reference to a pixel (e.g. for editing).
   *
   * For efficiency, this function does not check that the
   * image has actually been allocated yet. */
  PixelType &
  GetPixel(const IndexType & index)
  {
    typename PixelContainer::SliceArrayType * slices = m_Container->GetSlices();
    OffsetValueType                           offset = this->ComputeOffset(index);
    SizeType                                  size = this->GetLargestPossibleRegion().GetSize();
    unsigned long                             sizeOfSlice = size[0] * size[1];
    const unsigned long                       sliceIndex = offset / sizeOfSlice;
    const unsigned long                       sliceOffset = offset % sizeOfSlice;
    return (slices->operator[](sliceIndex)[sliceOffset]);
  }

  /** \brief Access a pixel. This version cannot be an lvalue because the pixel
   * is converted on the fly to a VariableLengthVector.
   *
   * For efficiency, this function does not check that the
   * image has actually been allocated yet. */
  PixelType &
  operator[](const IndexType & index)
  {
    return this->GetPixel(index);
  }

  /** \brief Access a pixel.
   *
   * For efficiency, this function does not check that the
   * image has actually been allocated yet. */
  PixelType &
  operator[](const IndexType & index) const
  {
    return this->GetPixel(index);
  }

  /** Slice contiguous images do not have buffer. This method always returns 0 */
  InternalPixelType *
  GetBufferPointer()
  {
    return nullptr;
  }
  const InternalPixelType *
  GetBufferPointer() const
  {
    return nullptr;
  }

  /** Return a pointer to the container. */
  PixelContainer *
  GetPixelContainer()
  {
    return m_Container.GetPointer();
  }

  /** Return a pointer to the container. */
  const PixelContainer *
  GetPixelContainer() const
  {
    return m_Container.GetPointer();
  }

  /** Set the container to use. Note that this does not cause the
   * DataObject to be modified. */
  void
  SetPixelContainer(PixelContainer * container);

  /** Graft the data and information from one image to another. This
   * is a convenience method to setup a second image with all the meta
   * information of another image and use the same pixel
   * container. Note that this method is different than just using two
   * SmartPointers to the same image since separate DataObjects are
   * still maintained. This method is similar to
   * ImageSource::GraftOutput(). The implementation in ImageBase
   * simply calls CopyInformation() and copies the region ivars.
   * The implementation here refers to the superclass' implementation
   * and then copies over the pixel container. */
  void
  Graft(const DataObject * data) override;

  /** Return the Pixel Accessor object */
  AccessorType
  GetPixelAccessor()
  {
    return AccessorType(m_Container->GetSlices(), this->GetLargestPossibleRegion().GetSize());
  }

  /** Return the Pixel Accesor object */
  const AccessorType
  GetPixelAccessor() const
  {
    return AccessorType(m_Container->GetSlices(), this->GetLargestPossibleRegion().GetSize());
  }

  /** Return the NeighborhoodAccessor functor */
  NeighborhoodAccessorFunctorType
  GetNeighborhoodAccessor()
  {
    return NeighborhoodAccessorFunctorType(m_Container->GetSlices(), this->GetLargestPossibleRegion().GetSize());
  }

  /** Return the NeighborhoodAccessor functor */
  const NeighborhoodAccessorFunctorType
  GetNeighborhoodAccessor() const
  {
    return NeighborhoodAccessorFunctorType(m_Container->GetSlices(), this->GetLargestPossibleRegion().GetSize());
  }

protected:
  SliceContiguousImage();
  void
  PrintSelf(std::ostream & os, Indent indent) const override;
  ~SliceContiguousImage() override = default;

private:
  SliceContiguousImage(const Self &) = delete;
  void
  operator=(const Self &) = delete;

  /** Memory for the slices containing the pixel data. */
  PixelContainerPointer m_Container;
};


} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkSliceContiguousImage.txx"
#endif

#endif
