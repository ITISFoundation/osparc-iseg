#include "SlicesHandlerITKInterface.h"

#include <itkCastImageFilter.h>

namespace iseg {

itk::SliceContiguousImage<float>::Pointer SlicesHandlerITKInterface::GetSource(bool active_slices)
{
	if (active_slices)
		return GetSource(m_Handler->StartSlice(), m_Handler->EndSlice());
	else
		return GetSource(0, m_Handler->NumSlices());
}

itk::SliceContiguousImage<float>::Pointer SlicesHandlerITKInterface::GetSource(size_t start_slice, size_t end_slice)
{
	unsigned dims[3] = {m_Handler->Width(), m_Handler->Height(), m_Handler->NumSlices()};
	return wrapToITK(m_Handler->SourceSlices(), dims, start_slice, end_slice, m_Handler->Spacing(), m_Handler->ImageTransform());
}

itk::SliceContiguousImage<float>::Pointer SlicesHandlerITKInterface::GetTarget(bool active_slices)
{
	if (active_slices)
		return GetTarget(m_Handler->StartSlice(), m_Handler->EndSlice());
	else
		return GetTarget(0, m_Handler->NumSlices());
}

itk::SliceContiguousImage<float>::Pointer SlicesHandlerITKInterface::GetTarget(size_t start_slice, size_t end_slice)
{
	unsigned dims[3] = {m_Handler->Width(), m_Handler->Height(), m_Handler->NumSlices()};
	return wrapToITK(m_Handler->TargetSlices(), dims, start_slice, end_slice, m_Handler->Spacing(), m_Handler->ImageTransform());
}

itk::SliceContiguousImage<tissues_size_t>::Pointer SlicesHandlerITKInterface::GetTissues(bool active_slices)
{
	if (active_slices)
		return GetTissues(m_Handler->StartSlice(), m_Handler->EndSlice());
	else
		return GetTissues(0, m_Handler->NumSlices());
}

itk::SliceContiguousImage<tissues_size_t>::Pointer SlicesHandlerITKInterface::GetTissues(size_t start_slice, size_t end_slice)
{
	unsigned dims[3] = {m_Handler->Width(), m_Handler->Height(), m_Handler->NumSlices()};
	auto all_slices = m_Handler->TissueSlices(m_Handler->ActiveTissuelayer());
	return wrapToITK(all_slices, dims, start_slice, end_slice, m_Handler->Spacing(), m_Handler->ImageTransform());
}

template<typename T>
typename itk::Image<T>::Pointer _GetITKView2D(T* slice, size_t dims[2], double spacing[2])
{
	using image_type = itk::Image<T, 2>;

	typename image_type::IndexType start;
	start[0] = 0; // first index on X
	start[1] = 0; // first index on Y
	typename image_type::SizeType size;
	size[0] = dims[0]; // size along X
	size[1] = dims[1]; // size along Y

	auto image = itk::Image<T>::New();
	image->SetRegions(typename image_type::RegionType(start, size));
	image->SetSpacing(spacing);
	// \warning transform is ignored
	//image->SetOrigin(origin);
	//image->SetDirection(direction);

	bool const manage_memory = false;
	image->GetPixelContainer()->SetImportPointer(slice, size[0] * size[1], manage_memory);
	return image;
}

itk::Image<float, 2>::Pointer SlicesHandlerITKInterface::GetSourceSlice(int slice)
{
	size_t dims[2] = {
			m_Handler->Width(), m_Handler->Height()};
	auto spacing3d = m_Handler->Spacing();
	double spacing[2] = {spacing3d[0], spacing3d[1]};

	auto all_slices = m_Handler->SourceSlices();
	slice = (slice == kActiveSlice) ? m_Handler->ActiveSlice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::Image<float, 2>::Pointer SlicesHandlerITKInterface::GetTargetSlice(int slice)
{
	size_t dims[2] = {
			m_Handler->Width(), m_Handler->Height()};
	auto spacing3d = m_Handler->Spacing();
	double spacing[2] = {spacing3d[0], spacing3d[1]};

	auto all_slices = m_Handler->TargetSlices();
	slice = (slice == kActiveSlice) ? m_Handler->ActiveSlice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::Image<tissues_size_t, 2>::Pointer SlicesHandlerITKInterface::GetTissuesSlice(int slice)
{
	size_t dims[2] = {
			m_Handler->Width(), m_Handler->Height()};
	auto spacing3d = m_Handler->Spacing();
	double spacing[2] = {spacing3d[0], spacing3d[1]};

	auto all_slices = m_Handler->TissueSlices(m_Handler->ActiveTissuelayer());
	slice = (slice == kActiveSlice) ? m_Handler->ActiveSlice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::ImageRegion<3> SlicesHandlerITKInterface::GetActiveRegion() const
{
	itk::Index<3> start = {0, 0, m_Handler->StartSlice()};
	itk::Size<3> size = {m_Handler->Width(), m_Handler->Height(), static_cast<size_t>(m_Handler->EndSlice() - m_Handler->StartSlice())};
	return itk::ImageRegion<3>(start, size);
}

itk::Image<float, 3>::Pointer SlicesHandlerITKInterface::GetImageDeprecated(eImageType type, bool active_slices)
{
	using input_image_type = itk::SliceContiguousImage<float>;
	using output_image_type = itk::Image<float, 3>;

	auto cast = itk::CastImageFilter<input_image_type, output_image_type>::New();
	cast->SetInput(type == eImageType::kSource ? GetSource(active_slices) : GetTarget(active_slices));
	cast->Update();
	return cast->GetOutput();
}

itk::Image<tissues_size_t, 3>::Pointer SlicesHandlerITKInterface::GetTissuesDeprecated(bool active_slices)
{
	using input_image_type = itk::SliceContiguousImage<tissues_size_t>;
	using output_image_type = itk::Image<tissues_size_t, 3>;

	auto cast = itk::CastImageFilter<input_image_type, output_image_type>::New();
	cast->SetInput(GetTissues(active_slices));
	cast->Update();
	return cast->GetOutput();
}

} // namespace iseg
