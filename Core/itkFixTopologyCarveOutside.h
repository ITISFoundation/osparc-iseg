/// \file itkFixTopologyCarveOutside.h
///
/// \author Bryn Lloyd
/// \copyright 2020, IT'IS Foundation

#ifndef itkFixTopologyCarveOutside_h
#define itkFixTopologyCarveOutside_h

#include <itkConstantBoundaryCondition.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToImageFilter.h>
#include <itkNeighborhoodIterator.h>

namespace itk {
/** \class FixTopologyCarveOutside
 *
 * \brief This filter adds voxels to the "inside" region making it manifold with genus 0.
 *
 * This filter implements ideas from:
 *
 * Vanderhyde, James. "Topology control of volumetric data." 
 * PhD diss., Georgia Institute of Technology, 2007..
 */
template<class TInputImage, class TOutputImage>
class /*ITK_TEMPLATE_EXPORT*/ FixTopologyCarveOutside : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
	ITK_DISALLOW_COPY_AND_ASSIGN(FixTopologyCarveOutside);

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

	/** Type for the region of the input image. */
	using RegionType = typename InputImageType::RegionType;

	/** Type for the index of the input image. */
	using IndexType = typename RegionType::IndexType;

	/** Type for the pixel type of the input image. */
	using InputImagePixelType = typename InputImageType::PixelType;

	/** Type for the pixel type of the input image. */
	using OutputImagePixelType = typename OutputImageType::PixelType;

	/** Type for the size of the input image. */
	using SizeType = typename RegionType::SizeType;

	/** Pointer Type for input image. */
	using InputImagePointer = typename InputImageType::ConstPointer;

	/** Pointer Type for the output image. */
	using OutputImagePointer = typename OutputImageType::Pointer;

	/** Boundary condition type for the neighborhood iterator */
	using ConstBoundaryConditionType = ConstantBoundaryCondition<TOutputImage>;

	/** Neighborhood iterator type */
	using NeighborhoodIteratorType = NeighborhoodIterator<TOutputImage, ConstBoundaryConditionType>;

	/** Neighborhood type */
	using NeighborhoodType = typename NeighborhoodIteratorType::NeighborhoodType;

	itkSetMacro(InsideValue, InputImagePixelType);
	itkGetConstMacro(InsideValue, InputImagePixelType);

	itkSetMacro(OutsideValue, InputImagePixelType);
	itkGetConstMacro(OutsideValue, InputImagePixelType);

	/** Get Skeleton by thinning image. */
	OutputImageType* GetThinning();

	/** ImageDimension enumeration   */
	itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
	itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

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
	void PrintSelf(std::ostream& os, Indent indent) const override;

	/** Compute thinning Image. */
	void GenerateData() override;

	/** Prepare data. */
	void PrepareData();

	/** Compute thinning image. */
	void ComputeThinImage();

private:
	InputImagePixelType m_InsideValue = 1;
	InputImagePixelType m_OutsideValue = 0;

}; // end of FixTopologyCarveOutside class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#	include "itkFixTopologyCarveOutside.hxx"
#endif

#endif // itkFixTopologyCarveOutside_h
