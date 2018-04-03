/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "RTDoseReader.h"

#include "gdcmAttribute.h"
#include "gdcmImageReader.h"

#pragma warning(disable : 4244)

using namespace iseg;

RTDoseReader::RTDoseReader() {}

RTDoseReader::~RTDoseReader() {}

bool RTDoseReader::ReadSizeData(const char* filename, unsigned int* dims,
								double* spacing, double* origin, double* dc)
{
	gdcm::ImageReader imreader;
	imreader.SetFileName(filename);
	if (!imreader.Read())
	{
		return false;
	}
	gdcm::Image image = imreader.GetImage();

	const unsigned int* tmp0 = image.GetDimensions();
	const double* tmp1 = image.GetSpacing();
	const double* tmp2 = image.GetOrigin();
	const double* tmp3 = image.GetDirectionCosines();
	for (unsigned short i = 0; i < 3; ++i)
	{
		dims[i] = tmp0[i];
		spacing[i] = tmp1[i];
		origin[i] = tmp2[i];
		dc[i] = tmp3[i];
		dc[i + 3] = tmp3[i + 3];
	}

	return true;
}

bool RTDoseReader::ReadPixelData(const char* filename, float** bits)
{
	// Read RTdose data
	gdcm::Reader* reader = new gdcm::Reader();
	reader->SetFileName(filename);
	if (!reader->Read())
	{
		delete reader;
		return false;
	}

	gdcm::MediaStorage ms;
	ms.SetFromFile(reader->GetFile());
	if (ms != gdcm::MediaStorage::RTDoseStorage)
	{
		delete reader;
		return false;
	}

	const gdcm::DataSet& ds = reader->GetFile().GetDataSet();

	// (0028,0009) SQ (Sequence with explicit length #=1)      # Frame increment pointer // TODO: Should be equal to 3004000C (Grid Frame Offset Vector), ignore???
	gdcm::Attribute<0x0028, 0x0009> atframeincrptr;
	atframeincrptr.SetFromDataElement(
		ds.GetDataElement(atframeincrptr.GetTag()));
	if (atframeincrptr.GetValue() != gdcm::Tag(0x3004, 0x000C))
	{
		delete reader;
		return false;
	}

	// (0028,0100) SQ (Sequence with explicit length #=1)      # Bits Allocated (16 or 32 for RTdose)
	gdcm::Attribute<0x0028, 0x0100> atbitsppx;
	const gdcm::DataElement& debitsppx = ds.GetDataElement(atbitsppx.GetTag());
	atbitsppx.SetFromDataElement(debitsppx);
	unsigned short bitsppx = atbitsppx.GetValue();
	if (!(bitsppx == 16 || bitsppx == 32))
	{
		delete reader;
		return false;
	}

	// (3004,000C)										       # Grid Frame Offset Vector (array which contains the dose image plane offsets (in mm) of the dose image frames)
	gdcm::Attribute<0x3004, 0x000C> atoffsetvec;
	atoffsetvec.SetFromDataElement(ds.GetDataElement(atoffsetvec.GetTag()));
	const double* offsetvals = atoffsetvec.GetValues();
	if (offsetvals[0] != 0.0)
	{
		// TODO: Plane locations in patient coordinate system not handled
		delete reader;
		return false;
	}

	// (3004,0002) SQ (Sequence with explicit length #=1)      # Dose Units (GY = Gray or RELATIVE = dose relative to implicit reference value for RT images)
	gdcm::Attribute<0x3004, 0x0002> atdoseunits;
	atdoseunits.SetFromDataElement(ds.GetDataElement(atdoseunits.GetTag()));
	double gridscaling = 1.0;
	if (atdoseunits.GetValue().compare("GY") == 0)
	{
		// (3004,000E) SQ (Sequence with explicit length #=1)      # Dose Grid Scaling
		// Scaling factor that when multiplied by the pixel data yields grid doses in the dose units
		gdcm::Attribute<0x3004, 0x000E> atgridscaling;
		atgridscaling.SetFromDataElement(
			ds.GetDataElement(atgridscaling.GetTag()));
		gridscaling = atgridscaling.GetValue();
	}

	// Ignored attributes (incomplete)
	// (3004,0010) SQ (Sequence with explicit length #=4)      # RTDoseROISequence (RTdose data in form of isodose contours not yet defined in standard PS 3.3-2011)
	// (3006,0039) SQ (Sequence with explicit length #=4)      # ROIContourSequence (RTdose data in form of isodose contours not yet defined in standard PS 3.3-2011)
	// (0028,0002) SQ (Sequence with explicit length #=1)      # Samples per Pixel (1 for RTdose)
	// (0028,0101) SQ (Sequence with explicit length #=1)      # Bits Stored (equal to Bits Allocated for RTdose)
	// (0028,0102) SQ (Sequence with explicit length #=1)      # High Bit (Bits Stored - 1 for RTdose)
	// (0028,0103) SQ (Sequence with explicit length #=1)      # Pixel Representation (unsigned integer for RT images)
	// (3004,0006)										       # Dose Comment
	// (0028,0103) SQ (Sequence with explicit length #=1)      # Dose Summation Type

	delete reader;

	// Read pixel data
	gdcm::ImageReader* imreader = new gdcm::ImageReader();
	imreader->SetFileName(filename);
	if (!imreader->Read())
	{
		delete imreader;
		return false;
	}
	gdcm::Image image = imreader->GetImage();
	//if( image.GetPhotometricInterpretation() != gdcm::PhotometricInterpretation::MONOCHROME2 ) { // MONOCHROME2 for RTdose by standard
	//	delete imreader;
	//	return false;
	//}

	const unsigned int* dims = image.GetDimensions();
	const double* spacing = image.GetSpacing();
	const double* origin = image.GetOrigin();

	for (unsigned int i = 0; i < dims[2]; ++i)
	{
		if (offsetvals[i] != i * spacing[2])
		{
			// TODO: Non-equidistant z spacing not handled
			delete imreader;
			return false;
		}
	}

	std::vector<char> vbuffer;
	vbuffer.resize(image.GetBufferLength());
	char* buffer = &vbuffer[0];
	image.GetBuffer(buffer);

	if (image.GetPixelFormat() == gdcm::PixelFormat::UINT16)
	{
		unsigned short* buffer16 = (unsigned short*)buffer;
		for (unsigned short z = 0; z < dims[2]; ++z)
		{
			float* tmp = &(bits[z][0]);
			for (unsigned short y = 0; y < dims[1]; ++y)
			{
				for (unsigned short x = 0; x < dims[0]; ++x)
				{
					*tmp++ = *buffer16++ * gridscaling;
				}
			}
		}
	}
	else if (image.GetPixelFormat() == gdcm::PixelFormat::UINT32)
	{
		unsigned int* buffer32 = (unsigned int*)buffer;
		for (unsigned short z = 0; z < dims[2]; ++z)
		{
			float* tmp = &(bits[z][0]);
			for (unsigned short y = 0; y < dims[1]; ++y)
			{
				for (unsigned short x = 0; x < dims[0]; ++x)
				{
					*tmp++ = *buffer32++ * gridscaling;
				}
			}
		}
	}
	else
	{
		delete imreader;
		return false;
	}

	delete imreader;
	return true;
}
