/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "Data/Transform.h"

#include "ImageReader.h"
#include "VTIreader.h"

#include "itkDICOMOrientImageFilter.h"

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkRGBPixel.h>
#include <itkUnaryFunctorImageFilter.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace iseg {

namespace {
template<typename TComponent, typename TOutput>
class RGBToValue
{
public:
	RGBToValue() = default;
	~RGBToValue() = default;

	std::function<float(unsigned char, unsigned char, unsigned char)> m_Functor;

	bool operator==(const RGBToValue& other) const
	{
		return (&m_Functor == &other.m_Functor);
	}
	bool operator!=(const RGBToValue& other) const
	{
		return !(*this == other);
	}

	inline TOutput operator()(const itk::RGBPixel<TComponent>& A) const
	{
		return static_cast<TOutput>(m_Functor(A[0], A[1], A[2]));
	}
};
} // namespace

bool ImageReader::GetInfo2D(const std::string& filename, unsigned& width, unsigned& height)
{
	unsigned nrslices;
	float spacing[3];
	Transform tr;
	return ImageReader::GetInfo(filename, width, height, nrslices, spacing, tr);
}

bool ImageReader::GetImageStack(const std::vector<std::string>& filenames, float** img_stack, unsigned width, unsigned height, const std::function<float(unsigned char, unsigned char, unsigned char)>& color2grey)
{
	using rgbpixel_type = itk::RGBPixel<unsigned char>;
	using input_image_type = itk::Image<rgbpixel_type, 3>;
	using output_image_type = itk::Image<float, 3>;

	using reader_type = itk::ImageFileReader<input_image_type>;
	using functor_type = RGBToValue<unsigned char, float>;
	using rgbmapper_type = itk::UnaryFunctorImageFilter<input_image_type, output_image_type, functor_type>;

	auto reader = reader_type::New();

	functor_type functor;
	functor.m_Functor = color2grey;

	auto mapper = rgbmapper_type::New();
	mapper->SetInput(reader->GetOutput());
	mapper->SetFunctor(functor);

	for (size_t i = 0, iend = filenames.size(); i < iend; ++i)
	{
		reader->SetFileName(filenames[i]);

		try
		{
			mapper->Update();
		}
		catch (itk::ExceptionObject& e)
		{
			ISEG_ERROR("an exception occurred " << e.what());
			return false;
		}

		auto container = mapper->GetOutput()->GetPixelContainer();
		size_t size = static_cast<size_t>(width) * height;

		if (size != container->Size())
		{
			return false;
		}
		auto buffer = container->GetImportPointer();
		std::copy(buffer, buffer + size, img_stack[i]);
	}
	return true;
}

bool ImageReader::GetSlice(const std::string& filename, float* slice, unsigned slicenr, unsigned width, unsigned height)
{
	return GetVolume(filename, &slice, slicenr, 1, width, height);
}

float* ImageReader::GetSliceInfo(const std::string& filename, unsigned slicenr, unsigned& width, unsigned& height)
{
	unsigned nrslices;
	float spacing[3];
	Transform tr;
	if (GetInfo(filename, width, height, nrslices, spacing, tr))
	{
		float* slice = new float[height * (unsigned long)width];
		if (GetSlice(filename, slice, slicenr, width, height))
		{
			return slice;
		}
		delete[] slice;
	}
	return nullptr;
}

bool ImageReader::GetVolume(const std::string& filename, float** slices, unsigned nrslices, unsigned width, unsigned height)
{
	return GetVolume(filename, slices, 0, nrslices, width, height);
}

bool ImageReader::GetVolume(const std::string& filename, float** slices, unsigned startslice, unsigned nrslices, unsigned width, unsigned height)
{
	// ITK does not know how to load VTI
	boost::filesystem::path path(filename);
	std::string extension = boost::algorithm::to_lower_copy(path.has_extension() ? path.extension().string() : "");
	if (extension == ".vti")
	{
		unsigned w, h, n;
		float s[3], o[3];
		std::vector<std::string> array_names;
		if (VTIreader::GetInfo(filename, w, h, n, s, o, array_names) &&
				!array_names.empty())
		{
			return VTIreader::GetVolume(filename, slices, startslice, nrslices, width, height, array_names[0]);
		}
	}

	using image_type = itk::Image<float, 3>;
	using reader_type = itk::ImageFileReader<image_type>;

	auto reader = reader_type::New();
	reader->SetFileName(filename);
	try
	{
		reader->UpdateLargestPossibleRegion();
	}
	catch (itk::ExceptionObject&)
	{
		return false;
	}

	auto image = reader->GetOutput();
	auto container = image->GetPixelContainer();
	image_type::PixelType* buffer = container->GetImportPointer();

	for (unsigned k = 0; k < nrslices; k++)
	{
		size_t pos3d = (k + startslice) * width * height, pos = 0;
		for (unsigned j = 0; j < height; j++)
		{
			for (unsigned i = 0; i < width; i++, pos++, pos3d++)
			{
				slices[k][pos] = buffer[pos3d];
			}
		}
	}
	return true;
}

bool ImageReader::GetInfo(const std::string& filename, unsigned& width, unsigned& height, unsigned& nrslices, float spacing[3], Transform& transform)
{
	auto image_io = itk::ImageIOFactory::CreateImageIO(filename.c_str(), itk::ImageIOFactory::ReadMode);
	if (image_io)
	{
		image_io->SetFileName(filename);
		image_io->ReadImageInformation();
		unsigned int n = image_io->GetNumberOfDimensions();

		width = static_cast<unsigned>(image_io->GetDimensions(0));
		height = static_cast<unsigned>(image_io->GetDimensions(1));
		nrslices = (n == 3) ? static_cast<unsigned>(image_io->GetDimensions(2)) : 1;

		transform.SetIdentity();
		for (unsigned int r = 0; r < n; r++)
		{
			spacing[r] = static_cast<float>(image_io->GetSpacing(r));

			for (unsigned int c = 0; c < n; c++)
				transform[r][c] = image_io->GetDirection(c)[r];

			transform[r][3] = image_io->GetOrigin(r);
		}

		return true;
	}
	else
	{
		boost::filesystem::path path(filename);
		std::string extension = boost::algorithm::to_lower_copy(path.has_extension() ? path.extension().string() : "");
		if (extension == ".vti")
		{
			float offset[3];
			std::vector<std::string> array_names;
			VTIreader::GetInfo(filename, width, height, nrslices, spacing, offset, array_names);

			transform.SetIdentity();
			transform.SetOffset(offset);
			return true;
		}
	}
	return false;
}

bool ImageReader::GetVolume(const std::string& filename, std::vector<float>& buffer, unsigned& width, unsigned& height, unsigned& nrslices, float spacing[3], Transform& transform, eOrientation orientation)
{
	using image_type = itk::Image<float, 3>;
	using reader_type = itk::ImageFileReader<image_type>;

	auto reader = reader_type::New();
	reader->SetFileName(filename);
	try
	{
		reader->UpdateLargestPossibleRegion();
	}
	catch (itk::ExceptionObject&)
	{
		return false;
	}

	const unsigned int num_dim = (reader->GetImageIO() != nullptr) ? reader->GetImageIO()->GetNumberOfDimensions() : 3;
	typename reader_type::OutputImagePointer image = reader->GetOutput();

	auto orient = itk::DICOMOrientImageFilter<image_type>::New();
	if (orientation != eOrientation::noChange && num_dim == 3)
	{
		switch (orientation)
		{
		case eOrientation::RAS:
			orient->SetDesiredCoordinateOrientation(itk::DICOMOrientation::OrientationEnum::RAS);
			break;
		case eOrientation::LPS:
			orient->SetDesiredCoordinateOrientation(itk::DICOMOrientation::OrientationEnum::LPS);
			break;
		case eOrientation::LAS:
			orient->SetDesiredCoordinateOrientation(itk::DICOMOrientation::OrientationEnum::LAS);
			break;
		case eOrientation::PSL:
			orient->SetDesiredCoordinateOrientation(itk::DICOMOrientation::OrientationEnum::PSL);
			break;
		case eOrientation::RSP:
			orient->SetDesiredCoordinateOrientation(itk::DICOMOrientation::OrientationEnum::RSP);
			break;
		default:
			orient->SetDesiredCoordinateOrientation(itk::DICOMOrientation::OrientationEnum::RAS);
			break;
		}

		orient->SetInput(reader->GetOutput());
		orient->UpdateLargestPossibleRegion();
		image = orient->GetOutput();
	}

	// copy dimension
	const auto region = image->GetLargestPossibleRegion();
	const itk::SizeValueType dims[3] = {region.GetSize()[0], region.GetSize()[1], region.GetSize()[2]};

	width = static_cast<unsigned>(dims[0]);
	height = static_cast<unsigned>(dims[1]);
	nrslices = (num_dim == 3) ? static_cast<unsigned>(dims[2]) : 1;

	// copy transform & spacing
	transform.SetIdentity();
	for (unsigned int r = 0; r < num_dim; r++)
	{
		spacing[r] = static_cast<float>(image->GetSpacing()[r]);
		for (unsigned int c = 0; c < num_dim; c++)
			transform[r][c] = image->GetDirection()[r][c];
		transform[r][3] = image->GetOrigin()[r];
	}

	// copy buffer
	auto container = image->GetPixelContainer();
	const image_type::PixelType* import_buffer = container->GetImportPointer();
	const auto buffer_length = region.GetNumberOfPixels();
	buffer.assign(import_buffer, import_buffer + buffer_length);

	return true;
}

bool ImageReader::CopySlices(const std::vector<float>& buffer, float** slices, unsigned startslice, unsigned nrslices, unsigned width, unsigned height)
{
	const size_t required = static_cast<size_t>(startslice + nrslices) * width * height;
	if (buffer.size() < required)
	{
		return false;
	}

	for (size_t k = 0; k < nrslices; k++)
	{
		size_t pos3d = (k + startslice) * width * height, pos = 0;
		for (unsigned j = 0; j < height; j++)
		{
			for (unsigned i = 0; i < width; i++, pos++, pos3d++)
			{
				slices[k][pos] = buffer[pos3d];
			}
		}
	}
	return true;
}

} // namespace iseg
