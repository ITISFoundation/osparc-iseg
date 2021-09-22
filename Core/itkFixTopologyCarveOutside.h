/// \file itkFixTopologyCarveInside.h
///
/// \author Bryn Lloyd
/// \copyright 2020, IT'IS Foundation

#ifndef itkFixTopologyCarveOutside
#define itkFixTopologyCarveOutside

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToImageFilter.h>
#include <itkNeighborhoodIterator.h>
#include <itkProgressAccumulator.h>
#include <itkProgressReporter.h>

namespace itk {
/** \class FixTopologyCarveInside
 *
 * \brief This filter dilates the foreground, and then erodes from the "outside" while preserving the topology
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
	using ConstBoundaryConditionType = ConstantBoundaryCondition<TInputImage>;

	/** Neighborhood iterator type */
	using NeighborhoodIteratorType = NeighborhoodIterator<TInputImage, ConstBoundaryConditionType>;

	/** Neighborhood type */
	using NeighborhoodType = typename NeighborhoodIteratorType::NeighborhoodType;

	itkSetMacro(Radius, float);
	itkGetConstMacro(Radius, float);

	itkSetMacro(InsideValue, InputImagePixelType);
	itkGetConstMacro(InsideValue, InputImagePixelType);

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
	void PrepareData(ProgressAccumulator* progress);

	/** Compute thinning image. */
	void ComputeThinImage(ProgressAccumulator* progress);

private:
	// region id convention
	enum ePixelState : OutputImagePixelType {
		kBackground = 0,
		kHardForeground,
		kDilated,
		kVisited
	};

	OutputImagePointer m_PaddedMask;
	itk::Image<float, 3>::Pointer m_DistanceMap;

	float m_Radius = 1.f;
	InputImagePixelType m_InsideValue = 1;
}; // end of FixTopologyCarveOutside class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#	include "itkFixTopologyCarveOutside.hxx"
#endif

#endif // itkFixTopologyCarveOutside
