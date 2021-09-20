#include "TopologyEditing.h"

#include "Morpho.h"
#include "itkFixTopologyCarveOutside2.h"
#include "itkLabelRegionCalculator.h"

#include "../Data/ItkUtils.h"
#include "../Data/Logger.h"
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
	ISEG_INFO_MSG("Casted/threshold done");

	auto mask = cast->GetOutput();
	auto working_region = itk::GetLabelRegion<mask_type>(mask, 1);
	working_region.PadByRadius(1);
	working_region.Crop(target_region);

	auto crop = itk::ExtractImageFilter<mask_type, mask_type>::New();
	crop->SetInput(mask);
	crop->SetExtractionRegion(working_region);
	SAFE_UPDATE(crop, return false);
	mask = crop->GetOutput();
	ISEG_INFO_MSG("Cropping done");

#if 0
	// dilate
	using kernel_type = itk::FlatStructuringElement<3>;
	auto ball = MakeBall<mask_type>(mask, radius);

	auto dilate = itk::BinaryDilateImageFilter<mask_type, mask_type, kernel_type>::New();
	dilate->SetInput(mask);
	dilate->SetKernel(ball);
	dilate->SetDilateValue(1);
	SAFE_UPDATE(dilate, return false);
	auto dilated = dilate->GetOutput();
	ISEG_INFO_MSG("Dilation done");
    dump_image(dilated, "/Users/lloyd/_dilated.nii.gz");

	// get surface skeleton of dilated mask
	auto thinning = itk::BinaryThinningImageFilter3D<mask_type, mask_type>::New();
	thinning->SetInput(dilated);
	SAFE_UPDATE(thinning, return false);
	auto surface_skeleton = thinning->GetOutput();
	ISEG_INFO_MSG("Skeletonization done");
	dump_image(surface_skeleton, "/Users/lloyd/_skeleton.nii.gz");
#else
	auto thinning = itk::FixTopologyCarveOutside2<mask_type, mask_type>::New();
	thinning->SetInput(mask);
	thinning->SetRadius(boost::get<float>(radius));
	SAFE_UPDATE(thinning, return false);
	auto surface_skeleton = thinning->GetOutput();
	ISEG_INFO_MSG("Skeletonization done");
#ifdef WIN32
	dump_image(surface_skeleton, "F:\\Temp\\_skeleton.nii.gz");
#else
	dump_image(surface_skeleton, "/Users/lloyd/_skeleton.nii.gz");
#endif
#endif
	// copy skeleton to target
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