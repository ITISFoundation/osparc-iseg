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

#include "RTDoseWriter.h"

#include "gdcmAttribute.h"
#include "gdcmImageWriter.h"
#include "gdcmItem.h"
#include "gdcmSequenceOfItems.h"

#include <sstream>

namespace iseg {

RTDoseWriter::RTDoseWriter()
{
	if (_vrTypeMaxLength.size() <= 0)
	{
		_vrTypeMaxLength[gdcm::VR::AE] = 16;
		_vrTypeMaxLength[gdcm::VR::AS] = 4;
		_vrTypeMaxLength[gdcm::VR::AT] = 4;
		_vrTypeMaxLength[gdcm::VR::CS] = 16;
		_vrTypeMaxLength[gdcm::VR::DA] = 8;
		_vrTypeMaxLength[gdcm::VR::DS] = 16;
		_vrTypeMaxLength[gdcm::VR::DT] = 26;
		_vrTypeMaxLength[gdcm::VR::FL] = 4;
		_vrTypeMaxLength[gdcm::VR::FD] = 16;
		_vrTypeMaxLength[gdcm::VR::IS] = 12;
		_vrTypeMaxLength[gdcm::VR::LO] = 64;
		_vrTypeMaxLength[gdcm::VR::LT] = 10240;
		_vrTypeMaxLength[gdcm::VR::OB] = 1;
		_vrTypeMaxLength[gdcm::VR::OF] = 0;
		_vrTypeMaxLength[gdcm::VR::OW] = 0;
		_vrTypeMaxLength[gdcm::VR::PN] = 64;
		_vrTypeMaxLength[gdcm::VR::SH] = 16;
		_vrTypeMaxLength[gdcm::VR::SL] = 4;
		_vrTypeMaxLength[gdcm::VR::SQ] = 0;
		_vrTypeMaxLength[gdcm::VR::SS] = 2;
		_vrTypeMaxLength[gdcm::VR::ST] = 1024;
		_vrTypeMaxLength[gdcm::VR::TM] = 16;
		_vrTypeMaxLength[gdcm::VR::UI] = 64;
		_vrTypeMaxLength[gdcm::VR::UL] = 4;
		_vrTypeMaxLength[gdcm::VR::UN] = 0;
		_vrTypeMaxLength[gdcm::VR::US] = 2;
		_vrTypeMaxLength[gdcm::VR::UT] = 0;
	}
	if (_attributeTagVRType.size() <= 0)
	{
		_attributeTagVRType["PatientName"] =
			TagVRTypePair(gdcm::Tag(0x0010, 0x0010), gdcm::VR::PN);
		_attributeTagVRType["PatientID"] =
			TagVRTypePair(gdcm::Tag(0x0010, 0x0020), gdcm::VR::LO);
		_attributeTagVRType["PatientBirthDate"] =
			TagVRTypePair(gdcm::Tag(0x0010, 0x0030), gdcm::VR::DA);
		_attributeTagVRType["PatientSex"] =
			TagVRTypePair(gdcm::Tag(0x0010, 0x0040), gdcm::VR::CS);
		_attributeTagVRType["StudyInstanceUID"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x000D), gdcm::VR::UI);
		_attributeTagVRType["StudyDate"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0020), gdcm::VR::DA);
		_attributeTagVRType["StudyTime"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0030), gdcm::VR::TM);
		_attributeTagVRType["ReferringPhysicianName"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0090), gdcm::VR::PN);
		_attributeTagVRType["StudyID"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x0010), gdcm::VR::SH);
		_attributeTagVRType["AccessionNumber"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0050), gdcm::VR::SH);
		_attributeTagVRType["Modality"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0060), gdcm::VR::CS);
		_attributeTagVRType["SeriesInstanceUID"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x000E), gdcm::VR::UI);
		_attributeTagVRType["SeriesNumber"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x0011), gdcm::VR::IS);
		_attributeTagVRType["OperatorsName"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x1070), gdcm::VR::PN);
		_attributeTagVRType["FrameOfReferenceUID"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x0052), gdcm::VR::UI);
		_attributeTagVRType["PositionReferenceIndicator"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x1040), gdcm::VR::LO);
		_attributeTagVRType["Manufacturer"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0070), gdcm::VR::LO);
		_attributeTagVRType["InstanceNumber"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x0013), gdcm::VR::IS);
		_attributeTagVRType["PixelSpacing"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0030), gdcm::VR::DS);
		_attributeTagVRType["ImageOrientationPatient"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x0037), gdcm::VR::DS);
		_attributeTagVRType["ImagePositionPatient"] =
			TagVRTypePair(gdcm::Tag(0x0020, 0x0032), gdcm::VR::DS);
		_attributeTagVRType["SliceThickness"] =
			TagVRTypePair(gdcm::Tag(0x0018, 0x0050), gdcm::VR::DS);
		_attributeTagVRType["SamplesPerPixel"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0002), gdcm::VR::US);
		_attributeTagVRType["PhotometricInterpretation"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0004), gdcm::VR::CS);
		_attributeTagVRType["Rows"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0010), gdcm::VR::US);
		_attributeTagVRType["Columns"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0011), gdcm::VR::US);
		_attributeTagVRType["BitsAllocated"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x00100), gdcm::VR::US);
		_attributeTagVRType["BitsStored"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0101), gdcm::VR::US);
		_attributeTagVRType["HighBit"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0102), gdcm::VR::US);
		_attributeTagVRType["PixelRepresentation"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0103), gdcm::VR::US);
		_attributeTagVRType["PixelData"] = TagVRTypePair(
			gdcm::Tag(0x7fe0, 0x0010), gdcm::VR::OW); // TODO: OW or OB
		_attributeTagVRType["NumberOfFrames"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0008), gdcm::VR::IS);
		_attributeTagVRType["FrameIncrementPointer"] =
			TagVRTypePair(gdcm::Tag(0x0028, 0x0009), gdcm::VR::AT);
		_attributeTagVRType["DoseUnits"] =
			TagVRTypePair(gdcm::Tag(0x3004, 0x0002), gdcm::VR::CS);
		_attributeTagVRType["DoseType"] =
			TagVRTypePair(gdcm::Tag(0x3004, 0x0004), gdcm::VR::CS);
		_attributeTagVRType["DoseSummationType"] =
			TagVRTypePair(gdcm::Tag(0x3004, 0x000A), gdcm::VR::CS);
		_attributeTagVRType["ReferencedRTPlanSequence"] =
			TagVRTypePair(gdcm::Tag(0x300C, 0x0002), gdcm::VR::SQ);
		_attributeTagVRType["ReferencedSOPClassUID"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x1150), gdcm::VR::UI);
		_attributeTagVRType["ReferencedSOPInstanceUID"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x1155), gdcm::VR::UI);
		_attributeTagVRType["GridFrameOffsetVector"] =
			TagVRTypePair(gdcm::Tag(0x3004, 0x000C), gdcm::VR::DS);
		_attributeTagVRType["DoseGridScaling"] =
			TagVRTypePair(gdcm::Tag(0x3004, 0x000E), gdcm::VR::DS);
		_attributeTagVRType["SOPClassUID"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0016), gdcm::VR::UI);
		_attributeTagVRType["SOPInstanceUID"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0018), gdcm::VR::UI);
		_attributeTagVRType["SpecificCharacterSet"] =
			TagVRTypePair(gdcm::Tag(0x0008, 0x0005), gdcm::VR::CS);
	}
}

RTDoseWriter::~RTDoseWriter() {}

bool RTDoseWriter::Write(const char* filename, RTDoseIODModule* rtDose)
{
#if 1
	// TODO: Only supports volumetric image data. Should dose points and isodose curves be supported? (Find reference which states that only volumetric data in use.)
	// TODO: Only mandatory RT Dose IOD modules included
	// TODO: Only single-frame data supported (C.7.6.6 not included)
	// TODO: Does not Frame-Level retrieve request (C.12.3 not included)
	// TODO: Optional and - if possible - conditional data elements ignored

	if (rtDose->GetReferencedRTPlanInstanceUIDCount() <= 0)
	{
		return false; // TODO: Check for different dose summation types
	}

	gdcm::ImageWriter writer;
	gdcm::Image& image = writer.GetImage();
	gdcm::DataSet& dataset = writer.GetFile().GetDataSet();

	unsigned int dimensions[3] = {rtDose->GetRows(), rtDose->GetColumns(),
								  rtDose->GetNumberOfFrames()};
	image.SetNumberOfDimensions(3);
	image.SetDimensions(dimensions);
	double spacing[3] = {rtDose->GetPixelSpacing()[0],
						 rtDose->GetPixelSpacing()[1],
						 rtDose->GetSliceThickness()};
	image.SetSpacing(spacing);
	image.SetDirectionCosines(rtDose->GetImageOrientationPatient());
	image.SetOrigin(rtDose->GetImagePositionPatient());
	image.SetSlope(rtDose->GetSliceThickness());
	/* image.SetIntercept();
	image.SetLossyFlag();
	image.SetLUT();
	image.SetNeedByteSwap();
	image.SetNumberOfCurves();
	image.SetNumberOfOverlays();
	image.SetPlanarConfiguration();
	image.SetSwapCode();
	image.SetTransferSyntax( gdcm::TransferSyntax::ExplicitVRLittleEndian );
	image.SetDataElement();
	gdcm::File &file = writer.GetFile();
	gdcm::FileMetaInformation &metaInfo = file.GetHeader(); */

	gdcm::PixelFormat pf;
	if (rtDose->GetBitsAllocated() == 32)
	{
		pf = gdcm::PixelFormat::UINT32;
	}
	else
	{
		pf = gdcm::PixelFormat::UINT16;
	}
	pf.SetSamplesPerPixel(rtDose->GetSamplesPerPixel());
	image.SetPixelFormat(pf);

	gdcm::PhotometricInterpretation pi =
		gdcm::PhotometricInterpretation::MONOCHROME2;
	image.SetPhotometricInterpretation(pi);

	// Patient Module (C7.1.1)
	// --------------------------------------------------------------

	// Patient's Name (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("PatientName"),
					   rtDose->GetPatientName());

	// Patient ID (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("PatientID"),
					   rtDose->GetPatientID());

	// Issuer of Patient ID Macro (Table 10-18)

	// Patient's Birth Date (Type = 2, VM = 1)
	std::string birthDate;
	unsigned short* birthDatePtr = rtDose->GetPatientBirthDate();
	FormatDate(birthDatePtr[0], birthDatePtr[1], birthDatePtr[2], birthDate);
	ReplaceDataElement(dataset, std::string("PatientBirthDate"), birthDate);

	// Patient's Sex (Type = 2, VM = 1)
	std::string patientSex = "";
	if (rtDose->GetPatientSex() == Male)
	{
		patientSex = "M";
	}
	else if (rtDose->GetPatientSex() == Female)
	{
		patientSex = "F";
	}
	else if (rtDose->GetPatientSex() == Other)
	{
		patientSex = "O";
	}
	ReplaceDataElement(dataset, std::string("PatientSex"), patientSex);

	// Referenced Patient Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// Patient's Birth Time (Type = 3, VM = ?)
	// Other Patient IDs (Type = 3, VM = ?)
	// Other Patient IDs Sequence (Type = 3, VM = ?)
	// > Patient ID (Type = 1, VM = 1)
	// > Issuer of Patient ID Macro (Table 10-18)
	// > Type of Patient ID (Type = 1, VM = 1)
	// Other Patient Names (Type = 3, VM = ?)
	// Ethnic Group (Type = 3, VM = ?)
	// Patient Comments (Type = 3, VM = ?)
	// Patient Species Description (Type = 1C, VM = ?)
	// Patient Species Code Sequence (Type = 1C, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Patient Breed Description (Type = 2C, VM = ?)
	// Patient Breed Code Sequence (Type = 2C, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Breed Registration Sequence (Type = 2C, VM = ?)
	// > Breed Registration Number (Type = 1, VM = ?)
	// > Breed Registry Code Sequence (Type = 1, VM = ?)
	// >> Code Sequence Macro (Table 8.8-1)
	// Responsible Person (Type = 2C, VM = ?)
	// Responsible Person Role (Type = 1C, VM = ?)
	// Responsible Organization (Type = 2C, VM = ?)
	// Patient Identity Removed (Type = 3, VM = ?)
	// De-identification Method (Type = 1C, VM = ?)
	// De-identification Method Code Sequence (Type = 1C, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)

	// General Study Module (C7.2.1)
	// --------------------------------------------------------------

	// Study Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("StudyInstanceUID"),
					   rtDose->GetStudyInstanceUID());

	// Study Date (Type = 2, VM = 1)
	std::string studyDate;
	unsigned short* studyDatePtr = rtDose->GetStudyDate();
	FormatDate(studyDatePtr[0], studyDatePtr[1], studyDatePtr[2], studyDate);
	ReplaceDataElement(dataset, std::string("StudyDate"), studyDate);

	// Study Time (Type = 2, VM = 1)
	std::string studyTime;
	unsigned short* studyTimePtr = rtDose->GetStudyTime();
	FormatTime(studyTimePtr[0], studyTimePtr[1], studyTimePtr[2], studyTime);
	ReplaceDataElement(dataset, std::string("StudyTime"), studyTime);

	// Referring Physician's Name (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("ReferringPhysicianName"),
					   rtDose->GetReferringPhysicianName());

	// Referring Physician Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)

	// Study ID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("StudyID"), rtDose->GetStudyID());

	// Accession Number (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("AccessionNumber"),
					   rtDose->GetAccessionNumber());

	// Issuer of Accession Number Sequence (Type = 3, VM = ?)
	// > HL7v2 Hierarchic Designator Macro (Table 10-17)
	// Study Description (Type = 3, VM = ?)
	// Physician(s) of Record (Type = 3, VM = ?)
	// Physician(s) of Record Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)
	// Name of Physician(s) Reading Study (Type = 3, VM = ?)
	// Physician(s) Reading Study Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)
	// Requesting Service Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Referenced Study Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// Procedure Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Reason For Performed Procedure Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)

	// RT Series Module (C8.8.1)
	// --------------------------------------------------------------

	// Modality (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("Modality"), rtDose->GetModality());

	// Series Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("SeriesInstanceUID"),
					   rtDose->GetSeriesInstanceUID());

	// Series Number (Type = 2, VM = 1)
	std::string seriesNr = "";
	if (rtDose->GetSeriesNumber() > 0)
	{
		StringEncodeNumber(rtDose->GetSeriesNumber(), seriesNr);
	}
	ReplaceDataElement(dataset, std::string("SeriesNumber"), seriesNr);

	// Series Description (Type = 3, VM = ?)
	// Series Description Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)

	// Operators' Name (Type = 2, VM = 1-n)
	for (unsigned int i = 0; i < rtDose->GetOperatorsNameCount(); ++i)
	{
		InsertDataElement(dataset, std::string("OperatorsName"),
						  rtDose->GetOperatorsName(i));
	}

	// Referenced Performed Procedure Step Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// Request Attributes Sequence (Type = 3, VM = ?)
	// > Request Attributes Macro (Table 10-11)

	// Frame Of Reference Module (C7.4.1)
	// --------------------------------------------------------------

	// Frame of Reference UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("FrameOfReferenceUID"),
					   rtDose->GetFrameOfReferenceUID());

	// Position Reference Indicator (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("PositionReferenceIndicator"),
					   rtDose->GetPositionReferenceIndicator());

	// General Equipment Module (C7.5.1)
	// --------------------------------------------------------------

	// Manufacturer (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("Manufacturer"),
					   rtDose->GetManufacturer());

	// Institution Name (Type = 3, VM = ?)
	// Institution Address (Type = 3, VM = ?)
	// Station Name (Type = 3, VM = ?)
	// Institutional Department Name (Type = 3, VM = ?)
	// Manufacturer's Model Name (Type = 3, VM = ?)
	// Device Serial Number (Type = 3, VM = ?)
	// Software Versions (Type = 3, VM = ?)
	// Gantry ID (Type = 3, VM = ?)
	// Spatial Resolution (Type = 3, VM = ?)
	// Date of Last Calibration (Type = 3, VM = ?)
	// Time of Last Calibration (Type = 3, VM = ?)
	// Pixel Padding Value (Type = 1C, VM = ?) // TODO: ???

	// General Image Module (C7.6.1)
	// --------------------------------------------------------------

	// Instance Number (Type = 2, VM = 1)
	std::string instanceNr = "";
	if (rtDose->GetInstanceNumber() > 0)
	{
		StringEncodeNumber(rtDose->GetInstanceNumber(), instanceNr);
	}
	ReplaceDataElement(dataset, std::string("InstanceNumber"), instanceNr);

	// Patient Orientation (Type = 2C, VM = ?)
	// Content Date (Type = 2C, VM = ?)
	// Content Time (Type = 2C, VM = ?)
	// Image Type (Type = 3, VM = ?)
	// Acquisition Number (Type = 3, VM = ?)
	// Acquisition Date (Type = 3, VM = ?)
	// Acquisition DateTime (Type = 3, VM = ?)
	// Referenced Image Sequence (Type = 3, VM = ?)
	// > Image SOP Instance Reference Macro (Table 10-3)
	// > Purpose of Reference Code Sequence (Type = 3, VM = ?)
	// >> Code Sequence Macro (Table 8.8-1)
	// Derivation Description (Type = 3, VM = ?)
	// Derivation Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Source Image Sequence (Type = 3, VM = ?)
	// > Image SOP Instance Reference Macro (Table 10-3)
	// > Purpose of Reference Code Sequence (Type = 3, VM = ?)
	// >> Code Sequence Macro (Table 8.8-1)
	// > Spatial Locations Preserved (Type = 3, VM = ?)
	// > Patient Orientation (Type = 1C, VM = ?)
	// Referenced Instance Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// > Purpose of Reference Code Sequence (Type = 1, VM = ?)
	// >> Code Sequence Macro (Table 8.8-1)
	// Images in Acquisition (Type = 3, VM = ?)
	// Image Comments (Type = 3, VM = ?)
	// Quality Control Image (Type = 3, VM = ?)
	// Burned In Annotation (Type = 3, VM = ?)
	// Recognizable Visual Features (Type = 3, VM = ?)
	// Lossy Image Compression (Type = 3, VM = ?)
	// Lossy Image Compression Ratio (Type = 3, VM = ?)
	// Lossy Image Compression Method (Type = 3, VM = ?)
	// Icon Image Sequence (Type = 3, VM = ?)
	// > Image Pixel Macro (Table C.7-11b)
	// Presentation LUT Shape (Type = 3, VM = ?)
	// Irradiation Event UID (Type = 3, VM = ?)

	// Image Plane Module (C7.6.2)
	// --------------------------------------------------------------

	// Pixel Spacing (Type = 1, VM = 2)
	double* pixelSpacingPtr = rtDose->GetPixelSpacing();
	std::string pixelSpacing;
	for (unsigned short i = 0; i < 2; ++i)
	{
		StringEncodeNumber(pixelSpacingPtr[i], pixelSpacing);
		InsertDataElement(dataset, std::string("PixelSpacing"), pixelSpacing);
	}

	// Image Orientation (Patient) (Type = 1, VM = 6)
	double* imgOrientationPtr = rtDose->GetImageOrientationPatient();
	std::string imgOrientation;
	for (unsigned short i = 0; i < 6; ++i)
	{
		StringEncodeNumber(imgOrientationPtr[i], imgOrientation);
		InsertDataElement(dataset, std::string("ImageOrientationPatient"),
						  imgOrientation);
	}

	// Image Position (Patient) (Type = 1, VM = 3)
	double* imgPositionPtr = rtDose->GetImagePositionPatient();
	std::string imgPosition;
	for (unsigned short i = 0; i < 3; ++i)
	{
		StringEncodeNumber(imgPositionPtr[i], imgPosition);
		InsertDataElement(dataset, std::string("ImagePositionPatient"),
						  imgPosition);
	}

	// Slice Thickness (Type = 2, VM = 1)
	std::string sliceThickness = "";
	if (rtDose->GetSliceThickness() > 0)
	{
		StringEncodeNumber(rtDose->GetSliceThickness(), sliceThickness);
	}
	ReplaceDataElement(dataset, std::string("SliceThickness"), sliceThickness);

	// Slice Location (Type = 3, VM = ?)

	// Image Pixel Module (C7.6.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1, VM = 1)
	std::string samplesPerPixel;
	ByteEncodeNumber(rtDose->GetSamplesPerPixel(), samplesPerPixel);
	ReplaceDataElement(dataset, std::string("SamplesPerPixel"),
					   samplesPerPixel);

	// Photometric Interpretation (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("PhotometricInterpretation"),
					   rtDose->GetPhotometricInterpretation());

	// Rows (Type = 1, VM = 1)
	std::string dims;
	ByteEncodeNumber(rtDose->GetRows(), dims);
	ReplaceDataElement(dataset, std::string("Rows"), dims);

	// Columns (Type = 1, VM = 1)
	ByteEncodeNumber(rtDose->GetColumns(), dims);
	ReplaceDataElement(dataset, std::string("Columns"), dims);

	// Bits Allocated (Type = 1, VM = 1)
	std::string bits;
	ByteEncodeNumber(rtDose->GetBitsAllocated(), bits);
	ReplaceDataElement(dataset, std::string("BitsAllocated"), bits);

	// Bits Stored (Type = 1, VM = 1)
	ByteEncodeNumber(rtDose->GetBitsStored(), bits);
	ReplaceDataElement(dataset, std::string("BitsStored"), bits);

	// High Bit (Type = 1, VM = 1)
	ByteEncodeNumber(rtDose->GetHighBit(), bits);
	ReplaceDataElement(dataset, std::string("HighBit"), bits);

	// Pixel Representation (Type = 1, VM = 1)
	ByteEncodeNumber(rtDose->GetPixelRepresentation(), bits);
	ReplaceDataElement(dataset, std::string("PixelRepresentation"), bits);

	// Pixel Data (Type = 1C, VM = 1)
	gdcm::DataElement pixeldata(_attributeTagVRType["PixelData"].first);
	pixeldata.SetVR(_attributeTagVRType["PixelData"].second);
	// Uniform quantization float to integer with dose grid scaling
	double doseGridScaling = rtDose->GetDoseGridScaling();
	unsigned short bytesPerVoxel = rtDose->GetBitsAllocated() / 8;
	unsigned long volume = dimensions[0] * dimensions[1] * dimensions[2];
	char* quantizedPixelData = new char[volume * bytesPerVoxel];
	float* ptrSrc = &rtDose->GetPixelData()[0];
	char* ptrDst = &quantizedPixelData[0];
	for (unsigned long i = 0; i < volume; ++i)
	{
		unsigned int val = std::floor(*ptrSrc++ / doseGridScaling);
		// Byte encode value
		for (unsigned short i = 0; i < bytesPerVoxel; ++i)
		{
			*ptrDst++ = val & 0xFF;
			val = val >> 8;
		}
	}
	pixeldata.SetByteValue(quantizedPixelData,
						   (uint32_t)(volume * bytesPerVoxel));
	image.SetDataElement(pixeldata);

	// Planar Configuration (Type = 1C, VM = 1)
	// Pixel Aspect Ratio (Type = 1C, VM = 1)
	// Smallest Image Pixel Value (Type = 3, VM = ?)
	// Largest Image Pixel Value (Type = 3, VM = ?)
	// Red Palette Color Lookup Table Descriptor (Type = 1C, VM = ?)
	// Green Palette Color Lookup Table Descriptor (Type = 1C, VM = ?)
	// Blue Palette Color Lookup Table Descriptor (Type = 1C, VM = ?)
	// Red Palette Color Lookup Table Data (Type = 1C, VM = ?)
	// Green Palette Color Lookup Table Data (Type = 1C, VM = ?)
	// Blue Palette Color Lookup Table Data (Type = 1C, VM = ?)
	// ICC Profile (Type = 3, VM = ?)
	// Pixel Data Provider URL (Type = 1C, VM = ?)
	// Pixel Padding Range Limit (Type = 1C, VM = ?)

	// Multi-Frame Module (C7.6.6)
	// --------------------------------------------------------------

	// Number Of Frames (Type = 1, VM = 1)
	std::string framesNr;
	StringEncodeNumber(rtDose->GetNumberOfFrames(), framesNr);
	ReplaceDataElement(dataset, std::string("NumberOfFrames"), framesNr);

	// Frame Increment Pointer (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("FrameIncrementPointer"),
					   rtDose->GetFrameIncrementPointer());

	// RT Dose Module (C8.8.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1C, VM = 1)			// See Image Pixel Module (C7.6.3)
	// Photometric Interpretation (Type = 1C, VM = 1)	// See Image Pixel Module (C7.6.3)
	// Bits Allocated (Type = 1C, VM = 1)				// See Image Pixel Module (C7.6.3)
	// Bits Stored (Type = 1C, VM = 1)					// See Image Pixel Module (C7.6.3)
	// High Bit (Type = 1C, VM = 1)						// See Image Pixel Module (C7.6.3)
	// Pixel Representation (Type = 1C, VM = 1)			// See Image Pixel Module (C7.6.3)

	// Dose Units (Type = 1, VM = 1)
	std::string doseUnits = "";
	switch (rtDose->GetDoseUnits())
	{
	case Gray: doseUnits = "GY"; break;
	case Relative: doseUnits = "RELATIVE"; break;
	}
	ReplaceDataElement(dataset, std::string("DoseUnits"), doseUnits);

	// Dose Type (Type = 1, VM = 1)
	std::string doseType = "";
	switch (rtDose->GetDoseType())
	{
	case Physical: doseType = "PHYSICAL"; break;
	case Effective: doseType = "EFFECTIVE"; break;
	case Error: doseType = "ERROR"; break;
	}
	ReplaceDataElement(dataset, std::string("DoseType"), doseType);

	// Instance Number (Type = 3, VM = ?)
	// Dose Comment (Type = 3, VM = ?)
	// Normalization Point (Type = 3, VM = ?)

	// Dose Summation Type (Type = 1, VM = 1)
	std::string summationType = "";
	switch (rtDose->GetDoseSummationType())
	{
	case Plan: summationType = "PLAN"; break;
	case MultiPlan: summationType = "MULTI_PLAN"; break;
	case Fraction: summationType = "FRACTION"; break;
	case Beam: summationType = "BEAM"; break;
	case Brachy: summationType = "BRACHY"; break;
	case ControlPoint: summationType = "CONTROL_POINT"; break;
	}
	ReplaceDataElement(dataset, std::string("DoseSummationType"),
					   summationType);

	// Referenced RT Plan Sequence (Type = 1C, VM = 1) // TODO: Different summation types
	// > SOP Instance Reference Macro (Table 10-11)
	gdcm::SmartPointer<gdcm::SequenceOfItems> sq = new gdcm::SequenceOfItems();
	sq->SetLengthToUndefined();
	for (unsigned int i = 0; i < rtDose->GetReferencedRTPlanInstanceUIDCount();
		 ++i)
	{
		gdcm::Item it;
		it.SetVLToUndefined();
		gdcm::DataSet& nds = it.GetNestedDataSet();
		InsertDataElement(nds, std::string("ReferencedSOPClassUID"),
						  rtDose->GetReferencedRTPlanClassUID());
		InsertDataElement(nds, std::string("ReferencedSOPInstanceUID"),
						  rtDose->GetReferencedRTPlanInstanceUID(i));
		sq->AddItem(it);
	}
	InsertSequence(dataset, std::string("ReferencedRTPlanSequence"), sq);

	// > Referenced Fraction Group Sequence (Type = 1C, VM = ?) // TODO
	// >> Referenced Fraction Group Number (Type = 1, VM = ?) // TODO
	// >> Referenced Beam Sequence (Type = 1C, VM = ?) // TODO
	// >>> Referenced Beam Number (Type = 1, VM = ?) // TODO
	// >>> Referenced Control Point Sequence (Type = 1C, VM = ?) // TODO
	// >>>> Referenced Start Control Point Index (Type = 1, VM = ?) // TODO
	// >>>> Referenced Stop Control Point Index (Type = 1, VM = ?) // TODO
	// >> Referenced Brachy Application Setup Sequence (Type = 1C, VM = ?) // TODO
	// >>> Referenced Brachy Application Setup Number (Type = 1, VM = ?) // TODO

	// Grid Frame Offset Vector (Type = 1C, VM = 2-n)
	// Array which contains the dose image plane offsets (in mm) of the dose image frames
	// TODO: GDCM writer automatically computes grid frame offset vector!!
	/*double offset = rtDose->GetSliceThickness();
	std::string offsetStr;
	for (unsigned short i = 0; i < rtDose->GetNumberOfFrames(); ++i) {
		StringEncodeNumber(i*offset, offsetStr);
		InsertDataElement(dataset, std::string("GridFrameOffsetVector"), offsetStr);
	}*/

	// Dose Grid Scaling (Type = 1C, VM = 1)
	// Scaling factor that when multiplied by the pixel data yields grid doses in the dose units
	std::string gridScaling;
	StringEncodeNumber(doseGridScaling, gridScaling);
	ReplaceDataElement(dataset, std::string("DoseGridScaling"), gridScaling);

	// Tissue Heterogeneity Correction (Type = 3, VM = ?)

	// SOP Common Module (C12.1)
	// --------------------------------------------------------------

	// SOP Class UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("SOPClassUID"),
					   rtDose->GetSOPClassUID());

	// SOP Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("SOPInstanceUID"),
					   rtDose->GetSOPInstanceUID());

	// Specific Character Set (Type = 1C, VM = 1-n)
	for (unsigned short i = 0; i < rtDose->GetSpecificCharacterSetCount(); ++i)
	{
		std::string characterSet = "";
		switch (rtDose->GetSpecificCharacterSet(i))
		{
		case Latin1:
			characterSet = "ISO_IR 100";
			break;
			// TODO: Support other character sets
		}
		InsertDataElement(dataset, std::string("SpecificCharacterSet"),
						  characterSet);
	}

	// Instance Creation Date (Type = 3, VM = ?)
	// Instance Creation Time (Type = 3, VM = ?)
	// Instance Creator UID (Type = 3, VM = ?)
	// Related General SOP Class UID (Type = 3, VM = ?)
	// Original Specialized SOP Class UID (Type = 3, VM = ?)
	// Coding Scheme Identification Sequence (Type = 3, VM = ?)
	// > Coding Scheme Designator (Type = 1, VM = ?)
	// > Coding Scheme Registry (Type = 1C, VM = ?)
	// > Coding Scheme UID (Type = 1C, VM = ?)
	// > Coding Scheme External ID (Type = 2C, VM = ?)
	// > Coding Scheme Name (Type = 3, VM = ?)
	// > Coding Scheme Version (Type = 3, VM = ?)
	// > Coding Scheme Responsible Organization (Type = 3, VM = ?)
	// Timezone Offset From UTC (Type = 3, VM = ?)
	// Contributing Equipment Sequence (Type = 3, VM = ?)
	// > Purpose of Reference Code Sequence (Type = 1, VM = ?)
	// >> Code Sequence Macro (Table 8.8-1)
	// > Manufacturer (Type = 1, VM = ?)
	// > Institution Name (Type = 3, VM = ?)
	// > Institution Address (Type = 3, VM = ?)
	// > Station Name (Type = 3, VM = ?)
	// > Institutional Department Name (Type = 3, VM = ?)
	// > Operators' Name (Type = 3, VM = ?)
	// > Operator Identfication Sequence (Type = 3, VM = ?)
	// >> Person Identification Macro (Table 10-1)
	// > Manufacturer's Model Name (Type = 3, VM = ?)
	// > Device Serial Number (Type = 3, VM = ?)
	// > Software Versions (Type = 3, VM = ?)
	// > Spatial Resolution (Type = 3, VM = ?)
	// > Date of Last Calibration (Type = 3, VM = ?)
	// > Time of Last Calibration (Type = 3, VM = ?)
	// > Contribution DateTime (Type = 3, VM = ?)
	// > Contribution Description (Type = 3, VM = ?)
	// Instance Number (Type = 3, VM = ?)
	// SOP Instance Status (Type = 3, VM = ?)
	// SOP Authorization DateTime (Type = 3, VM = ?)
	// SOP Authorization Comment (Type = 3, VM = ?)
	// Authorization Equipment Certification Number (Type = 3, VM = ?)
	// > Digital Signatures Macro (Table C.12-6)
	// Encrypted Attributes Sequence (Type = 1C, VM = ?)
	// > Encrypted Content Transfer Syntax UID (Type = 1, VM = ?)
	// > Encrypted Content (Type = 1, VM = ?)
	// Original Attributes Sequence (Type = 3, VM = ?)
	// > Source of Previous Values (Type = 2, VM = ?)
	// > Attribute Modification DateTime (Type = 1, VM = ?)
	// > Modifying System (Type = 1, VM = ?)
	// > Reason for the Attribute Modification (Type = 1, VM = ?)
	// > Modified Attributes Sequence (Type = 1, VM = ?)
	// >> Any Attribute from the main data set that was modified or removed may include Sequence Attributes and their items.
	// HL7 Structured Document Reference Sequence (Type = 1C, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// > HL7 Instance Identifier (Type = 1, VM = ?)
	// > Retrieve URI (Type = 3, VM = ?)
	// Longitudinal Temporal Information Modified (Type = 3, VM = ?)

	writer.SetFileName(filename);
	if (!writer.Write())
	{
		delete[] quantizedPixelData;
		return false;
	}

	delete[] quantizedPixelData;
	return true;

#elif 1 // TODO: vtkGDCMImageWriter

	vtkSmartPointer<vtkImageData> input = vtkSmartPointer<vtkImageData>::New();
	input->SetNumberOfScalarComponents(1);
	input
		->SetScalarTypeToUnsignedInt(); // RTDose only accepts UINT16 and UINT32
	input->SetDimensions(dims[0], dims[1], dims[2]);
	input->SetSpacing(spacing);
	input->SetOrigin(origin);
	unsigned int* field = (unsigned int*)input->GetScalarPointer(0, 0, 0);

	unsigned int* fieldptr = &(field[0]);
	for (unsigned int z = 0; z < dims[2]; ++z)
	{
		float* bitsptr = &(bits[z][0]);
		for (unsigned int y = 0; y < dims[1]; ++y)
		{
			for (unsigned int x = 0; x < dims[0]; ++x)
			{
				*fieldptr++ =
					(unsigned int)*bitsptr++; // TODO: RT Dose scalar scaling
			}
		}
	}

	vtkGDCMImageWriter* writer = vtkGDCMImageWriter::New();
	writer->SetFileName(filename);
	writer->SetInput(input);
	writer->SetFileDimensionality(3);
	vtkSmartPointer<vtkMedicalImageProperties> medicalImageProps =
		vtkSmartPointer<vtkMedicalImageProperties>::New();
	medicalImageProps->SetModality("RT");
	writer->SetMedicalImageProperties(medicalImageProps);
	writer->SetShift(0.0);
	writer->SetScale(1.0); // TODO
	writer->SetImageFormat();
	writer->Write();

	// TODO writer->SetMedicalImageProperties();

	writer->Delete();
	return true;
#endif
}

void RTDoseWriter::InsertDataElement(gdcm::DataSet& dataset,
									 const std::string keyword,
									 std::string value, unsigned int vm)
{
	gdcm::Tag tag = _attributeTagVRType[keyword].first;
	gdcm::VR::VRType vr = _attributeTagVRType[keyword].second;

	AdjustVRTypeString(vr, vm, value);

	gdcm::DataElement de(tag);
	de.SetVR(vr);
	de.SetByteValue(value.c_str(), gdcm::VL(value.length()));

	dataset.Insert(de);
}

void RTDoseWriter::ReplaceDataElement(gdcm::DataSet& dataset,
									  const std::string keyword,
									  std::string value, unsigned int vm)
{
	gdcm::Tag tag = _attributeTagVRType[keyword].first;
	gdcm::VR::VRType vr = _attributeTagVRType[keyword].second;

	AdjustVRTypeString(vr, vm, value);

	gdcm::DataElement de(tag);
	de.SetVR(vr);
	de.SetByteValue(value.c_str(), gdcm::VL(value.length()));

	dataset.Replace(de);
}

void RTDoseWriter::InsertSequence(gdcm::DataSet& dataset,
								  const std::string keyword,
								  gdcm::SequenceOfItems* sequence)
{
	gdcm::Tag tag = _attributeTagVRType[keyword].first;
	gdcm::VR::VRType vr = _attributeTagVRType[keyword].second;

	gdcm::DataElement de(tag);
	de.SetVR(vr);
	de.SetValue(*sequence);
	de.SetVLToUndefined();

	dataset.Insert(de);
}

void RTDoseWriter::ReplaceStringSection(std::string& text,
										const std::string oldSection,
										const std::string newSection)
{
	int tmp;
	unsigned int len = newSection.length();
	while ((tmp = text.find(oldSection)) != std::string::npos)
	{
		text.replace(tmp, len, newSection);
	}
}

void RTDoseWriter::FormatDate(unsigned short& year, unsigned short& month,
							  unsigned short& day, std::string& result)
{
	if (year && month && day)
	{
		char buffer[100];
		std::sprintf(buffer, "%04d%02d%02d", year, month, day);
		result = std::string(buffer);
		result.resize(8);
	}
	else
	{
		result = "";
	}
}

void RTDoseWriter::FormatTime(unsigned short& hours, unsigned short& minutes,
							  unsigned short& seconds, std::string& result)
{
	if (hours && minutes && seconds)
	{
		char buffer[100];
		std::sprintf(buffer, "%02d%02d%02d", hours, minutes, seconds);
		result = std::string(buffer);
		result.resize(6);
	}
	else
	{
		result = "";
	}
}

void RTDoseWriter::AdjustVRTypeString(gdcm::VR::VRType& vr, unsigned int vm,
									  std::string& value)
{
	// TODO: Confirm padding
	switch (vr)
	{
	/*case gdcm::VR::AE:
		// Padding with trailing space
		if (value.length()%2 != 0) {
			value.append(" ");
		}
		break;*/
	case gdcm::VR::AS: break;
	case gdcm::VR::AT: break;
	//case gdcm::VR::CS:
	case gdcm::VR::DS:
	case gdcm::VR::IS:
	case gdcm::VR::SH:
	//case gdcm::VR::ST:
	case gdcm::VR::LT:
	case gdcm::VR::UT:
		// Padding with trailing space
		if (value.length() % 2 != 0)
		{
			value.append(" ");
		}
		break;
	case gdcm::VR::DA: break;
	case gdcm::VR::DT: break;
	case gdcm::VR::FL: break;
	case gdcm::VR::FD: break;
	case gdcm::VR::LO:
		ReplaceStringSection(value, std::string("\\"), std::string(""));
		// Padding with trailing space
		if (value.length() % 2 != 0)
		{
			value.append(" ");
		}
		break;
	case gdcm::VR::OB:
		//case gdcm::VR::UI:
		// Padding with trailing nullptr byte
		if (value.length() % 2 != 0)
		{
			value.append("0");
		}
		break;
	case gdcm::VR::OF: break;
	case gdcm::VR::OW: break;
	case gdcm::VR::PN:
		ReplaceStringSection(value, std::string(" "), std::string("^"));
		// Padding with trailing space
		if (value.length() % 2 != 0)
		{
			value.append(" ");
		}
		break;
	case gdcm::VR::SL: break;
	case gdcm::VR::SQ: break;
	case gdcm::VR::SS: break;
	case gdcm::VR::TM: break;
	case gdcm::VR::UL: break;
	case gdcm::VR::UN: break;
	case gdcm::VR::US: break;
	default: break;
	}

	if (vm == 1)
	{
		unsigned short maxLen = _vrTypeMaxLength[vr];
		if (maxLen && value.length() > maxLen)
		{
			value.resize(maxLen);
		}
	}
}

template<typename T>
void RTDoseWriter::StringEncodeNumber(T number, std::string& value)
{
	std::stringstream out;
	out << number;
	value = out.str();
}

template<typename T>
void RTDoseWriter::ByteEncodeNumber(T number, std::string& value)
{
	unsigned short bytes = sizeof(T);
	char* buffer = new char[bytes];
	for (unsigned short i = 0; i < bytes; ++i)
	{
		buffer[bytes - i - 1] = number & 0xFF;
		number = number >> 8;
	}
	value = std::string(buffer);
	delete[] buffer;
}

template<typename T>
void RTDoseWriter::StringEncodeVector(const T* offsets, unsigned int count,
									  std::string& value)
{
	if (count <= 0)
		return;
	const std::string delimiter = "\\";
	value = "";

	for (unsigned int i = 0; i < count; ++i)
	{
		std::string numberStr;
		StringEncodeNumber(offsets[i], numberStr);
		value.append(numberStr);
		value.append(delimiter);
	}
	value.resize(value.size() - 1); // Remove last delimiter
}

template<typename T>
void RTDoseWriter::ByteEncodeVector(const T* offsets, unsigned int count,
									std::string& value)
{
	// TODO: test or remove
	if (count <= 0)
		return;
	char delimiter = '\\';
	value = "";

	unsigned short bytes = sizeof(T);
	char* buffer = new char[bytes];
	for (unsigned int idx = 0; idx < count; ++idx)
	{
		T number = offsets[idx];
		for (unsigned short i = 0; i < bytes; ++i)
		{
			buffer[bytes - i - 1] = number & 0xFF;
			number = number >> 8;
		}
		std::string numberStr = std::string(buffer);
		numberStr = numberStr.erase(0, numberStr.find_first_not_of('0'));
		value.append(numberStr);
		value.append(std::string(1,delimiter));
	}
	value.resize(value.size() - 1); // Remove last delimiter

	delete[] buffer;
}

std::map<gdcm::VR::VRType, unsigned short> RTDoseWriter::_vrTypeMaxLength;
std::map<std::string, RTDoseWriter::TagVRTypePair>
	RTDoseWriter::_attributeTagVRType;
	
} // namespace iseg