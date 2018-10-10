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

#include "Data/SlicesHandlerInterface.h"
#include "Data/Transform.h"
#include "Data/Vec3.h"

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkRGBPixel.h>

#include <boost/filesystem.hpp>

#include <string>

namespace iseg {

namespace {

class TestHandler : public SliceHandlerInterface
{
	unsigned short _dims[3];
	unsigned short _start;
	unsigned short _end;
	unsigned short _active_slice = 0;

public:
	TestHandler(unsigned short w, unsigned short h, unsigned short nrslices, unsigned short start = 0, unsigned short end = 0)
	{
		_dims[0] = w;
		_dims[1] = h;
		_dims[2] = nrslices;
		_start = start;
		_end = (end == 0) ? nrslices : end;

		_float_data.resize(_dims[0] * _dims[1] * _dims[2], 1.3f);
		_tissue_data.resize(_dims[0] * _dims[1] * _dims[2], tissues_size_t(3));
	}

	std::vector<float> _float_data;
	std::vector<tissues_size_t> _tissue_data;
	Transform _transform;
	Vec3 _spacing;

	unsigned short width() const override { return _dims[0]; }
	unsigned short height() const override { return _dims[1]; }
	unsigned short num_slices() const override { return _dims[2]; }
	unsigned short start_slice() const override { return _start; }
	unsigned short end_slice() const override { return _end; }

	unsigned short active_slice() const override { return _active_slice; }
	void set_active_slice(unsigned short slice, bool signal_change) override { _active_slice = slice; }

	Transform transform() const override { return _transform; }
	Vec3 spacing() const override { return _spacing; }

	tissuelayers_size_t active_tissuelayer() const override { return 0; }

	std::vector<const tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) const override
	{
		std::vector<const tissues_size_t*> d(_dims[2], nullptr);
		for (size_t i = 0; i < _dims[2]; ++i)
		{
			d[i] = _tissue_data.data() + i * _dims[0] * _dims[1];
		}
		return d;
	}

	std::vector<tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) override
	{
		std::vector<tissues_size_t*> d(_dims[2], nullptr);
		for (size_t i = 0; i < _dims[2]; ++i)
		{
			d[i] = _tissue_data.data() + i * _dims[0] * _dims[1];
		}
		return d;
	}

	std::vector<const float*> source_slices() const override
	{
		std::vector<const float*> d(_dims[2], nullptr);
		for (size_t i = 0; i < _dims[2]; ++i)
		{
			d[i] = _float_data.data() + i * _dims[0] * _dims[1];
		}
		return d;
	}

	std::vector<float*> source_slices() override
	{
		std::vector<float*> d(_dims[2], nullptr);
		for (size_t i = 0; i < _dims[2]; ++i)
		{
			d[i] = _float_data.data() + i * _dims[0] * _dims[1];
		}
		return d;
	}

	std::vector<const float*> target_slices() const override
	{
		return source_slices();
	}

	std::vector<float*> target_slices() override
	{
		return source_slices();
	}

	std::vector<std::string> tissue_names() const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::vector<bool> tissue_locks() const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::vector<tissues_size_t> tissue_selection() const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void set_tissue_selection(const std::vector<tissues_size_t>& sel) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool has_colors() const override
	{
		return false;
	}

	size_t number_of_colors() const override
	{
		return 0;
	}

	void get_color(size_t, unsigned char& r, unsigned char& g, unsigned char& b) const override
	{
		throw std::logic_error("No colors available.");
	}

	virtual void set_target_fixed_range(bool on) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
};

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
		TestHandler handler(_dims[0], _dims[1], _dims[2]);
		handler._float_data = _data;
		handler._spacing = _spacing;
		handler._transform = _transform;

		bool active_slices = false;
		BOOST_REQUIRE(ImageWriter().writeVolume(_file_name.string(), handler.target_slices(), active_slices, &handler));
	}

	void Read()
	{
		unsigned width, height, nrslices;
		float spacing1[3];
		Transform transform1;
		BOOST_REQUIRE(ImageReader::getInfo(_file_name.string().c_str(), width, height, nrslices, spacing1, transform1));

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
					slices.data(), 0, nrslices, width, height));

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
	Vec3 _spacing;
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

		std::vector<float> data(w * h, 0);
		std::vector<float*> stack(1, data.data());
		std::vector<const char*> files(1, file_path.c_str());
		BOOST_REQUIRE(ImageReader::getImageStack(files, stack.data(), w, h,
				[](unsigned char, unsigned char g, unsigned char) { return static_cast<float>(g); }));

		itk::Index<2> idx = {0, 0};
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

// --run_test=iSeg_suite/ImageIO_suite/Metaheader --log_level=message
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
