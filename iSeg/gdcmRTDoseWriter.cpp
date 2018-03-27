/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "gdcmRTDoseWriter.h"
#include "gdcmUIDGenerator.h"

namespace gdcm {

RTDoseWriter::RTDoseWriter()
{
	CreateVRTypeMaxLengthMap();
	CreateAttributeTagVRTypeMap();

	QuantizedPixelData = NULL;
	PatientName = "";
	PatientID = "";
	PatientBirthDate[0] = PatientBirthDate[1] = PatientBirthDate[2] = 0;
	PatientSex = Unspecified;
	StudyInstanceUID = GenerateUID();
	StudyDate[0] = StudyDate[1] = StudyDate[2] = 0;
	StudyTime[0] = StudyTime[1] = StudyTime[2] = 0;
	ReferringPhysicianName = "";
	StudyID = "";
	AccessionNumber = "";
	StudyDescription = "";
	StudyDescriptionExists = false;
	SeriesInstanceUID = GenerateUID();
	SeriesNumber = 0;
	FrameOfReferenceUID = GenerateUID();
	PositionReferenceIndicator = "";
	Manufacturer = "";
	StationName = "";
	StationNameExists = false;
	ManufacturersModelName = "";
	ManufacturersModelNameExists = false;
	InstanceNumber = 0;
	ContentDate[0] = ContentDate[1] = ContentDate[2] = 0;
	ContentDateExists = false;
	ContentTime[0] = ContentTime[1] = ContentTime[2] = 0;
	ContentTimeExists = false;
	PixelSpacing[0] = PixelSpacing[1] = 0;
	ImageOrientationPatient[0] = ImageOrientationPatient[1] =
			ImageOrientationPatient[2] = 0.0;
	ImageOrientationPatient[3] = ImageOrientationPatient[4] =
			ImageOrientationPatient[5] = 0.0;
	ImagePositionPatient[0] = ImagePositionPatient[1] = ImagePositionPatient[2] =
			0.0;
	SliceThickness = 0.0;
	Rows = 0;
	Columns = 0;
	BitsAllocated = 0;
	PixelData = NULL;
	NumberOfFrames = 0;
	DoseUnits = Gray;
	DoseType = Physical;
	NormalizationPoint[0] = NormalizationPoint[1] = NormalizationPoint[2] = 0.0;
	NormalizationPointExists = false;
	DoseSummationType = Plan;
	GridFrameOffsetVector = NULL;
	DoseGridScaling = 0.0;
	SOPInstanceUID = GenerateUID();
	InstanceCreationDate[0] = InstanceCreationDate[1] = InstanceCreationDate[2] =
			0;
	InstanceCreationDateExists = false;
	InstanceCreationTime[0] = InstanceCreationTime[1] = InstanceCreationTime[2] =
			0;
	InstanceCreationTimeExists = false;
}

void RTDoseWriter::CreateVRTypeMaxLengthMap()
{
	_vrTypeMaxLength[VR::AE] = 16;
	_vrTypeMaxLength[VR::AS] = 4;
	_vrTypeMaxLength[VR::AT] = 4;
	_vrTypeMaxLength[VR::CS] = 16;
	_vrTypeMaxLength[VR::DA] = 8;
	_vrTypeMaxLength[VR::DS] = 16;
	_vrTypeMaxLength[VR::DT] = 26;
	_vrTypeMaxLength[VR::FL] = 4;
	_vrTypeMaxLength[VR::FD] = 16;
	_vrTypeMaxLength[VR::IS] = 12;
	_vrTypeMaxLength[VR::LO] = 64;
	_vrTypeMaxLength[VR::LT] = 10240;
	_vrTypeMaxLength[VR::OB] = 1;
	_vrTypeMaxLength[VR::OF] = 0;
	_vrTypeMaxLength[VR::OW] = 0;
	_vrTypeMaxLength[VR::PN] = 64;
	_vrTypeMaxLength[VR::SH] = 16;
	_vrTypeMaxLength[VR::SL] = 4;
	_vrTypeMaxLength[VR::SQ] = 0;
	_vrTypeMaxLength[VR::SS] = 2;
	_vrTypeMaxLength[VR::ST] = 1024;
	_vrTypeMaxLength[VR::TM] = 16;
	_vrTypeMaxLength[VR::UI] = 64;
	_vrTypeMaxLength[VR::UL] = 4;
	_vrTypeMaxLength[VR::UN] = 0;
	_vrTypeMaxLength[VR::US] = 2;
	_vrTypeMaxLength[VR::UT] = 0;
}

void RTDoseWriter::CreateAttributeTagVRTypeMap()
{
	_attributeTagVRType["PatientName"] =
			TagVRTypePair(Tag(0x0010, 0x0010), VR::PN);
	_attributeTagVRType["PatientID"] = TagVRTypePair(Tag(0x0010, 0x0020), VR::LO);
	_attributeTagVRType["PatientBirthDate"] =
			TagVRTypePair(Tag(0x0010, 0x0030), VR::DA);
	_attributeTagVRType["PatientSex"] =
			TagVRTypePair(Tag(0x0010, 0x0040), VR::CS);
	_attributeTagVRType["StudyInstanceUID"] =
			TagVRTypePair(Tag(0x0020, 0x000D), VR::UI);
	_attributeTagVRType["StudyDate"] = TagVRTypePair(Tag(0x0008, 0x0020), VR::DA);
	_attributeTagVRType["StudyTime"] = TagVRTypePair(Tag(0x0008, 0x0030), VR::TM);
	_attributeTagVRType["ReferringPhysicianName"] =
			TagVRTypePair(Tag(0x0008, 0x0090), VR::PN);
	_attributeTagVRType["StudyID"] = TagVRTypePair(Tag(0x0020, 0x0010), VR::SH);
	_attributeTagVRType["AccessionNumber"] =
			TagVRTypePair(Tag(0x0008, 0x0050), VR::SH);
	_attributeTagVRType["StudyDescription"] =
			TagVRTypePair(Tag(0x0008, 0x1030), VR::LO);
	_attributeTagVRType["ReferencedStudySequence"] =
			TagVRTypePair(Tag(0x0008, 0x1110), VR::SQ);
	_attributeTagVRType["Modality"] = TagVRTypePair(Tag(0x0008, 0x0060), VR::CS);
	_attributeTagVRType["SeriesInstanceUID"] =
			TagVRTypePair(Tag(0x0020, 0x000E), VR::UI);
	_attributeTagVRType["SeriesNumber"] =
			TagVRTypePair(Tag(0x0020, 0x0011), VR::IS);
	_attributeTagVRType["OperatorsName"] =
			TagVRTypePair(Tag(0x0008, 0x1070), VR::PN);
	_attributeTagVRType["FrameOfReferenceUID"] =
			TagVRTypePair(Tag(0x0020, 0x0052), VR::UI);
	_attributeTagVRType["PositionReferenceIndicator"] =
			TagVRTypePair(Tag(0x0020, 0x1040), VR::LO);
	_attributeTagVRType["Manufacturer"] =
			TagVRTypePair(Tag(0x0008, 0x0070), VR::LO);
	_attributeTagVRType["StationName"] =
			TagVRTypePair(Tag(0x0008, 0x1010), VR::SH);
	_attributeTagVRType["ManufacturersModelName"] =
			TagVRTypePair(Tag(0x0008, 0x1090), VR::LO);
	_attributeTagVRType["SoftwareVersions"] =
			TagVRTypePair(Tag(0x0008, 0x1020), VR::LO);
	_attributeTagVRType["InstanceNumber"] =
			TagVRTypePair(Tag(0x0020, 0x0013), VR::IS);
	_attributeTagVRType["ContentDate"] =
			TagVRTypePair(Tag(0x0008, 0x0023), VR::DA);
	_attributeTagVRType["ContentTime"] =
			TagVRTypePair(Tag(0x0008, 0x0033), VR::TM);
	_attributeTagVRType["PixelSpacing"] =
			TagVRTypePair(Tag(0x0028, 0x0030), VR::DS);
	_attributeTagVRType["ImageOrientationPatient"] =
			TagVRTypePair(Tag(0x0020, 0x0037), VR::DS);
	_attributeTagVRType["ImagePositionPatient"] =
			TagVRTypePair(Tag(0x0020, 0x0032), VR::DS);
	_attributeTagVRType["SliceThickness"] =
			TagVRTypePair(Tag(0x0018, 0x0050), VR::DS);
	_attributeTagVRType["SamplesPerPixel"] =
			TagVRTypePair(Tag(0x0028, 0x0002), VR::US);
	_attributeTagVRType["PhotometricInterpretation"] =
			TagVRTypePair(Tag(0x0028, 0x0004), VR::CS);
	_attributeTagVRType["Rows"] = TagVRTypePair(Tag(0x0028, 0x0010), VR::US);
	_attributeTagVRType["Columns"] = TagVRTypePair(Tag(0x0028, 0x0011), VR::US);
	_attributeTagVRType["BitsAllocated"] =
			TagVRTypePair(Tag(0x0028, 0x00100), VR::US);
	_attributeTagVRType["BitsStored"] =
			TagVRTypePair(Tag(0x0028, 0x0101), VR::US);
	_attributeTagVRType["HighBit"] = TagVRTypePair(Tag(0x0028, 0x0102), VR::US);
	_attributeTagVRType["PixelRepresentation"] =
			TagVRTypePair(Tag(0x0028, 0x0103), VR::US);
	_attributeTagVRType["PixelData"] =
			TagVRTypePair(Tag(0x7fe0, 0x0010), VR::OW); // TODO: OW or OB
	_attributeTagVRType["NumberOfFrames"] =
			TagVRTypePair(Tag(0x0028, 0x0008), VR::IS);
	_attributeTagVRType["FrameIncrementPointer"] =
			TagVRTypePair(Tag(0x0028, 0x0009), VR::AT);
	_attributeTagVRType["DoseUnits"] = TagVRTypePair(Tag(0x3004, 0x0002), VR::CS);
	_attributeTagVRType["DoseType"] = TagVRTypePair(Tag(0x3004, 0x0004), VR::CS);
	_attributeTagVRType["NormalizationPoint"] =
			TagVRTypePair(Tag(0x3004, 0x0008), VR::DS);
	_attributeTagVRType["DoseSummationType"] =
			TagVRTypePair(Tag(0x3004, 0x000A), VR::CS);
	_attributeTagVRType["ReferencedRTPlanSequence"] =
			TagVRTypePair(Tag(0x300C, 0x0002), VR::SQ);
	_attributeTagVRType["ReferencedSOPClassUID"] =
			TagVRTypePair(Tag(0x0008, 0x1150), VR::UI);
	_attributeTagVRType["ReferencedSOPInstanceUID"] =
			TagVRTypePair(Tag(0x0008, 0x1155), VR::UI);
	_attributeTagVRType["GridFrameOffsetVector"] =
			TagVRTypePair(Tag(0x3004, 0x000C), VR::DS);
	_attributeTagVRType["DoseGridScaling"] =
			TagVRTypePair(Tag(0x3004, 0x000E), VR::DS);
	_attributeTagVRType["SOPClassUID"] =
			TagVRTypePair(Tag(0x0008, 0x0016), VR::UI);
	_attributeTagVRType["SOPInstanceUID"] =
			TagVRTypePair(Tag(0x0008, 0x0018), VR::UI);
	_attributeTagVRType["SpecificCharacterSet"] =
			TagVRTypePair(Tag(0x0008, 0x0005), VR::CS);
	_attributeTagVRType["InstanceCreationDate"] =
			TagVRTypePair(Tag(0x0008, 0x0012), VR::DA);
	_attributeTagVRType["InstanceCreationTime"] =
			TagVRTypePair(Tag(0x0008, 0x0013), VR::TM);
}

RTDoseWriter::~RTDoseWriter()
{
	if (QuantizedPixelData != NULL) {
		delete[] QuantizedPixelData;
		QuantizedPixelData = NULL;
	}
}

bool RTDoseWriter::Write()
{
	// TODO: Only supports volumetric image data. Should dose points and isodose curves be supported? (Find reference which states that only volumetric data in use.)
	// TODO: Only mandatory RT Dose IOD modules included
	// TODO: Only single-frame data supported (C.7.6.6 not included)
	// TODO: Does not Frame-Level retrieve request (C.12.3 not included)
	// TODO: Optional and - if possible - conditional data elements ignored

	if (GetReferencedRTPlanInstanceUIDCount() <= 0)
		return false; // TODO: Check for different dose summation types

	Image &pixeldata = GetImage();
	DataSet &ds = GetFile().GetDataSet();

	unsigned int dimensions[3] = {GetRows(), GetColumns(), GetNumberOfFrames()};
	pixeldata.SetNumberOfDimensions(3);
	pixeldata.SetDimensions(dimensions);
	double spacing[3] = {GetPixelSpacing()[0], GetPixelSpacing()[1],
											 GetSliceThickness()};
	pixeldata.SetSpacing(spacing);
	pixeldata.SetDirectionCosines(GetImageOrientationPatient());
	pixeldata.SetOrigin(GetImagePositionPatient());
	pixeldata.SetSlope(GetDoseGridScaling());
	pixeldata.SetIntercept(0.0);
	/* pixeldata.SetLossyFlag();
  pixeldata.SetLUT();
  pixeldata.SetNeedByteSwap();
  pixeldata.SetNumberOfCurves();
  pixeldata.SetNumberOfOverlays();
  pixeldata.SetPlanarConfiguration();
  pixeldata.SetSwapCode();
  pixeldata.SetTransferSyntax( TransferSyntax::ExplicitVRLittleEndian );
  pixeldata.SetDataElement();
  File &file = writer.GetFile();
  FileMetaInformation &metaInfo = file.GetHeader(); */

	PixelFormat pf;
	if (GetBitsAllocated() == 32) {
		pf = PixelFormat::UINT32;
	}
	else {
		pf = PixelFormat::UINT16;
	}
	pf.SetSamplesPerPixel(GetSamplesPerPixel());
	pixeldata.SetPixelFormat(pf);

	PhotometricInterpretation pi = PhotometricInterpretation::MONOCHROME2;
	pixeldata.SetPhotometricInterpretation(pi);

	// Write RT dose modules
	// --------------------------------------------------------------

	WritePatientModule(ds);
	WriteGeneralStudyModule(ds);
	WriteRTSeriesModule(ds);
	WriteFrameOfReferenceModule(ds);
	WriteGeneralEquipmentModule(ds);
	WriteGeneralImageModule(ds);
	WriteImagePlaneModule(ds);
	WriteImagePixelModule(ds, pixeldata);
	WriteMultiFrameModule(ds);
	WriteRTDoseModule(ds);
	WriteSOPCommonModule(ds);

	return ImageWriter::Write();
}

inline void RTDoseWriter::WritePatientModule(DataSet &ds)
{
	// Patient Module (C7.1.1)
	// --------------------------------------------------------------

	// Patient's Name (Type = 2, VM = 1)
	ReplaceDataElement(ds, std::string("PatientName"), GetPatientName());

	// Patient ID (Type = 2, VM = 1)
	ReplaceDataElement(ds, std::string("PatientID"), GetPatientID());

	// Issuer of Patient ID Macro (Table 10-18)

	// Patient's Birth Date (Type = 2, VM = 1)
	std::string birthDate;
	unsigned short *birthDatePtr = GetPatientBirthDate();
	FormatDate(birthDatePtr[0], birthDatePtr[1], birthDatePtr[2], birthDate);
	ReplaceDataElement(ds, std::string("PatientBirthDate"), birthDate);

	// Patient's Sex (Type = 2, VM = 1)
	std::string patientSex = "";
	if (GetPatientSex() == Male) {
		patientSex = "M";
	}
	else if (GetPatientSex() == Female) {
		patientSex = "F";
	}
	else if (GetPatientSex() == Other) {
		patientSex = "O";
	}
	ReplaceDataElement(ds, std::string("PatientSex"), patientSex);

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
}

inline void RTDoseWriter::WriteGeneralStudyModule(DataSet &ds)
{
	// General Study Module (C7.2.1)
	// --------------------------------------------------------------

	// Study Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("StudyInstanceUID"),
										 GetStudyInstanceUID());

	// Study Date (Type = 2, VM = 1)
	std::string studyDate;
	unsigned short *studyDatePtr = GetStudyDate();
	FormatDate(studyDatePtr[0], studyDatePtr[1], studyDatePtr[2], studyDate);
	ReplaceDataElement(ds, std::string("StudyDate"), studyDate);

	// Study Time (Type = 2, VM = 1)
	std::string studyTime;
	unsigned short *studyTimePtr = GetStudyTime();
	FormatTime(studyTimePtr[0], studyTimePtr[1], studyTimePtr[2], studyTime);
	ReplaceDataElement(ds, std::string("StudyTime"), studyTime);

	// Referring Physician's Name (Type = 2, VM = 1)
	ReplaceDataElement(ds, std::string("ReferringPhysicianName"),
										 GetReferringPhysicianName());

	// Referring Physician Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)

	// Study ID (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("StudyID"), GetStudyID());

	// Accession Number (Type = 2, VM = 1)
	ReplaceDataElement(ds, std::string("AccessionNumber"), GetAccessionNumber());

	// Issuer of Accession Number Sequence (Type = 3, VM = ?)
	// > HL7v2 Hierarchic Designator Macro (Table 10-17)

	// Study Description (Type = 3, VM = 1)
	if (StudyDescriptionExists) {
		ReplaceDataElement(ds, std::string("StudyDescription"),
											 GetStudyDescription());
	}

	// Physician(s) of Record (Type = 3, VM = ?)
	// Physician(s) of Record Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)
	// Name of Physician(s) Reading Study (Type = 3, VM = ?)
	// Physician(s) Reading Study Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)
	// Requesting Service Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)

	// Referenced Study Sequence (Type = 3, VM = 1-n)
	// > SOP Instance Reference Macro (Table 10-11)
	if (GetReferencedStudyInstanceUIDCount() > 0) {
		SmartPointer<SequenceOfItems> sq = new SequenceOfItems();
		sq->SetLengthToUndefined();
		for (unsigned int i = 0; i < GetReferencedStudyInstanceUIDCount(); ++i) {
			Item it;
			it.SetVLToUndefined();
			DataSet &nds = it.GetNestedDataSet();
			InsertDataElement(nds, std::string("ReferencedSOPClassUID"),
												GetReferencedStudyClassUID());
			InsertDataElement(nds, std::string("ReferencedSOPInstanceUID"),
												GetReferencedStudyInstanceUID(i));
			sq->AddItem(it);
		}
		InsertSequence(ds, std::string("ReferencedStudySequence"), sq);
	}

	// Procedure Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Reason For Performed Procedure Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
}

inline void RTDoseWriter::WriteRTSeriesModule(DataSet &ds)
{
	// RT Series Module (C8.8.1)
	// --------------------------------------------------------------

	// Modality (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("Modality"), GetModality());

	// Series Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("SeriesInstanceUID"),
										 GetSeriesInstanceUID());

	// Series Number (Type = 2, VM = 1)
	std::string seriesNr = "";
	if (GetSeriesNumber() > 0) {
		StringEncodeNumber(GetSeriesNumber(), seriesNr);
	}
	ReplaceDataElement(ds, std::string("SeriesNumber"), seriesNr);

	// Series Description (Type = 3, VM = ?)
	// Series Description Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)

	// Operators' Name (Type = 2, VM = 1-n)
	for (unsigned int i = 0; i < GetOperatorsNameCount(); ++i) {
		InsertDataElement(ds, std::string("OperatorsName"), GetOperatorsName(i));
	}

	// Referenced Performed Procedure Step Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// Request Attributes Sequence (Type = 3, VM = ?)
	// > Request Attributes Macro (Table 10-11)
}

inline void RTDoseWriter::WriteFrameOfReferenceModule(DataSet &ds)
{
	// Frame Of Reference Module (C7.4.1)
	// --------------------------------------------------------------

	// Frame of Reference UID (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("FrameOfReferenceUID"),
										 GetFrameOfReferenceUID());

	// Position Reference Indicator (Type = 2, VM = 1)
	ReplaceDataElement(ds, std::string("PositionReferenceIndicator"),
										 GetPositionReferenceIndicator());
}

inline void RTDoseWriter::WriteGeneralEquipmentModule(DataSet &ds)
{
	// General Equipment Module (C7.5.1)
	// --------------------------------------------------------------

	// Manufacturer (Type = 2, VM = 1)
	ReplaceDataElement(ds, std::string("Manufacturer"), GetManufacturer());

	// Institution Name (Type = 3, VM = ?)
	// Institution Address (Type = 3, VM = ?)

	// Station Name (Type = 3, VM = 1)
	if (StationNameExists) {
		ReplaceDataElement(ds, std::string("StationName"), GetStationName());
	}

	// Institutional Department Name (Type = 3, VM = ?)

	// Manufacturer's Model Name (Type = 3, VM = 1)
	if (ManufacturersModelNameExists) {
		ReplaceDataElement(ds, std::string("ManufacturersModelName"),
											 GetManufacturersModelName());
	}

	// Device Serial Number (Type = 3, VM = ?)

	// Software Versions (Type = 3, VM = 1-n)
	for (unsigned int i = 0; i < GetSoftwareVersionsCount(); ++i) {
		InsertDataElement(ds, std::string("SoftwareVersions"),
											GetSoftwareVersions(i));
	}

	// Gantry ID (Type = 3, VM = ?)
	// Spatial Resolution (Type = 3, VM = ?)
	// Date of Last Calibration (Type = 3, VM = ?)
	// Time of Last Calibration (Type = 3, VM = ?)
	// Pixel Padding Value (Type = 1C, VM = ?) // TODO: ???
}

inline void RTDoseWriter::WriteGeneralImageModule(DataSet &ds)
{
	// General Image Module (C7.6.1)
	// --------------------------------------------------------------

	// Instance Number (Type = 2, VM = 1)
	std::string instanceNr = "";
	if (GetInstanceNumber() > 0) {
		StringEncodeNumber(GetInstanceNumber(), instanceNr);
	}
	ReplaceDataElement(ds, std::string("InstanceNumber"), instanceNr);

	// Patient Orientation (Type = 2C, VM = ?)

	// Content Date (Type = 2C, VM = 1)
	if (ContentDateExists) {
		std::string contentDate;
		unsigned short *contentDatePtr = GetContentDate();
		FormatDate(contentDatePtr[0], contentDatePtr[1], contentDatePtr[2],
							 contentDate);
		ReplaceDataElement(ds, std::string("ContentDate"), contentDate);
	}

	// Content Time (Type = 2C, VM = 1)
	if (ContentTimeExists) {
		std::string contentTime;
		unsigned short *contentTimePtr = GetContentTime();
		FormatTime(contentTimePtr[0], contentTimePtr[1], contentTimePtr[2],
							 contentTime);
		ReplaceDataElement(ds, std::string("ContentTime"), contentTime);
	}

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
}

inline void RTDoseWriter::WriteImagePlaneModule(DataSet &ds)
{
	// Image Plane Module (C7.6.2)
	// --------------------------------------------------------------

	// Pixel Spacing (Type = 1, VM = 2)
	double *pixelSpacingPtr = GetPixelSpacing();
	std::string pixelSpacing;
	for (unsigned short i = 0; i < 2; ++i) {
		StringEncodeNumber(pixelSpacingPtr[i], pixelSpacing);
		InsertDataElement(ds, std::string("PixelSpacing"), pixelSpacing);
	}

	// Image Orientation (Patient) (Type = 1, VM = 6)
	double *imgOrientationPtr = GetImageOrientationPatient();
	std::string imgOrientation;
	for (unsigned short i = 0; i < 6; ++i) {
		StringEncodeNumber(imgOrientationPtr[i], imgOrientation);
		InsertDataElement(ds, std::string("ImageOrientationPatient"),
											imgOrientation);
	}

	// Image Position (Patient) (Type = 1, VM = 3)
	double *imgPositionPtr = GetImagePositionPatient();
	std::string imgPosition;
	for (unsigned short i = 0; i < 3; ++i) {
		StringEncodeNumber(imgPositionPtr[i], imgPosition);
		InsertDataElement(ds, std::string("ImagePositionPatient"), imgPosition);
	}

	// Slice Thickness (Type = 2, VM = 1)
	std::string sliceThickness = "";
	if (GetSliceThickness() > 0) {
		StringEncodeNumber(GetSliceThickness(), sliceThickness);
	}
	ReplaceDataElement(ds, std::string("SliceThickness"), sliceThickness);

	// Slice Location (Type = 3, VM = ?)
}

inline void RTDoseWriter::WriteImagePixelModule(DataSet &ds, Image &pixeldata)
{
	// Image Pixel Module (C7.6.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1, VM = 1)
	std::string samplesPerPixel;
	ByteEncodeNumber(GetSamplesPerPixel(), samplesPerPixel);
	ReplaceDataElement(ds, std::string("SamplesPerPixel"), samplesPerPixel);

	// Photometric Interpretation (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("PhotometricInterpretation"),
										 GetPhotometricInterpretation());

	// Rows (Type = 1, VM = 1)
	std::string dims;
	ByteEncodeNumber(GetRows(), dims);
	ReplaceDataElement(ds, std::string("Rows"), dims);

	// Columns (Type = 1, VM = 1)
	ByteEncodeNumber(GetColumns(), dims);
	ReplaceDataElement(ds, std::string("Columns"), dims);

	// Bits Allocated (Type = 1, VM = 1)
	std::string bits;
	ByteEncodeNumber(GetBitsAllocated(), bits);
	ReplaceDataElement(ds, std::string("BitsAllocated"), bits);

	// Bits Stored (Type = 1, VM = 1)
	ByteEncodeNumber(GetBitsStored(), bits);
	ReplaceDataElement(ds, std::string("BitsStored"), bits);

	// High Bit (Type = 1, VM = 1)
	ByteEncodeNumber(GetHighBit(), bits);
	ReplaceDataElement(ds, std::string("HighBit"), bits);

	// Pixel Representation (Type = 1, VM = 1)
	ByteEncodeNumber(GetPixelRepresentation(), bits);
	ReplaceDataElement(ds, std::string("PixelRepresentation"), bits);

	// Pixel Data (Type = 1C, VM = 1)
	DataElement de(_attributeTagVRType["PixelData"].first);
	de.SetVR(_attributeTagVRType["PixelData"].second);
	// Uniform quantization float to integer with dose grid scaling
	double doseGridScaling = GetDoseGridScaling();
	unsigned short bytesPerVoxel = GetBitsAllocated() / 8;
	unsigned long volume = GetRows() * GetColumns() * GetNumberOfFrames();
	QuantizedPixelData = new char[volume * bytesPerVoxel];
	float *ptrSrc = &GetPixelData()[0];
	char *ptrDst = &QuantizedPixelData[0];
	for (unsigned long i = 0; i < volume; ++i) {
		unsigned int val = std::floor(*ptrSrc++ / doseGridScaling);
		// Byte encode value
		for (unsigned short i = 0; i < bytesPerVoxel; ++i) {
			*ptrDst++ = val & 0xFF;
			val = val >> 8;
		}
	}
	de.SetByteValue(QuantizedPixelData, (uint32_t)(volume * bytesPerVoxel));
	pixeldata.SetDataElement(de);

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
}

inline void RTDoseWriter::WriteMultiFrameModule(DataSet &ds)
{
	// Multi-Frame Module (C7.6.6)
	// --------------------------------------------------------------

	// Number Of Frames (Type = 1, VM = 1)
	std::string framesNr;
	StringEncodeNumber(GetNumberOfFrames(), framesNr);
	ReplaceDataElement(ds, std::string("NumberOfFrames"), framesNr);

	// Frame Increment Pointer (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("FrameIncrementPointer"),
										 GetFrameIncrementPointer());
}

inline void RTDoseWriter::WriteRTDoseModule(DataSet &ds)
{
	// RT Dose Module (C8.8.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1C, VM = 1)			    // See Image Pixel Module (C7.6.3)
	// Photometric Interpretation (Type = 1C, VM = 1)	// See Image Pixel Module (C7.6.3)
	// Bits Allocated (Type = 1C, VM = 1)				      // See Image Pixel Module (C7.6.3)
	// Bits Stored (Type = 1C, VM = 1)					      // See Image Pixel Module (C7.6.3)
	// High Bit (Type = 1C, VM = 1)						        // See Image Pixel Module (C7.6.3)
	// Pixel Representation (Type = 1C, VM = 1)			  // See Image Pixel Module (C7.6.3)

	// Dose Units (Type = 1, VM = 1)
	std::string doseUnits = "";
	switch (GetDoseUnits()) {
	case Gray: doseUnits = "GY"; break;
	case Relative: doseUnits = "RELATIVE"; break;
	}
	ReplaceDataElement(ds, std::string("DoseUnits"), doseUnits);

	// Dose Type (Type = 1, VM = 1)
	std::string doseType = "";
	switch (GetDoseType()) {
	case Physical: doseType = "PHYSICAL"; break;
	case Effective: doseType = "EFFECTIVE"; break;
	case Error: doseType = "ERROR"; break;
	}
	ReplaceDataElement(ds, std::string("DoseType"), doseType);

	// Instance Number (Type = 3, VM = ?)
	// Dose Comment (Type = 3, VM = ?)

	// Normalization Point (Type = 3, VM = 3)
	if (NormalizationPointExists) {
		double *normPointPtr = GetNormalizationPoint();
		std::string normPoint;
		for (unsigned short i = 0; i < 3; ++i) {
			StringEncodeNumber(normPointPtr[i], normPoint);
			InsertDataElement(ds, std::string("NormalizationPoint"), normPoint);
		}
	}

	// Dose Summation Type (Type = 1, VM = 1)
	std::string summationType = "";
	switch (GetDoseSummationType()) {
	case Plan: summationType = "PLAN"; break;
	case MultiPlan: summationType = "MULTI_PLAN"; break;
	case Fraction: summationType = "FRACTION"; break;
	case Beam: summationType = "BEAM"; break;
	case Brachy: summationType = "BRACHY"; break;
	case ControlPoint: summationType = "CONTROL_POINT"; break;
	}
	ReplaceDataElement(ds, std::string("DoseSummationType"), summationType);

	// Referenced RT Plan Sequence (Type = 1C, VM = 1) // TODO: Different summation types
	// > SOP Instance Reference Macro (Table 10-11)
	SmartPointer<SequenceOfItems> sq = new SequenceOfItems();
	sq->SetLengthToUndefined();
	for (unsigned int i = 0; i < GetReferencedRTPlanInstanceUIDCount(); ++i) {
		Item it;
		it.SetVLToUndefined();
		DataSet &nds = it.GetNestedDataSet();
		InsertDataElement(nds, std::string("ReferencedSOPClassUID"),
											GetReferencedRTPlanClassUID());
		InsertDataElement(nds, std::string("ReferencedSOPInstanceUID"),
											GetReferencedRTPlanInstanceUID(i));
		sq->AddItem(it);
	}
	InsertSequence(ds, std::string("ReferencedRTPlanSequence"), sq);

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
	/*double offset = GetSliceThickness();
  std::string offsetStr;
  for ( unsigned short i = 0; i < GetNumberOfFrames(); ++i ) {
    StringEncodeNumber( i*offset, offsetStr );
    InsertDataElement( ds, std::string("GridFrameOffsetVector"), offsetStr );
  }*/

	// Dose Grid Scaling (Type = 1C, VM = 1)
	// Scaling factor that when multiplied by the pixel data yields grid doses in the dose units
	std::string gridScaling;
	StringEncodeNumber(GetDoseGridScaling(), gridScaling);
	ReplaceDataElement(ds, std::string("DoseGridScaling"), gridScaling);

	// Tissue Heterogeneity Correction (Type = 3, VM = ?)
}

inline void RTDoseWriter::WriteSOPCommonModule(DataSet &ds)
{
	// SOP Common Module (C12.1)
	// --------------------------------------------------------------

	// SOP Class UID (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("SOPClassUID"), GetSOPClassUID());

	// SOP Instance UID (Type = 1, VM = 1)
	ReplaceDataElement(ds, std::string("SOPInstanceUID"), GetSOPInstanceUID());

	// Specific Character Set (Type = 1C, VM = 1-n)
	for (unsigned short i = 0; i < GetSpecificCharacterSetCount(); ++i) {
		std::string characterSet = "";
		switch (GetSpecificCharacterSet(i)) {
		case Latin1:
			characterSet = "ISO_IR 100";
			break;
			// TODO: Support other character sets
		}
		InsertDataElement(ds, std::string("SpecificCharacterSet"), characterSet);
	}

	// Instance Creation Date (Type = 3, VM = 1)
	if (InstanceCreationDateExists) {
		std::string instanceDate;
		unsigned short *instanceDatePtr = GetInstanceCreationDate();
		FormatDate(instanceDatePtr[0], instanceDatePtr[1], instanceDatePtr[2],
							 instanceDate);
		ReplaceDataElement(ds, std::string("InstanceCreationDate"), instanceDate);
	}

	// Instance Creation Time (Type = 3, VM = 1)
	if (InstanceCreationTimeExists) {
		std::string instanceTime;
		unsigned short *instanceTimePtr = GetInstanceCreationTime();
		FormatTime(instanceTimePtr[0], instanceTimePtr[1], instanceTimePtr[2],
							 instanceTime);
		ReplaceDataElement(ds, std::string("InstanceCreationTime"), instanceTime);
	}

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
}

std::string RTDoseWriter::GenerateUID()
{
	UIDGenerator uidGen;
	return std::string(uidGen.Generate());
}

double RTDoseWriter::GetDoseGridScaling()
{
	if (DoseGridScaling == 0.0) {
		float maxValue = 0.0f;
		float *ptr = &PixelData[0];
		for (unsigned long i = 0; i < Rows * Columns * NumberOfFrames; ++i) {
			maxValue = std::max(maxValue, *ptr++);
		}

		if (BitsAllocated == 16) {
			DoseGridScaling = maxValue / UINT16_MAX;
		}
		else { // BitsAllocated == 32
			DoseGridScaling = maxValue / UINT32_MAX;
		}

		// VR encoding size limit: Truncate decimals
		unsigned short decimals =
				_vrTypeMaxLength[_attributeTagVRType["DoseGridScaling"].second] - 1;
		double tmp = std::floor(DoseGridScaling);
		if (tmp == 0.0) {
			decimals--; // Pre-decimal zero
		}
		else {
			while (tmp > 0.0) {
				tmp = std::floor(tmp / 10.0);
				decimals--;
			}
		}
		tmp = std::pow(10.0, decimals);
		DoseGridScaling = std::floor(DoseGridScaling * tmp + 0.5) / tmp;
	}

	return DoseGridScaling;
}

void RTDoseWriter::InsertDataElement(DataSet &dataset,
																		 const std::string keyword,
																		 std::string value, unsigned int vm)
{
	Tag tag = _attributeTagVRType[keyword].first;
	VR::VRType vr = _attributeTagVRType[keyword].second;

	AdjustVRTypeString(vr, vm, value);

	DataElement de(tag);
	de.SetVR(vr);
	de.SetByteValue(value.c_str(), VL(value.length()));

	dataset.Insert(de);
}

void RTDoseWriter::ReplaceDataElement(DataSet &dataset,
																			const std::string keyword,
																			std::string value, unsigned int vm)
{
	Tag tag = _attributeTagVRType[keyword].first;
	VR::VRType vr = _attributeTagVRType[keyword].second;

	AdjustVRTypeString(vr, vm, value);

	DataElement de(tag);
	de.SetVR(vr);
	de.SetByteValue(value.c_str(), VL(value.length()));

	dataset.Replace(de);
}

void RTDoseWriter::InsertSequence(DataSet &dataset, const std::string keyword,
																	SequenceOfItems *sequence)
{
	Tag tag = _attributeTagVRType[keyword].first;
	VR::VRType vr = _attributeTagVRType[keyword].second;

	DataElement de(tag);
	de.SetVR(vr);
	de.SetValue(*sequence);
	de.SetVLToUndefined();

	dataset.Insert(de);
}

void RTDoseWriter::ReplaceStringSection(std::string &text,
																				const std::string oldSection,
																				const std::string newSection)
{
	int tmp;
	unsigned int len = newSection.length();
	while ((tmp = text.find(oldSection)) != std::string::npos) {
		text.replace(tmp, len, newSection);
	}
}

void RTDoseWriter::FormatDate(unsigned short &year, unsigned short &month,
															unsigned short &day, std::string &result)
{
	if (year && month && day) {
		char buffer[100];
		std::sprintf(buffer, "%04d%02d%02d", year, month, day);
		result = std::string(buffer);
		result.resize(8);
	}
	else {
		result = "";
	}
}

void RTDoseWriter::FormatTime(unsigned short &hours, unsigned short &minutes,
															unsigned short &seconds, std::string &result)
{
	if (hours && minutes && seconds) {
		char buffer[100];
		std::sprintf(buffer, "%02d%02d%02d", hours, minutes, seconds);
		result = std::string(buffer);
		result.resize(6);
	}
	else {
		result = "";
	}
}

void RTDoseWriter::AdjustVRTypeString(VR::VRType &vr, unsigned int vm,
																			std::string &value)
{
	// TODO: Confirm padding
	switch (vr) {
	/*case VR::AE:
    // Padding with trailing space
    if (value.length()%2 != 0) {
    value.append(" ");
    }
    break;*/
	case VR::AS: break;
	case VR::AT: break;
	//case VR::CS:
	case VR::DS:
	case VR::IS:
	case VR::SH:
	//case VR::ST:
	case VR::LT:
	case VR::UT:
		// Padding with trailing space
		if (value.length() % 2 != 0) {
			value.append(" ");
		}
		break;
	case VR::DA: break;
	case VR::DT: break;
	case VR::FL: break;
	case VR::FD: break;
	case VR::LO:
		ReplaceStringSection(value, std::string("\\"), std::string(""));
		// Padding with trailing space
		if (value.length() % 2 != 0) {
			value.append(" ");
		}
		break;
	case VR::OB:
		//case VR::UI:
		// Padding with trailing NULL byte
		if (value.length() % 2 != 0) {
			value.append("0");
		}
		break;
	case VR::OF: break;
	case VR::OW: break;
	case VR::PN:
		ReplaceStringSection(value, std::string(" "), std::string("^"));
		// Padding with trailing space
		if (value.length() % 2 != 0) {
			value.append(" ");
		}
		break;
	case VR::SL: break;
	case VR::SQ: break;
	case VR::SS: break;
	case VR::TM: break;
	case VR::UL: break;
	case VR::UN: break;
	case VR::US: break;
	}

	if (vm == 1) {
		unsigned short maxLen = _vrTypeMaxLength[vr];
		if (maxLen && value.length() > maxLen) {
			value.resize(maxLen);
		}
	}
}

template<typename T>
void RTDoseWriter::StringEncodeNumber(T number, std::string &value)
{
	unsigned short decimals = 0;
	T tmp = number;
	if (tmp < 0.0)
		tmp = -tmp;
	tmp -= int(tmp);
	while (tmp > 0.0) {
		tmp *= 10.0;
		tmp -= int(tmp);
		decimals++;
	}
	std::stringstream out;
	out.precision(decimals);
	out << std::fixed << number;
	value = out.str();
}

template<typename T>
void RTDoseWriter::ByteEncodeNumber(T number, std::string &value)
{
	unsigned short bytes = sizeof(T);
	char *buffer = new char[bytes];
	for (unsigned short i = 0; i < bytes; ++i) {
		buffer[bytes - i - 1] = number & 0xFF;
		number = number >> 8;
	}
	value = std::string(buffer);
	delete[] buffer;
}

template<typename T>
void RTDoseWriter::StringEncodeVector(const T *offsets, unsigned int count,
																			std::string &value)
{
	if (count <= 0)
		return;
	const std::string delimiter = "\\";
	value = "";

	for (unsigned int i = 0; i < count; ++i) {
		std::string numberStr;
		StringEncodeNumber(offsets[i], numberStr);
		value.append(numberStr);
		value.append(delimiter);
	}
	value.resize(value.size() - 1); // Remove last delimiter
}

template<typename T>
void RTDoseWriter::ByteEncodeVector(const T *offsets, unsigned int count,
																		std::string &value)
{
	// TODO: test or remove
	if (count <= 0)
		return;
	char delimiter = '\\';
	value = "";

	unsigned short bytes = sizeof(T);
	char *buffer = new char[bytes];
	for (unsigned int idx = 0; idx < count; ++idx) {
		T number = offsets[idx];
		for (unsigned short i = 0; i < bytes; ++i) {
			buffer[bytes - i - 1] = number & 0xFF;
			number = number >> 8;
		}
		std::string numberStr = std::string(buffer);
		numberStr = numberStr.erase(0, numberStr.find_first_not_of('0'));
		value.append(numberStr);
		value.append(delimiter);
	}
	value.resize(value.size() - 1); // Remove last delimiter

	delete[] buffer;
}

} // end namespace gdcm
