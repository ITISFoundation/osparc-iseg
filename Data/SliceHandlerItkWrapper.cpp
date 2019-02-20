#include "SliceHandlerItkWrapper.h"

#include <itkCastImageFilter.h>

namespace iseg {

itk::SliceContiguousImage<float>::Pointer SliceHandlerItkWrapper::GetSource(bool active_slices)
{
	if (active_slices)
		return GetSource(_handler->start_slice(), _handler->end_slice());
	else
		return GetSource(0, _handler->num_slices());
}

itk::SliceContiguousImage<float>::Pointer SliceHandlerItkWrapper::GetSource(size_t start_slice, size_t end_slice)
{
	unsigned dims[3] = {_handler->width(), _handler->height(), _handler->num_slices()};
	return wrapToITK(_handler->source_slices(), dims, start_slice, end_slice, _handler->spacing(), _handler->transform());
}

itk::SliceContiguousImage<float>::Pointer SliceHandlerItkWrapper::GetTarget(bool active_slices)
{
	if (active_slices)
		return GetTarget(_handler->start_slice(), _handler->end_slice());
	else
		return GetTarget(0, _handler->num_slices());
}

itk::SliceContiguousImage<float>::Pointer SliceHandlerItkWrapper::GetTarget(size_t start_slice, size_t end_slice)
{
	unsigned dims[3] = {_handler->width(), _handler->height(), _handler->num_slices()};
	return wrapToITK(_handler->target_slices(), dims, start_slice, end_slice, _handler->spacing(), _handler->transform());
}

itk::SliceContiguousImage<tissues_size_t>::Pointer SliceHandlerItkWrapper::GetTissues(bool active_slices)
{
	if (active_slices)
		return GetTissues(_handler->start_slice(), _handler->end_slice());
	else
		return GetTissues(0, _handler->num_slices());
}

itk::SliceContiguousImage<tissues_size_t>::Pointer SliceHandlerItkWrapper::GetTissues(size_t start_slice, size_t end_slice)
{
	unsigned dims[3] = {_handler->width(), _handler->height(), _handler->num_slices()};
	auto all_slices = _handler->tissue_slices(_handler->active_tissuelayer());
	return wrapToITK(all_slices, dims, start_slice, end_slice, _handler->spacing(), _handler->transform());
}

template<typename T>
typename itk::Image<T>::Pointer _GetITKView2D(T* slice, size_t dims[2], double spacing[2])
{
	typedef itk::Image<T, 2> image_type;

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

itk::Image<float, 2>::Pointer SliceHandlerItkWrapper::GetSourceSlice(int slice)
{
	size_t dims[2] = {
			_handler->width(),
			_handler->height()};
	auto spacing3d = _handler->spacing();
	double spacing[2] = {spacing3d[0], spacing3d[1]};

	auto all_slices = _handler->source_slices();
	slice = (slice == kActiveSlice) ? _handler->active_slice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::Image<float, 2>::Pointer SliceHandlerItkWrapper::GetTargetSlice(int slice)
{
	size_t dims[2] = {
			_handler->width(),
			_handler->height()};
	auto spacing3d = _handler->spacing();
	double spacing[2] = {spacing3d[0], spacing3d[1]};

	auto all_slices = _handler->target_slices();
	slice = (slice == kActiveSlice) ? _handler->active_slice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::Image<tissues_size_t, 2>::Pointer SliceHandlerItkWrapper::GetTissuesSlice(int slice)
{
	size_t dims[2] = {
			_handler->width(),
			_handler->height()};
	auto spacing3d = _handler->spacing();
	double spacing[2] = {spacing3d[0], spacing3d[1]};

	auto all_slices = _handler->tissue_slices(_handler->active_tissuelayer());
	slice = (slice == kActiveSlice) ? _handler->active_slice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::ImageRegion<3> SliceHandlerItkWrapper::GetActiveRegion() const
{
	itk::Index<3> start = {0, 0, _handler->start_slice()};
	itk::Size<3> size = {_handler->width(), _handler->height(), static_cast<size_t>(_handler->end_slice() - _handler->start_slice())};
	return itk::ImageRegion<3>(start, size);
}

itk::Image<float, 3>::Pointer SliceHandlerItkWrapper::GetImageDeprecated(eImageType type, bool active_slices)
{
	using input_image = itk::SliceContiguousImage<float>;
	using output_image = itk::Image<float, 3>;

	auto cast = itk::CastImageFilter<input_image, output_image>::New();
	cast->SetInput(type == eImageType::kSource ? GetSource(active_slices) : GetTarget(active_slices));
	cast->Update();
	return cast->GetOutput();
}

itk::Image<tissues_size_t, 3>::Pointer SliceHandlerItkWrapper::GetTissuesDeprecated(bool active_slices)
{
	using input_image = itk::SliceContiguousImage<tissues_size_t>;
	using output_image = itk::Image<tissues_size_t, 3>;

	auto cast = itk::CastImageFilter<input_image, output_image>::New();
	cast->SetInput(GetTissues(active_slices));
	cast->Update();
	return cast->GetOutput();
}

} // namespace iseg
