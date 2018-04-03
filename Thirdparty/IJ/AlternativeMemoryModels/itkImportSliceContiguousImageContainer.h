/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkImportSliceContiguousImageContainer,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkImportSliceContiguousImageContainer_h
#define __itkImportSliceContiguousImageContainer_h

#include "itkObject.h"
#include "itkObjectFactory.h"
#include <utility>

namespace itk
{

/** \class ImportSliceContiguousImageContainer
 *  \brief Image container for itk::SliceContiguousImage.
 *
 * \author Dan Mueller, Philips Healthcare, PII Development
 *
 * This implementation was taken from the Insight Journal paper:
 * http://hdl.handle.net/10380/3068
 *
 */
  
template <typename TElementIdentifier, typename TElement>
class ImportSliceContiguousImageContainer:  public Object
{
public:
  /** Standard class typedefs. */
  typedef ImportSliceContiguousImageContainer Self;
  typedef Object Superclass;
  typedef SmartPointer<Self> Pointer;
  typedef SmartPointer<const Self> ConstPointer;
    
  /** Save the template parameters. */
  typedef TElementIdentifier  ElementIdentifier;
  typedef TElement            Element;
  typedef std::vector< TElement * > SliceArrayType;
    
  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Standard part of every itk Object. */
  itkTypeMacro(ImportSliceContiguousImageContainer, Object);

  /** Copy the slice arrays from which the image is imported.
   *  If "LetContainerManageMemory" is false, then the application retains
   *  the responsibility of freeing the memory for this image data.
   *  If "LetContainerManageMemory" is true, then this class will free
   *  the memory when this object is destroyed.*/
  void SetImportPointersForSlices(SliceArrayType& slices,
                                  TElementIdentifier sizeOfSlice,
                                  bool LetContainerManageMemory = false);

  /** Get the slices comprising the slice contiguous buffer. */
  SliceArrayType* GetSlices()
    { return &m_SlicesArray; }

  /** Get the slice capacity of the container. */
  unsigned long Capacity(void) const
    { return (unsigned long) m_Capacity; };

  /** Get the number of elements currently stored in the container. */
  unsigned long Size(void) const
    { return (unsigned long) m_SizeOfSlice*m_NumberOfSlices; };

  /** Get the number of elements per slice currently stored in the container. */
  unsigned long SizeOfSlice(void) const
    { return (unsigned long) m_SizeOfSlice; };

  /** Get the number of slices currently stored in the container. */
  unsigned long NumberOfSlices(void) const
    { return (unsigned long) m_NumberOfSlices; };

  /** Tell the container to allocate enough memory to allow at least
   * as many elements as the size given to be stored.  If new memory
   * needs to be allocated, the contents of the old buffer are copied
   * to the new area.  The old buffer is deleted if the original pointer
   * was passed in using "LetContainerManageMemory"=true. The new buffer's
   * memory management will be handled by the container from that point on.
   *
   * \sa SetImportPointersForSlices() */
  void Reserve(ElementIdentifier numberOfSlices, ElementIdentifier sizeOfSlice);
  
  /** Tell the container to try to minimize its memory usage for
   * storage of the current number of elements.  If new memory is
   * allocated, the contents of old buffer are copied to the new area.
   * The previous buffer is deleted if the original pointer was in
   * using "LetContainerManageMemory"=true.  The new buffer's memory
   * management will be handled by the container from that point on. */
  void Squeeze(void);
  
  /** Tell the container to release any of its allocated memory. */
  void Initialize(void);

  /** These methods allow to define whether upon destruction of this class
   *  the memory buffer should be released or not.  Setting it to true
   *  (or ON) makes that this class will take care of memory release.
   *  Setting it to false (or OFF) will prevent the destructor from
   *  deleting the memory buffer. This is desirable only when the data
   *  is intended to be used by external applications.
   *  Note that the normal logic of this class set the value of the boolean
   *  flag. This may override your setting if you call this methods prematurely.
   *  \warning Improper use of these methods will result in memory leaks */
  itkSetMacro(ContainerManageMemory,bool);
  itkGetMacro(ContainerManageMemory,bool);
  itkBooleanMacro(ContainerManageMemory);

protected:
  ImportSliceContiguousImageContainer();
  virtual ~ImportSliceContiguousImageContainer();

  /** PrintSelf routine. Normally this is a protected internal method. It is
   * made public here so that Image can call this method.  Users should not
   * call this method but should call Print() instead. */
  void PrintSelf(std::ostream& os, Indent indent) const;

  virtual SliceArrayType AllocateSlices(ElementIdentifier numberOfSlices,
                                        ElementIdentifier sizeOfSlice) const;

private:
  ImportSliceContiguousImageContainer(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  SliceArrayType       m_SlicesArray;
  TElementIdentifier   m_SizeOfSlice;
  TElementIdentifier   m_NumberOfSlices;
  TElementIdentifier   m_Capacity;
  bool                 m_ContainerManageMemory;

};

} // end namespace itk

// Define instantiation macro for this template.
#define ITK_TEMPLATE_ImportSliceContiguousImageContainer(_, EXPORT, x, y) namespace itk { \
  _(2(class EXPORT ImportSliceContiguousImageContainer< ITK_TEMPLATE_2 x >)) \
  namespace Templates { typedef ImportSliceContiguousImageContainer< ITK_TEMPLATE_2 x > ImportSliceContiguousImageContainer##y; } \
  }

#if ITK_TEMPLATE_EXPLICIT
# include "Templates/ImportSliceContiguousImageContainer+-.h"
#endif

#if ITK_TEMPLATE_TXX
# include "itkImportSliceContiguousImageContainer.txx"
#endif

#endif
