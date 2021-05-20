/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

class TestHandler : public SlicesHandlerInterface
{
	unsigned short m_Dims[3];
	unsigned short m_Start;
	unsigned short m_End;
	unsigned short m_ActiveSlice = 0;

public:
	TestHandler(unsigned short w, unsigned short h, unsigned short nrslices, unsigned short start = 0, unsigned short end = 0)
	{
		m_Dims[0] = w;
		m_Dims[1] = h;
		m_Dims[2] = nrslices;
		m_Start = start;
		m_End = (end == 0) ? nrslices : end;

		m_FloatData.resize(m_Dims[0] * m_Dims[1] * m_Dims[2], 1.3f);
		m_TissueData.resize(m_Dims[0] * m_Dims[1] * m_Dims[2], tissues_size_t(3));
	}

	std::vector<float> m_FloatData;
	std::vector<tissues_size_t> m_TissueData;
	Transform m_Transform;
	Vec3 m_Spacing;

	unsigned short Width() const override { return m_Dims[0]; }
	unsigned short Height() const override { return m_Dims[1]; }
	unsigned short NumSlices() const override { return m_Dims[2]; }
	unsigned short StartSlice() const override { return m_Start; }
	unsigned short EndSlice() const override { return m_End; }

	unsigned short ActiveSlice() const override { return m_ActiveSlice; }
	void SetActiveSlice(unsigned short slice, bool signal_change) override { m_ActiveSlice = slice; }

	Transform ImageTransform() const override { return m_Transform; }
	Vec3 Spacing() const override { return m_Spacing; }

	tissuelayers_size_t ActiveTissuelayer() const override { return 0; }

	std::vector<const tissues_size_t*> TissueSlices(tissuelayers_size_t layeridx) const override
	{
		std::vector<const tissues_size_t*> d(m_Dims[2], nullptr);
		for (size_t i = 0; i < m_Dims[2]; ++i)
		{
			d[i] = m_TissueData.data() + i * m_Dims[0] * m_Dims[1];
		}
		return d;
	}

	std::vector<tissues_size_t*> TissueSlices(tissuelayers_size_t layeridx) override
	{
		std::vector<tissues_size_t*> d(m_Dims[2], nullptr);
		for (size_t i = 0; i < m_Dims[2]; ++i)
		{
			d[i] = m_TissueData.data() + i * m_Dims[0] * m_Dims[1];
		}
		return d;
	}

	std::vector<const float*> SourceSlices() const override
	{
		std::vector<const float*> d(m_Dims[2], nullptr);
		for (size_t i = 0; i < m_Dims[2]; ++i)
		{
			d[i] = m_FloatData.data() + i * m_Dims[0] * m_Dims[1];
		}
		return d;
	}

	std::vector<float*> SourceSlices() override
	{
		std::vector<float*> d(m_Dims[2], nullptr);
		for (size_t i = 0; i < m_Dims[2]; ++i)
		{
			d[i] = m_FloatData.data() + i * m_Dims[0] * m_Dims[1];
		}
		return d;
	}

	std::vector<const float*> TargetSlices() const override
	{
		return SourceSlices();
	}

	std::vector<float*> TargetSlices() override
	{
		return SourceSlices();
	}

	std::vector<std::string> TissueNames() const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::vector<bool> TissueLocks() const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::vector<tissues_size_t> TissueSelection() const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void SetTissueSelection(const std::vector<tissues_size_t>& sel) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool HasColors() const override
	{
		return false;
	}

	size_t NumberOfColors() const override
	{
		return 0;
	}

	void GetColor(size_t, unsigned char& r, unsigned char& g, unsigned char& b) const override
	{
		throw std::logic_error("No colors available.");
	}

	void SetTargetFixedRange(bool on) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
};

class TestIO
{
public:
	TestIO(const std::string& fname, bool non_trivial_transform, float tolerance = 1e-2f)
			: m_FileName(fname), m_Tolerance(tolerance)
	{
		m_Dims[0] = 4;
		m_Dims[1] = 7;
		m_Dims[2] = 3;

		m_Spacing[0] = 0.5;
		m_Spacing[1] = 1.0;
		m_Spacing[2] = 2.0;

		m_Transform.SetIdentity();
		if (non_trivial_transform)
		{
			Vec3 d0(0.1f, 0.9f, 0.1f);
			d0.Normalize();
			Vec3 d1(0.9f, -0.1f, 0.1f);
			d1.Normalize();
			Vec3 d2(d0 ^ d1);
			d2.Normalize();
			m_Transform.SetRotation(d0, d1, d2);

			float offset[3] = {3.4f, -10.f, 2.5f};
			m_Transform.SetOffset(offset);
		}

		m_Data.resize(m_Dims[0] * m_Dims[1] * m_Dims[2], 0);
		for (size_t i = 0; i < m_Data.size(); i++)
		{
			m_Data[i] = static_cast<float>(i);
		}

		boost::system::error_code ec;
		if (boost::filesystem::exists(m_FileName, ec))
		{
			boost::filesystem::remove(m_FileName, ec);
		}
	}
	~TestIO()
	{
		boost::system::error_code ec;
		if (boost::filesystem::exists(m_FileName, ec))
		{
			boost::filesystem::remove(m_FileName, ec);
		}
	}

	void Write()
	{
		TestHandler handler(m_Dims[0], m_Dims[1], m_Dims[2]);
		handler.m_FloatData = m_Data;
		handler.m_Spacing = m_Spacing;
		handler.m_Transform = m_Transform;

		auto active_slices = ImageWriter::eSliceSelection::kActiveSlices;
		BOOST_REQUIRE(ImageWriter().WriteVolume(m_FileName.string(), handler.TargetSlices(), active_slices, &handler));
	}

	void Read()
	{
		unsigned width, height, nrslices;
		float spacing1[3];
		Transform transform1;
		BOOST_REQUIRE(ImageReader::GetInfo(m_FileName.string().c_str(), width, height, nrslices, spacing1, transform1));

		BOOST_CHECK_EQUAL(width, m_Dims[0]);
		BOOST_CHECK_EQUAL(height, m_Dims[1]);
		BOOST_CHECK_EQUAL(nrslices, m_Dims[2]);

		BOOST_CHECK_CLOSE(spacing1[0], m_Spacing[0], m_Tolerance);
		BOOST_CHECK_CLOSE(spacing1[1], m_Spacing[1], m_Tolerance);
		BOOST_CHECK_CLOSE(spacing1[2], m_Spacing[2], m_Tolerance);

		for (int r = 0; r < 4; r++)
		{
			float* tr = m_Transform[r];
			float* tr1 = transform1[r];
			BOOST_CHECK_CLOSE(tr[0], tr1[0], m_Tolerance);
			BOOST_CHECK_CLOSE(tr[1], tr1[1], m_Tolerance);
			BOOST_CHECK_CLOSE(tr[2], tr1[2], m_Tolerance);
			BOOST_CHECK_CLOSE(tr[2], tr1[2], m_Tolerance);
		}

		{
			std::vector<float> data(m_Data.size());
			std::vector<float*> slices(m_Dims[2]);
			for (unsigned s = 0; s < m_Dims[2]; s++)
			{
				slices[s] = data.data() + s * m_Dims[0] * m_Dims[1];
			}
			BOOST_REQUIRE(ImageReader::GetVolume(m_FileName.string().c_str(), slices.data(), 0, nrslices, width, height));

			bool ok = true;
			for (size_t i = 0; i < data.size(); i++)
			{
				if (m_Data[i] != data[i])
				{
					ok = false;
				}
			}
			BOOST_CHECK_MESSAGE(ok, "getVolume image pixels differ");
		}

		{
			std::vector<float> data(m_Dims[0] * m_Dims[1]);
			BOOST_REQUIRE(ImageReader::GetSlice(m_FileName.string().c_str(), data.data(), 2, width, height));

			bool ok = true;
			size_t shift = 2 * m_Dims[0] * m_Dims[1];
			for (size_t i = 0; i < data.size(); i++)
			{
				if (m_Data[i + shift] != data[i])
				{
					ok = false;
				}
			}
			BOOST_CHECK_MESSAGE(ok, "getSlice image pixels differ");
		}
	}

private:
	boost::filesystem::path m_FileName;
	std::vector<float> m_Data;
	unsigned m_Dims[3];
	Vec3 m_Spacing;
	Transform m_Transform;
	float m_Tolerance;
};

class TestColorImageIO
{
public:
	void Test(const std::string& file_path)
	{
		auto img = WriteRGBImage(file_path);

		unsigned w, h;
		ImageReader::GetInfo2D(file_path.c_str(), w, h);

		BOOST_REQUIRE_EQUAL(w, img->GetBufferedRegion().GetSize()[0]);
		BOOST_REQUIRE_EQUAL(h, img->GetBufferedRegion().GetSize()[1]);

		std::vector<float> data(w * h, 0);
		std::vector<float*> stack(1, data.data());
		std::vector<const char*> files(1, file_path.c_str());
		BOOST_REQUIRE(ImageReader::GetImageStack(files, stack.data(), w, h, [](unsigned char, unsigned char g, unsigned char) { return static_cast<float>(g); }));

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

// --run_test=iSeg_suite/ImageIO_suite/PNG --log_level=message
//BOOST_AUTO_TEST_CASE(PNG)
//{
//	TestIO test("temp.png", true);
//	test.Write();
//}

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
