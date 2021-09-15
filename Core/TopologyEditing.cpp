#include "TopologyEditing.h"

#include "Morpho.h"
#include "itkLabelRegionCalculator.h"

#include "../Data/ItkUtils.h"
#include "../Data/SlicesHandlerITKInterface.h"

#include <itkBinaryThinningImageFilter3D.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkExtractImageFilter.h>

namespace iseg {

bool FillLoopsAndGaps(SlicesHandlerInterface* handler, boost::variant<int, float> radius)
{
	using target_image_type = SlicesHandlerITKInterface::image_ref_type;
	using mask_type = itk::Image<unsigned char, 3>;

	SlicesHandlerITKInterface itkhandler(handler);
	auto target = itkhandler.GetTarget(true);
	auto target_region = target->GetBufferedRegion();

	auto cast = itk::BinaryThresholdImageFilter<target_image_type, mask_type>::New();
	cast->SetInput(target);
	cast->SetLowerThreshold(1e-6f); // background is '0'
	cast->SetInsideValue(1);
	cast->SetOutsideValue(0);
	cast->Update();
	SAFE_UPDATE(cast, return false);

	auto mask = cast->GetOutput();
	auto region = itk::GetLabelRegion<mask_type>(mask, 1);

	auto working_region = region;
	working_region.PadByRadius(1);

	// working region is inside whole region, simply crop it
	if (target_region.IsInside(working_region))
	{
		auto crop = itk::ExtractImageFilter<mask_type, mask_type>::New();
		crop->SetInput(mask);
		crop->SetExtractionRegion(working_region);
		SAFE_UPDATE(crop, return false);
		mask = crop->GetOutput();
	}
	else
	{
		auto crop = itk::ExtractImageFilter<mask_type, mask_type>::New();
		crop->SetInput(mask);
		crop->SetExtractionRegion(region);

		auto pad = itk::ConstantPadImageFilter<mask_type, mask_type>::New();
		pad->SetInput(crop->GetOutput());
		pad->SetConstant(0);
		pad->SetPadBound(itk::Size<3>{1, 1, 1});
		SAFE_UPDATE(pad, return false);
		mask = pad->GetOutput();

		if (mask->GetBufferedRegion() != working_region)
		{
			throw std::runtime_error("Something is wrong with logic - unpexpected working region");
		}
	}

	// dilate
	using kernel_type = itk::FlatStructuringElement<3>;
	auto ball = MakeBall<mask_type>(mask, radius);

	auto dilate = itk::BinaryDilateImageFilter<mask_type, mask_type, kernel_type>::New();
	dilate->SetInput(mask);
	dilate->SetKernel(ball);
	dilate->SetDilateValue(1);
	SAFE_UPDATE(dilate, return false);
	auto dilated = dilate->GetOutput();

	// get surface skeleton of dilated mask
	auto thinning = itk::BinaryThinningImageFilter3D<mask_type, mask_type>::New();
	thinning->SetInput(dilated);
	SAFE_UPDATE(thinning, return false);
	auto surface_skeleton = thinning->GetOutput();

	// copy skeleton to target
	working_region.Crop(target_region);
	itk::ImageRegionConstIterator<mask_type> sit(surface_skeleton, working_region);
	itk::ImageRegionIterator<target_image_type> tit(target, working_region);
	for (sit.GoToBegin(), tit.GoToBegin(); !sit.IsAtEnd() && !tit.IsAtEnd(); ++sit, ++tit)
	{
		if (sit.Get() != 0)
			tit.Set(255.0f);
	}

	return true;
}
} // namespace iseg