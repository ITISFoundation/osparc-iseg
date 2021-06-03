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

#include "RTDoseWriter.h"

#include "gdcmAttribute.h"
#include "gdcmImageWriter.h"
#include "gdcmItem.h"
#include "gdcmSequenceOfItems.h"

#include <sstream>

namespace iseg {

RTDoseWriter::RTDoseWriter()
{
	if (vr_type_max_length.empty())
	{
		vr_type_max_length[gdcm::VR::AE] = 16;
		vr_type_max_length[gdcm::VR::AS] = 4;
		vr_type_max_length[gdcm::VR::AT] = 4;
		vr_type_max_length[gdcm::VR::CS] = 16;
		vr_type_max_length[gdcm::VR::DA] = 8;
		vr_type_max_length[gdcm::VR::DS] = 16;
		vr_type_max_length[gdcm::VR::DT] = 26;
		vr_type_max_length[gdcm::VR::FL] = 4;
		vr_type_max_length[gdcm::VR::FD] = 16;
		vr_type_max_length[gdcm::VR::IS] = 12;
		vr_type_max_length[gdcm::VR::LO] = 64;
		vr_type_max_length[gdcm::VR::LT] = 10240;
		vr_type_max_length[gdcm::VR::OB] = 1;
		vr_type_max_length[gdcm::VR::OF] = 0;
		vr_type_max_length[gdcm::VR::OW] = 0;
		vr_type_max_length[gdcm::VR::PN] = 64;
		vr_type_max_length[gdcm::VR::SH] = 16;
		vr_type_max_length[gdcm::VR::SL] = 4;
		vr_type_max_length[gdcm::VR::SQ] = 0;
		vr_type_max_length[gdcm::VR::SS] = 2;
		vr_type_max_length[gdcm::VR::ST] = 1024;
		vr_type_max_length[gdcm::VR::TM] = 16;
		vr_type_max_length[gdcm::VR::UI] = 64;
		vr_type_max_length[gdcm::VR::UL] = 4;
		vr_type_max_length[gdcm::VR::UN] = 0;
		vr_type_max_length[gdcm::VR::US] = 2;
		vr_type_max_length[gdcm::VR::UT] = 0;
	}
	if (attribute_tag_vr_type.empty())
	{
		attribute_tag_vr_type["PatientName"] =
				TagVRTypePair(gdcm::Tag(0x0010, 0x0010), gdcm::VR::PN);
		attribute_tag_vr_type["PatientID"] =
				TagVRTypePair(gdcm::Tag(0x0010, 0x0020), gdcm::VR::LO);
		attribute_tag_vr_type["PatientBirthDate"] =
				TagVRTypePair(gdcm::Tag(0x0010, 0x0030), gdcm::VR::DA);
		attribute_tag_vr_type["PatientSex"] =
				TagVRTypePair(gdcm::Tag(0x0010, 0x0040), gdcm::VR::CS);
		attribute_tag_vr_type["StudyInstanceUID"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x000D), gdcm::VR::UI);
		attribute_tag_vr_type["StudyDate"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0020), gdcm::VR::DA);
		attribute_tag_vr_type["StudyTime"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0030), gdcm::VR::TM);
		attribute_tag_vr_type["ReferringPhysicianName"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0090), gdcm::VR::PN);
		attribute_tag_vr_type["StudyID"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x0010), gdcm::VR::SH);
		attribute_tag_vr_type["AccessionNumber"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0050), gdcm::VR::SH);
		attribute_tag_vr_type["Modality"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0060), gdcm::VR::CS);
		attribute_tag_vr_type["SeriesInstanceUID"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x000E), gdcm::VR::UI);
		attribute_tag_vr_type["SeriesNumber"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x0011), gdcm::VR::IS);
		attribute_tag_vr_type["OperatorsName"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x1070), gdcm::VR::PN);
		attribute_tag_vr_type["FrameOfReferenceUID"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x0052), gdcm::VR::UI);
		attribute_tag_vr_type["PositionReferenceIndicator"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x1040), gdcm::VR::LO);
		attribute_tag_vr_type["Manufacturer"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0070), gdcm::VR::LO);
		attribute_tag_vr_type["InstanceNumber"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x0013), gdcm::VR::IS);
		attribute_tag_vr_type["PixelSpacing"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0030), gdcm::VR::DS);
		attribute_tag_vr_type["ImageOrientationPatient"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x0037), gdcm::VR::DS);
		attribute_tag_vr_type["ImagePositionPatient"] =
				TagVRTypePair(gdcm::Tag(0x0020, 0x0032), gdcm::VR::DS);
		attribute_tag_vr_type["SliceThickness"] =
				TagVRTypePair(gdcm::Tag(0x0018, 0x0050), gdcm::VR::DS);
		attribute_tag_vr_type["SamplesPerPixel"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0002), gdcm::VR::US);
		attribute_tag_vr_type["PhotometricInterpretation"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0004), gdcm::VR::CS);
		attribute_tag_vr_type["Rows"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0010), gdcm::VR::US);
		attribute_tag_vr_type["Columns"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0011), gdcm::VR::US);
		attribute_tag_vr_type["BitsAllocated"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x00100), gdcm::VR::US);
		attribute_tag_vr_type["BitsStored"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0101), gdcm::VR::US);
		attribute_tag_vr_type["HighBit"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0102), gdcm::VR::US);
		attribute_tag_vr_type["PixelRepresentation"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0103), gdcm::VR::US);
		attribute_tag_vr_type["PixelData"] = TagVRTypePair(gdcm::Tag(0x7fe0, 0x0010), gdcm::VR::OW); // TODO: OW or OB
		attribute_tag_vr_type["NumberOfFrames"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0008), gdcm::VR::IS);
		attribute_tag_vr_type["FrameIncrementPointer"] =
				TagVRTypePair(gdcm::Tag(0x0028, 0x0009), gdcm::VR::AT);
		attribute_tag_vr_type["DoseUnits"] =
				TagVRTypePair(gdcm::Tag(0x3004, 0x0002), gdcm::VR::CS);
		attribute_tag_vr_type["DoseType"] =
				TagVRTypePair(gdcm::Tag(0x3004, 0x0004), gdcm::VR::CS);
		attribute_tag_vr_type["DoseSummationType"] =
				TagVRTypePair(gdcm::Tag(0x3004, 0x000A), gdcm::VR::CS);
		attribute_tag_vr_type["ReferencedRTPlanSequence"] =
				TagVRTypePair(gdcm::Tag(0x300C, 0x0002), gdcm::VR::SQ);
		attribute_tag_vr_type["ReferencedSOPClassUID"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x1150), gdcm::VR::UI);
		attribute_tag_vr_type["ReferencedSOPInstanceUID"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x1155), gdcm::VR::UI);
		attribute_tag_vr_type["GridFrameOffsetVector"] =
				TagVRTypePair(gdcm::Tag(0x3004, 0x000C), gdcm::VR::DS);
		attribute_tag_vr_type["DoseGridScaling"] =
				TagVRTypePair(gdcm::Tag(0x3004, 0x000E), gdcm::VR::DS);
		attribute_tag_vr_type["SOPClassUID"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0016), gdcm::VR::UI);
		attribute_tag_vr_type["SOPInstanceUID"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0018), gdcm::VR::UI);
		attribute_tag_vr_type["SpecificCharacterSet"] =
				TagVRTypePair(gdcm::Tag(0x0008, 0x0005), gdcm::VR::CS);
	}
}

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

	unsigned int dimensions[3] = {rtDose->GetRows(), rtDose->GetColumns(), rtDose->GetNumberOfFrames()};
	image.SetNumberOfDimensions(3);
	image.SetDimensions(dimensions);
	double spacing[3] = {rtDose->GetPixelSpacing()[0], rtDose->GetPixelSpacing()[1], rtDose->GetSliceThickness()};
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
	ReplaceDataElement(dataset, std::string("PatientName"), rtDose->GetPatientName());

	// Patient ID (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("PatientID"), rtDose->GetPatientID());

	// Issuer of Patient ID Macro (Table 10-18)

	// Patient's Birth Date (Type = 2, VM = 1)
	std::string birth_date;
	unsigned short* birth_date_ptr = rtDose->GetPatientBirthDate();
	FormatDate(birth_date_ptr[0], birth_date_ptr[1], birth_date_ptr[2], birth_date);
	ReplaceDataElement(dataset, std::string("PatientBirthDate"), birth_date);

	// Patient's Sex (Type = 2, VM = 1)
	std::string patient_sex = "";
	if (rtDose->GetPatientSex() == Male)
	{
		patient_sex = "M";
	}
	else if (rtDose->GetPatientSex() == Female)
	{
		patient_sex = "F";
	}
	else if (rtDose->GetPatientSex() == Other)
	{
		patient_sex = "O";
	}
	ReplaceDataElement(dataset, std::string("PatientSex"), patient_sex);

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
	ReplaceDataElement(dataset, std::string("StudyInstanceUID"), rtDose->GetStudyInstanceUID());

	// Study Date (Type = 2, VM = 1)
	std::string study_date;
	unsigned short* study_date_ptr = rtDose->GetStudyDate();
	FormatDate(study_date_ptr[0], study_date_ptr[1], study_date_ptr[2], study_date);
	ReplaceDataElement(dataset, std::string("StudyDate"), study_date);

	// Study Time (Type = 2, VM = 1)
	std::string study_time;
	unsigned short* study_time_ptr = rtDose->GetStudyTime();
	FormatTime(study_time_ptr[0], study_time_ptr[1], study_time_ptr[2], study_time);
	ReplaceDataElement(dataset, std::string("StudyTime"), study_time);

	// Referring Physician's Name (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("ReferringPhysicianName"), rtDose->GetReferringPhysicianName());

	// Referring Physician Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)

	// Study ID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("StudyID"), rtDose->GetStudyID());

	// Accession Number (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("AccessionNumber"), rtDose->GetAccessionNumber());

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
	ReplaceDataElement(dataset, std::string("SeriesInstanceUID"), rtDose->GetSeriesInstanceUID());

	// Series Number (Type = 2, VM = 1)
	std::string series_nr = "";
	if (rtDose->GetSeriesNumber() > 0)
	{
		StringEncodeNumber(rtDose->GetSeriesNumber(), series_nr);
	}
	ReplaceDataElement(dataset, std::string("SeriesNumber"), series_nr);

	// Series Description (Type = 3, VM = ?)
	// Series Description Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)

	// Operators' Name (Type = 2, VM = 1-n)
	for (unsigned int i = 0; i < rtDose->GetOperatorsNameCount(); ++i)
	{
		InsertDataElement(dataset, std::string("OperatorsName"), rtDose->GetOperatorsName(i));
	}

	// Referenced Performed Procedure Step Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// Request Attributes Sequence (Type = 3, VM = ?)
	// > Request Attributes Macro (Table 10-11)

	// Frame Of Reference Module (C7.4.1)
	// --------------------------------------------------------------

	// Frame of Reference UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("FrameOfReferenceUID"), rtDose->GetFrameOfReferenceUID());

	// Position Reference Indicator (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("PositionReferenceIndicator"), rtDose->GetPositionReferenceIndicator());

	// General Equipment Module (C7.5.1)
	// --------------------------------------------------------------

	// Manufacturer (Type = 2, VM = 1)
	ReplaceDataElement(dataset, std::string("Manufacturer"), rtDose->GetManufacturer());

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
	std::string instance_nr = "";
	if (rtDose->GetInstanceNumber() > 0)
	{
		StringEncodeNumber(rtDose->GetInstanceNumber(), instance_nr);
	}
	ReplaceDataElement(dataset, std::string("InstanceNumber"), instance_nr);

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
	double* pixel_spacing_ptr = rtDose->GetPixelSpacing();
	std::string pixel_spacing;
	for (unsigned short i = 0; i < 2; ++i)
	{
		StringEncodeNumber(pixel_spacing_ptr[i], pixel_spacing);
		InsertDataElement(dataset, std::string("PixelSpacing"), pixel_spacing);
	}

	// Image Orientation (Patient) (Type = 1, VM = 6)
	double* img_orientation_ptr = rtDose->GetImageOrientationPatient();
	std::string img_orientation;
	for (unsigned short i = 0; i < 6; ++i)
	{
		StringEncodeNumber(img_orientation_ptr[i], img_orientation);
		InsertDataElement(dataset, std::string("ImageOrientationPatient"), img_orientation);
	}

	// Image Position (Patient) (Type = 1, VM = 3)
	double* img_position_ptr = rtDose->GetImagePositionPatient();
	std::string img_position;
	for (unsigned short i = 0; i < 3; ++i)
	{
		StringEncodeNumber(img_position_ptr[i], img_position);
		InsertDataElement(dataset, std::string("ImagePositionPatient"), img_position);
	}

	// Slice Thickness (Type = 2, VM = 1)
	std::string slice_thickness = "";
	if (rtDose->GetSliceThickness() > 0)
	{
		StringEncodeNumber(rtDose->GetSliceThickness(), slice_thickness);
	}
	ReplaceDataElement(dataset, std::string("SliceThickness"), slice_thickness);

	// Slice Location (Type = 3, VM = ?)

	// Image Pixel Module (C7.6.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1, VM = 1)
	std::string samples_per_pixel;
	ByteEncodeNumber(rtDose->GetSamplesPerPixel(), samples_per_pixel);
	ReplaceDataElement(dataset, std::string("SamplesPerPixel"), samples_per_pixel);

	// Photometric Interpretation (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("PhotometricInterpretation"), rtDose->GetPhotometricInterpretation());

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
	gdcm::DataElement pixeldata(attribute_tag_vr_type["PixelData"].first);
	pixeldata.SetVR(attribute_tag_vr_type["PixelData"].second);
	// Uniform quantization float to integer with dose grid scaling
	double dose_grid_scaling = rtDose->GetDoseGridScaling();
	unsigned short bytes_per_voxel = rtDose->GetBitsAllocated() / 8;
	unsigned long volume = dimensions[0] * dimensions[1] * dimensions[2];
	char* quantized_pixel_data = new char[volume * bytes_per_voxel];
	float* ptr_src = &rtDose->GetPixelData()[0];
	char* ptr_dst = &quantized_pixel_data[0];
	for (unsigned long i = 0; i < volume; ++i)
	{
		unsigned int val = std::floor(*ptr_src++ / dose_grid_scaling);
		// Byte encode value
		for (unsigned short i = 0; i < bytes_per_voxel; ++i)
		{
			*ptr_dst++ = val & 0xFF;
			val = val >> 8;
		}
	}
	pixeldata.SetByteValue(quantized_pixel_data, (uint32_t)(volume * bytes_per_voxel));
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
	std::string frames_nr;
	StringEncodeNumber(rtDose->GetNumberOfFrames(), frames_nr);
	ReplaceDataElement(dataset, std::string("NumberOfFrames"), frames_nr);

	// Frame Increment Pointer (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("FrameIncrementPointer"), rtDose->GetFrameIncrementPointer());

	// RT Dose Module (C8.8.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1C, VM = 1)			// See Image Pixel Module (C7.6.3)
	// Photometric Interpretation (Type = 1C, VM = 1)	// See Image Pixel Module (C7.6.3)
	// Bits Allocated (Type = 1C, VM = 1)				// See Image Pixel Module (C7.6.3)
	// Bits Stored (Type = 1C, VM = 1)					// See Image Pixel Module (C7.6.3)
	// High Bit (Type = 1C, VM = 1)						// See Image Pixel Module (C7.6.3)
	// Pixel Representation (Type = 1C, VM = 1)			// See Image Pixel Module (C7.6.3)

	// Dose Units (Type = 1, VM = 1)
	std::string dose_units = "";
	switch (rtDose->GetDoseUnits())
	{
	case Gray: dose_units = "GY"; break;
	case Relative: dose_units = "RELATIVE"; break;
	}
	ReplaceDataElement(dataset, std::string("DoseUnits"), dose_units);

	// Dose Type (Type = 1, VM = 1)
	std::string dose_type = "";
	switch (rtDose->GetDoseType())
	{
	case Physical: dose_type = "PHYSICAL"; break;
	case Effective: dose_type = "EFFECTIVE"; break;
	case Error: dose_type = "ERROR"; break;
	}
	ReplaceDataElement(dataset, std::string("DoseType"), dose_type);

	// Instance Number (Type = 3, VM = ?)
	// Dose Comment (Type = 3, VM = ?)
	// Normalization Point (Type = 3, VM = ?)

	// Dose Summation Type (Type = 1, VM = 1)
	std::string summation_type = "";
	switch (rtDose->GetDoseSummationType())
	{
	case Plan: summation_type = "PLAN"; break;
	case MultiPlan: summation_type = "MULTI_PLAN"; break;
	case Fraction: summation_type = "FRACTION"; break;
	case Beam: summation_type = "BEAM"; break;
	case Brachy: summation_type = "BRACHY"; break;
	case ControlPoint: summation_type = "CONTROL_POINT"; break;
	}
	ReplaceDataElement(dataset, std::string("DoseSummationType"), summation_type);

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
		InsertDataElement(nds, std::string("ReferencedSOPClassUID"), rtDose->GetReferencedRTPlanClassUID());
		InsertDataElement(nds, std::string("ReferencedSOPInstanceUID"), rtDose->GetReferencedRTPlanInstanceUID(i));
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
	std::string grid_scaling;
	StringEncodeNumber(dose_grid_scaling, grid_scaling);
	ReplaceDataElement(dataset, std::string("DoseGridScaling"), grid_scaling);

	// Tissue Heterogeneity Correction (Type = 3, VM = ?)

	// SOP Common Module (C12.1)
	// --------------------------------------------------------------

	// SOP Class UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("SOPClassUID"), rtDose->GetSOPClassUID());

	// SOP Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(dataset, std::string("SOPInstanceUID"), rtDose->GetSOPInstanceUID());

	// Specific Character Set (Type = 1C, VM = 1-n)
	for (unsigned short i = 0; i < rtDose->GetSpecificCharacterSetCount(); ++i)
	{
		std::string character_set = "";
		switch (rtDose->GetSpecificCharacterSet(i))
		{
		case Latin1:
			character_set = "ISO_IR 100";
			break;
			// TODO: Support other character sets
		}
		InsertDataElement(dataset, std::string("SpecificCharacterSet"), character_set);
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
		delete[] quantized_pixel_data;
		return false;
	}

	delete[] quantized_pixel_data;
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

void RTDoseWriter::InsertDataElement(gdcm::DataSet& dataset, const std::string keyword, std::string value, unsigned int vm)
{
	gdcm::Tag tag = attribute_tag_vr_type[keyword].first;
	gdcm::VR::VRType vr = attribute_tag_vr_type[keyword].second;

	AdjustVRTypeString(vr, vm, value);

	gdcm::DataElement de(tag);
	de.SetVR(vr);
	de.SetByteValue(value.c_str(), gdcm::VL(value.length()));

	dataset.Insert(de);
}

void RTDoseWriter::ReplaceDataElement(gdcm::DataSet& dataset, const std::string keyword, std::string value, unsigned int vm)
{
	gdcm::Tag tag = attribute_tag_vr_type[keyword].first;
	gdcm::VR::VRType vr = attribute_tag_vr_type[keyword].second;

	AdjustVRTypeString(vr, vm, value);

	gdcm::DataElement de(tag);
	de.SetVR(vr);
	de.SetByteValue(value.c_str(), gdcm::VL(value.length()));

	dataset.Replace(de);
}

void RTDoseWriter::InsertSequence(gdcm::DataSet& dataset, const std::string keyword, gdcm::SequenceOfItems* sequence)
{
	gdcm::Tag tag = attribute_tag_vr_type[keyword].first;
	gdcm::VR::VRType vr = attribute_tag_vr_type[keyword].second;

	gdcm::DataElement de(tag);
	de.SetVR(vr);
	de.SetValue(*sequence);
	de.SetVLToUndefined();

	dataset.Insert(de);
}

void RTDoseWriter::ReplaceStringSection(std::string& text, const std::string oldSection, const std::string newSection)
{
	int tmp;
	unsigned int len = newSection.length();
	while ((tmp = text.find(oldSection)) != std::string::npos)
	{
		text.replace(tmp, len, newSection);
	}
}

void RTDoseWriter::FormatDate(unsigned short& year, unsigned short& month, unsigned short& day, std::string& result)
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

void RTDoseWriter::FormatTime(unsigned short& hours, unsigned short& minutes, unsigned short& seconds, std::string& result)
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

void RTDoseWriter::AdjustVRTypeString(gdcm::VR::VRType& vr, unsigned int vm, std::string& value)
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
		unsigned short max_len = vr_type_max_length[vr];
		if (max_len && value.length() > max_len)
		{
			value.resize(max_len);
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
void RTDoseWriter::StringEncodeVector(const T* offsets, unsigned int count, std::string& value)
{
	if (count <= 0)
		return;
	const std::string delimiter = "\\";
	value = "";

	for (unsigned int i = 0; i < count; ++i)
	{
		std::string number_str;
		StringEncodeNumber(offsets[i], number_str);
		value.append(number_str);
		value.append(delimiter);
	}
	value.resize(value.size() - 1); // Remove last delimiter
}

template<typename T>
void RTDoseWriter::ByteEncodeVector(const T* offsets, unsigned int count, std::string& value)
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
		std::string number_str = std::string(buffer);
		number_str = number_str.erase(0, number_str.find_first_not_of('0'));
		value.append(number_str);
		value.append(std::string(1, delimiter));
	}
	value.resize(value.size() - 1); // Remove last delimiter

	delete[] buffer;
}

std::map<gdcm::VR::VRType, unsigned short> RTDoseWriter::vr_type_max_length;
std::map<std::string, RTDoseWriter::TagVRTypePair>
		RTDoseWriter::attribute_tag_vr_type;

} // namespace iseg
