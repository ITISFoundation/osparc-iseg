/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <boost/test/unit_test.hpp>

#include "../ImageReader.h"
#include "../ImageWriter.h"

#include "Data/Transform.h"
#include "Data/Vec3.h"

#include <itkImage.h>
#include <itkRGBPixel.h>
#include <itkImageFileWriter.h>

#include <boost/filesystem.hpp>

#include <string>

namespace iseg {

namespace {
class TestIO
{
public:
	TestIO(const std::string& fname, bool non_trivial_transform,
			float tolerance = 1e-2f)
			: _file_name(fname), _tolerance(tolerance)
	{
		_dims[0] = 4;
		_dims[1] = 7;
		_dims[2] = 3;

		_spacing[0] = 0.5;
		_spacing[1] = 1.0;
		_spacing[2] = 2.0;

		_transform.setIdentity();
		if (non_trivial_transform)
		{
			Vec3 d0(0.1f, 0.9f, 0.1f);
			d0.normalize();
			Vec3 d1(0.9f, -0.1f, 0.1f);
			d1.normalize();
			Vec3 d2(d0 ^ d1);
			d2.normalize();
			_transform.setRotation(d0, d1, d2);

			float offset[3] = {3.4f, -10.f, 2.5f};
			_transform.setOffset(offset);
		}

		_data.resize(_dims[0] * _dims[1] * _dims[2], 0);
		for (size_t i = 0; i < _data.size(); i++)
		{
			_data[i] = static_cast<float>(i);
		}

		boost::system::error_code ec;
		if (boost::filesystem::exists(_file_name, ec))
		{
			boost::filesystem::remove(_file_name, ec);
		}
	}
	~TestIO()
	{
		boost::system::error_code ec;
		if (boost::filesystem::exists(_file_name, ec))
		{
			boost::filesystem::remove(_file_name, ec);
		}
	}

	void Write()
	{
		std::vector<const float*> slices(_dims[2]);
		for (unsigned s = 0; s < _dims[2]; s++)
		{
			slices[s] = _data.data() + s * _dims[0] * _dims[1];
		}

		BOOST_REQUIRE(ImageWriter().writeVolume(_file_name.string().c_str(),
				slices.data(), _dims[0], _dims[1],
				_dims[2], _spacing, _transform));
	}

	void Read()
	{
		unsigned width, height, nrslices;
		float spacing1[3];
		Transform transform1;
		BOOST_REQUIRE(ImageReader::getInfo(_file_name.string().c_str(), width,
				height, nrslices, spacing1, transform1));

		BOOST_CHECK_EQUAL(width, _dims[0]);
		BOOST_CHECK_EQUAL(height, _dims[1]);
		BOOST_CHECK_EQUAL(nrslices, _dims[2]);

		BOOST_CHECK_CLOSE(spacing1[0], _spacing[0], _tolerance);
		BOOST_CHECK_CLOSE(spacing1[1], _spacing[1], _tolerance);
		BOOST_CHECK_CLOSE(spacing1[2], _spacing[2], _tolerance);

		for (int r = 0; r < 4; r++)
		{
			float* tr = _transform[r];
			float* tr1 = transform1[r];
			BOOST_CHECK_CLOSE(tr[0], tr1[0], _tolerance);
			BOOST_CHECK_CLOSE(tr[1], tr1[1], _tolerance);
			BOOST_CHECK_CLOSE(tr[2], tr1[2], _tolerance);
			BOOST_CHECK_CLOSE(tr[2], tr1[2], _tolerance);
		}

		{
			std::vector<float> data(_data.size());
			std::vector<float*> slices(_dims[2]);
			for (unsigned s = 0; s < _dims[2]; s++)
			{
				slices[s] = data.data() + s * _dims[0] * _dims[1];
			}
			BOOST_REQUIRE(ImageReader::getVolume(_file_name.string().c_str(),
					slices.data(), 0, nrslices, width,
					height));

			bool ok = true;
			for (size_t i = 0; i < data.size(); i++)
			{
				if (_data[i] != data[i])
				{
					ok = false;
				}
			}
			BOOST_CHECK_MESSAGE(ok, "getVolume image pixels differ");
		}

		{
			std::vector<float> data(_dims[0] * _dims[1]);
			BOOST_REQUIRE(ImageReader::getSlice(_file_name.string().c_str(),
					data.data(), 2, width, height));

			bool ok = true;
			size_t shift = 2 * _dims[0] * _dims[1];
			for (size_t i = 0; i < data.size(); i++)
			{
				if (_data[i + shift] != data[i])
				{
					ok = false;
				}
			}
			BOOST_CHECK_MESSAGE(ok, "getSlice image pixels differ");
		}
	}

private:
	boost::filesystem::path _file_name;
	std::vector<float> _data;
	unsigned _dims[3];
	float _spacing[3];
	Transform _transform;
	float _tolerance;
};

class TestColorImageIO
{
public:
	void Test(const std::string& file_path)
	{
		auto img = WriteRGBImage(file_path);

		unsigned w, h;
		ImageReader::getInfo2D(file_path.c_str(), w, h);

		BOOST_REQUIRE_EQUAL(w, img->GetBufferedRegion().GetSize()[0]);
		BOOST_REQUIRE_EQUAL(h, img->GetBufferedRegion().GetSize()[1]);

		std::vector<float> data(w*h, 0);
		std::vector<float*> stack(1, data.data());
		std::vector<const char*> files(1, file_path.c_str());
		BOOST_REQUIRE(ImageReader::getImageStack(files, stack.data(), w, h,
			[](unsigned char, unsigned char g, unsigned char) { return static_cast<float>(g); }));

		itk::Index<2> idx = { 0,0 };
		BOOST_CHECK_EQUAL(data[0], img->GetPixel(idx)[1]);

		boost::system::error_code ec;
		if (boost::filesystem::exists(file_path, ec))
		{
			boost::filesystem::remove(file_path, ec);
		}
	}

private:
	itk::Image<itk::RGBPixel<unsigned char>, 2>::Pointer WriteRGBImage(const std::string& file_path)
	{
		using rgb_type = itk::RGBPixel<unsigned char>;
		using rgb_image_type = itk::Image<rgb_type, 2>;

		rgb_type val;
		val[0] = 1;
		val[1] = 234;
		val[2] = 171;

		rgb_image_type::SizeType size;
		size[0] = 32;
		size[1] = 16;

		rgb_image_type::IndexType start;
		start.Fill(0);

		auto image = rgb_image_type::New();
		image->SetRegions(rgb_image_type::RegionType(start, size));
		image->Allocate();
		image->FillBuffer(val);

		auto writer = itk::ImageFileWriter<rgb_image_type>::New();
		writer->SetInput(image);
		writer->SetFileName(file_path);
		BOOST_CHECK_NO_THROW(writer->Update());

		return image;
	}
};

} // namespace

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(ImageIO_suite);

BOOST_AUTO_TEST_CASE(Metaheader)
{
	TestIO test("temp.mhd", true);
	test.Write();
	test.Read();
}

BOOST_AUTO_TEST_CASE(Nifti)
{
	// BL TODO why is this SOO bad at serializing the transform?
	TestIO test("temp.nii.gz", true, 10.f);
	test.Write();
	test.Read();
}

BOOST_AUTO_TEST_CASE(Nrrd)
{
	TestIO test("temp.nrrd", true);
	test.Write();
	test.Read();
}

BOOST_AUTO_TEST_CASE(VTK)
{
	TestIO test("temp.vtk", false);
	test.Write();
	test.Read();
}

BOOST_AUTO_TEST_CASE(VTI)
{
	TestIO test("temp.vti", false);
	test.Write();
	test.Read();
}

// TestRunner.exe --run_test=iSeg_suite/ImageIO_suite/ColorImages --log_level=message
BOOST_AUTO_TEST_CASE(ColorImages)
{
	TestColorImageIO test;
	test.Test("temp.bmp");

	test.Test("temp.png");

	test.Test("temp.jpg");
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
