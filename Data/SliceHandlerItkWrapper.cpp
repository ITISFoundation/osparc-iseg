#include "SliceHandlerItkWrapper.h"

#include <itkCastImageFilter.h>

namespace iseg {

template<typename T>
typename itk::SliceContiguousImage<T>::Pointer _GetITKView(std::vector<T*>& slices, size_t dims[3], Vec3 spacing)
{
	typedef itk::SliceContiguousImage<T> SliceContiguousImageType;

	auto image = SliceContiguousImageType::New();
	image->SetSpacing(spacing.v);
	// \bug Transform (rotation/offset) not set

	typename SliceContiguousImageType::IndexType start;
	start.Fill(0);

	typename SliceContiguousImageType::SizeType size;
	size[0] = dims[0];
	size[1] = dims[1];
	size[2] = dims[2];

	typename SliceContiguousImageType::RegionType region(start, size);
	image->SetRegions(region);
	image->Allocate();

	// Set slice pointers
	auto container = SliceContiguousImageType::PixelContainer::New();
	container->SetImportPointersForSlices(slices, size[0] * size[1], false);
	image->SetPixelContainer(container);

	return image;
}

itk::SliceContiguousImage<float>::Pointer SliceHandlerItkWrapper::GetSource(bool active_slices)
{
	auto spacing = _handler->spacing();
	auto all_slices = _handler->source_slices();
	if (!active_slices)
	{
		size_t dims[3] = {
			_handler->width(),
			_handler->height(),
			_handler->num_slices()};

		return _GetITKView(all_slices, dims, spacing);
	}

	std::vector<float*> slices;
	for (unsigned i = _handler->start_slice(); i < _handler->end_slice(); ++i)
	{
		slices.push_back(all_slices[i]);
	}
	size_t dims[3] = {
		_handler->width(),
		_handler->height(),
		static_cast<size_t>(_handler->end_slice() - _handler->start_slice()) };
	return _GetITKView(slices, dims, spacing);
}

itk::SliceContiguousImage<float>::Pointer SliceHandlerItkWrapper::GetTarget(bool active_slices)
{
	auto spacing = _handler->spacing();
	auto all_slices = _handler->target_slices();
	if (!active_slices)
	{
		size_t dims[3] = {
			_handler->width(),
			_handler->height(),
			_handler->num_slices()};

		return _GetITKView(all_slices, dims, spacing);
	}

	std::vector<float*> slices;
	for (unsigned i = _handler->start_slice(); i < _handler->end_slice(); ++i)
	{
		slices.push_back(all_slices[i]);
	}
	size_t dims[3] = {
		_handler->width(),
		_handler->height(),
		static_cast<size_t>(_handler->end_slice() - _handler->start_slice()) };
	return _GetITKView(slices, dims, spacing);
}

itk::SliceContiguousImage<tissues_size_t>::Pointer SliceHandlerItkWrapper::GetTissues(bool active_slices)
{
	auto spacing = _handler->spacing();
	auto all_slices = _handler->tissue_slices(_handler->active_tissuelayer());
	if (!active_slices)
	{
		size_t dims[3] = {
			_handler->width(),
			_handler->height(),
			_handler->num_slices() };

		return _GetITKView(all_slices, dims, spacing);
	}

	std::vector<tissues_size_t*> slices;
	for (unsigned i = _handler->start_slice(); i < _handler->end_slice(); ++i)
	{
		slices.push_back(all_slices[i]);
	}
	size_t dims[3] = {
		_handler->width(),
		_handler->height(),
		static_cast<size_t>(_handler->end_slice() - _handler->start_slice()) };
	return _GetITKView(slices, dims, spacing);
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
		_handler->height() };
	auto spacing3d = _handler->spacing();
	double spacing[2] = { spacing3d[0], spacing3d[1] };

	auto all_slices = _handler->source_slices();
	slice = (slice == kActiveSlice) ? _handler->active_slice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::Image<float, 2>::Pointer SliceHandlerItkWrapper::GetTargetSlice(int slice)
{
	size_t dims[2] = {
		_handler->width(),
		_handler->height() };
	auto spacing3d = _handler->spacing();
	double spacing[2] = { spacing3d[0], spacing3d[1] };

	auto all_slices = _handler->target_slices();
	slice = (slice == kActiveSlice) ? _handler->active_slice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::Image<tissues_size_t, 2>::Pointer SliceHandlerItkWrapper::GetTissuesSlice(int slice)
{
	size_t dims[2] = {
		_handler->width(),
		_handler->height() };
	auto spacing3d = _handler->spacing();
	double spacing[2] = { spacing3d[0], spacing3d[1] };

	auto all_slices = _handler->tissue_slices(_handler->active_tissuelayer());
	slice = (slice == kActiveSlice) ? _handler->active_slice() : slice;
	return _GetITKView2D(all_slices.at(slice), dims, spacing);
}

itk::ImageRegion<3> SliceHandlerItkWrapper::GetActiveRegion() const
{
	itk::Index<3> start = { 0, 0, _handler->start_slice() };
	itk::Size<3> size = { _handler->width(), _handler->height(), static_cast<size_t>(_handler->end_slice() - _handler->start_slice()) };
	return itk::ImageRegion<3>(start, size);
}

itk::Image<float, 3>::Pointer SliceHandlerItkWrapper::GetImageDeprecated(eImageType type, bool active_slices)
{
	using input_image = itk::SliceContiguousImage<float>;
	using output_image = itk::Image<float, 3>;

	auto cast = itk::CastImageFilter<input_image, output_image>::New();
	cast->SetInput(type==eImageType::kSource ? GetSource(active_slices) : GetTarget(active_slices));
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

}