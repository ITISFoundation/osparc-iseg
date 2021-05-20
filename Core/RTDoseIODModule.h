/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "iSegCore.h"

#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4267)

namespace iseg {

enum ePatientSexEnum { Male,
	Female,
	Other,
	Unspecified };

enum eDoseUnitsEnum { Gray,
	Relative };

enum eDoseTypeEnum { Physical,
	Effective,
	Error };

enum eDoseSummationTypeEnum {
	Plan,
	MultiPlan,
	Fraction,
	Beam,
	Brachy,
	ControlPoint
};

enum eCharacterSetEnum {
	Latin1
	// TODO: Support other character sets
};

class ISEG_CORE_API RTDoseIODModule
{
public:
	RTDoseIODModule();
	~RTDoseIODModule();

	std::string GenerateUID();

	// Only mandatory modules included (see Table A.18.3-1 - RT Dose IOD Modules)

	// Patient Module (C7.1.1)
	// --------------------------------------------------------------

	// Patient's Name (Type = 2, VM = 1)
	void SetPatientName(std::string value) { m_PatientName = value; };
	std::string GetPatientName() { return m_PatientName; };
	// Patient ID (Type = 2, VM = 1)
	void SetPatientID(std::string value) { m_PatientId = value; };
	std::string GetPatientID() { return m_PatientId; };
	// Issuer of Patient ID Macro (Table 10-18)
	// Patient's Birth Date (Type = 2, VM = 1)
	void SetPatientBirthDate(const unsigned short* values)
	{
		for (unsigned short i = 0; i < 3; ++i)
		{
			m_PatientBirthDate[i] = values[i];
		}
	};
	unsigned short* GetPatientBirthDate() { return m_PatientBirthDate; };
	// Patient's Sex (Type = 2, VM = 1)
	void SetPatientSex(ePatientSexEnum value) { m_PatientSex = value; };
	ePatientSexEnum GetPatientSex() { return m_PatientSex; };
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
	void SetStudyInstanceUID(std::string value) { m_StudyInstanceUid = value; };
	std::string GetStudyInstanceUID() { return m_StudyInstanceUid; };
	// Study Date (Type = 2, VM = 1)
	void SetStudyDate(const unsigned short* values)
	{
		for (unsigned short i = 0; i < 3; ++i)
		{
			m_StudyDate[i] = values[i];
		}
	};
	unsigned short* GetStudyDate() { return m_StudyDate; };
	// Study Time (Type = 2, VM = 1)
	void SetStudyTime(const unsigned short* values)
	{
		for (unsigned short i = 0; i < 3; ++i)
		{
			m_StudyTime[i] = values[i];
		}
	};
	unsigned short* GetStudyTime() { return m_StudyTime; };
	// Referring Physician's Name (Type = 2, VM = 1)
	void SetReferringPhysicianName(std::string value)
	{
		m_ReferringPhysicianName = value;
	};
	std::string GetReferringPhysicianName() { return m_ReferringPhysicianName; };
	// Referring Physician Identification Sequence (Type = 3, VM = ?)
	// > Person Identification Macro (Table 10-1)
	// Study ID (Type = 1, VM = 1)
	void SetStudyID(std::string value) { m_StudyId = value; };
	std::string GetStudyID() { return m_StudyId; };
	// Accession Number (Type = 2, VM = 1)
	void SetAccessionNumber(std::string value) { m_AccessionNumber = value; };
	std::string GetAccessionNumber() { return m_AccessionNumber; };
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

	// Modality (Type = 1, VM = 1) // Fixed value "RTDOSE"
	std::string GetModality() { return std::string("RTDOSE"); };
	// Series Instance UID (Type = 1, VM = 1)
	void SetSeriesInstanceUID(std::string value) { m_SeriesInstanceUid = value; };
	std::string GetSeriesInstanceUID() { return m_SeriesInstanceUid; };
	// Series Number (Type = 2, VM = 1)
	void SetSeriesNumber(unsigned short value) { m_SeriesNumber = value; };
	unsigned short GetSeriesNumber() const { return m_SeriesNumber; };
	// Series Description (Type = 3, VM = ?)
	// Series Description Code Sequence (Type = 3, VM = ?)
	// > Code Sequence Macro (Table 8.8-1)
	// Operators' Name (Type = 2, VM = 1-n)
	void SetOperatorsName(unsigned short idx, std::string value)
	{
		m_OperatorsName[idx] = value;
	};
	void AddOperatorsName(std::string value)
	{
		m_OperatorsName.push_back(value);
	};
	std::string GetOperatorsName(unsigned short idx)
	{
		return m_OperatorsName[idx];
	};
	unsigned short GetOperatorsNameCount() { return m_OperatorsName.size(); };
	// Referenced Performed Procedure Step Sequence (Type = 3, VM = ?)
	// > SOP Instance Reference Macro (Table 10-11)
	// Request Attributes Sequence (Type = 3, VM = ?)
	// > Request Attributes Macro (Table 10-11)

	// Frame Of Reference Module (C7.4.1)
	// --------------------------------------------------------------

	// Frame of Reference UID (Type = 1, VM = 1)
	void SetFrameOfReferenceUID(std::string value)
	{
		m_FrameOfReferenceUid = value;
	};
	std::string GetFrameOfReferenceUID() { return m_FrameOfReferenceUid; };
	// Position Reference Indicator (Type = 2, VM = 1)
	void SetPositionReferenceIndicator(std::string value)
	{
		m_PositionReferenceIndicator = value;
	};
	std::string GetPositionReferenceIndicator()
	{
		return m_PositionReferenceIndicator;
	};

	// General Equipment Module (C7.5.1)
	// --------------------------------------------------------------

	// Manufacturer (Type = 2, VM = 1)
	void SetManufacturer(std::string value) { m_Manufacturer = value; };
	std::string GetManufacturer() { return m_Manufacturer; };
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
	void SetInstanceNumber(unsigned short value) { m_InstanceNumber = value; };
	unsigned short GetInstanceNumber() const { return m_InstanceNumber; };
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

	// Pixel Spacing (Type = 1, VM = 2) // TODO
	void SetPixelSpacing(const double* values)
	{
		for (unsigned short i = 0; i < 2; ++i)
		{
			m_PixelSpacing[i] = values[i];
		}
	};
	double* GetPixelSpacing() { return m_PixelSpacing; };
	// Image Orientation (Patient) (Type = 1, VM = 6) // TODO
	void SetImageOrientationPatient(const double* values)
	{
		for (unsigned short i = 0; i < 6; ++i)
		{
			m_ImageOrientationPatient[i] = values[i];
		}
	};
	double* GetImageOrientationPatient() { return m_ImageOrientationPatient; };
	// Image Position (Patient) (Type = 1, VM = 3) // TODO
	void SetImagePositionPatient(const double* values)
	{
		for (unsigned short i = 0; i < 3; ++i)
		{
			m_ImagePositionPatient[i] = values[i];
		}
	};
	double* GetImagePositionPatient() { return m_ImagePositionPatient; };
	// Slice Thickness (Type = 2, VM = 1)
	void SetSliceThickness(double value) { m_SliceThickness = value; };
	double GetSliceThickness() const { return m_SliceThickness; };
	// Slice Location (Type = 3, VM = ?)

	// Image Pixel Module (C7.6.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1, VM = 1) // Fixed value 1 (monochrome)
	unsigned short GetSamplesPerPixel() { return 1; };
	// Photometric Interpretation (Type = 1, VM = 1) // Fixed value "MONOCHROME2"
	std::string GetPhotometricInterpretation()
	{
		return std::string("MONOCHROME2");
	};
	// Rows (Type = 1, VM = 1)
	void SetRows(unsigned int value) { m_Rows = value; };
	unsigned int GetRows() const { return m_Rows; };
	// Columns (Type = 1, VM = 1)
	void SetColumns(unsigned int value) { m_Columns = value; };
	unsigned int GetColumns() const { return m_Columns; };
	// Bits Allocated (Type = 1, VM = 1)
	void SetBitsAllocated(unsigned short value) { m_BitsAllocated = value; };
	unsigned short GetBitsAllocated() const { return m_BitsAllocated; };
	// Bits Stored (Type = 1, VM = 1)
	unsigned short GetBitsStored() const { return m_BitsAllocated; };
	// High Bit (Type = 1, VM = 1)
	unsigned short GetHighBit() const { return m_BitsAllocated - 1; };
	// Pixel Representation (Type = 1, VM = 1)
	unsigned short GetPixelRepresentation()
	{
		if (m_DoseType == Error)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	};
	// Pixel Data (Type = 1C, VM = 1)
	void SetPixelData(float* values) { m_PixelData = values; };
	float* GetPixelData() { return m_PixelData; };
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
	void SetNumberOfFrames(unsigned int value) { m_NumberOfFrames = value; };
	unsigned int GetNumberOfFrames() const { return m_NumberOfFrames; };
	// Frame Increment Pointer (Type = 1, VM = 1) // Fixed value Tag(3004,000c)
	std::string GetFrameIncrementPointer()
	{
		unsigned short group = 0x3004;
		unsigned short element = 0x000c;
		char buffer[4];
		buffer[0] = group & 0xFF;
		buffer[1] = (group >> 8) & 0xFF;
		buffer[2] = element & 0xFF;
		buffer[3] = (element >> 8) & 0xFF;
		return std::string(buffer);
	};

	// RT Dose Module (C8.8.3)
	// --------------------------------------------------------------

	// Samples per Pixel (Type = 1C, VM = 1)			// See Image Pixel Module (C7.6.3)
	// Photometric Interpretation (Type = 1C, VM = 1)	// See Image Pixel Module (C7.6.3)
	// Bits Allocated (Type = 1C, VM = 1)				// See Image Pixel Module (C7.6.3)
	// Bits Stored (Type = 1C, VM = 1)					// See Image Pixel Module (C7.6.3)
	// High Bit (Type = 1C, VM = 1)						// See Image Pixel Module (C7.6.3)
	// Pixel Representation (Type = 1C, VM = 1)			// See Image Pixel Module (C7.6.3)
	// Dose Units (Type = 1, VM = 1)
	void SetDoseUnits(eDoseUnitsEnum value) { m_DoseUnits = value; };
	eDoseUnitsEnum GetDoseUnits() { return m_DoseUnits; };
	// Dose Type (Type = 1, VM = 1)
	void SetDoseType(eDoseTypeEnum value) { m_DoseType = value; };
	eDoseTypeEnum GetDoseType() { return m_DoseType; };
	// Instance Number (Type = 3, VM = ?)
	// Dose Comment (Type = 3, VM = ?)
	// Normalization Point (Type = 3, VM = ?)
	// Dose Summation Type (Type = 1, VM = 1)
	void SetDoseSummationType(eDoseSummationTypeEnum value)
	{
		m_DoseSummationType = value;
	};
	eDoseSummationTypeEnum GetDoseSummationType() { return m_DoseSummationType; };
	// Referenced RT Plan Sequence (Type = 1C, VM = 1-n)
	// > SOP Instance Reference Macro (Table 10-11)
	std::string GetReferencedRTPlanClassUID()
	{
		return std::string("1.2.840.10008.5.1.4.1.1.481.5");
	}; // Radiation Therapy Plan Storage
	void SetReferencedRTPlanInstanceUID(unsigned short idx, std::string value)
	{
		m_ReferencedRtPlanSequence[idx] = value;
	};
	void AddReferencedRTPlanInstanceUID(std::string value)
	{
		m_ReferencedRtPlanSequence.push_back(value);
	};
	std::string GetReferencedRTPlanInstanceUID(unsigned short idx)
	{
		return m_ReferencedRtPlanSequence[idx];
	};
	unsigned short GetReferencedRTPlanInstanceUIDCount()
	{
		return m_ReferencedRtPlanSequence.size();
	};
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
	void SetGridFrameOffsetVector(double* values)
	{
		m_GridFrameOffsetVector = values;
	};
	double* GetGridFrameOffsetVector() { return m_GridFrameOffsetVector; };
	// Dose Grid Scaling (Type = 1C, VM = 1)
	double GetDoseGridScaling();
	// Tissue Heterogeneity Correction (Type = 3, VM = ?)

	// SOP Common Module (C12.1)
	// --------------------------------------------------------------

	// SOP Class UID (Type = 1, VM = 1) // Radiation Therapy Dose Storage
	std::string GetSOPClassUID()
	{
		return std::string("1.2.840.10008.5.1.4.1.1.481.2");
	};
	// SOP Instance UID (Type = 1, VM = 1)
	void SetSOPInstanceUID(std::string value) { m_SopInstanceUid = value; };
	std::string GetSOPInstanceUID() { return m_SopInstanceUid; };
	// Specific Character Set (Type = 1C, VM = 1-n)
	void SetSpecificCharacterSet(unsigned short idx, eCharacterSetEnum value)
	{
		m_SpecificCharacterSet[idx] = value;
	};
	void AddSpecificCharacterSet(eCharacterSetEnum value)
	{
		m_SpecificCharacterSet.push_back(value);
	};
	eCharacterSetEnum GetSpecificCharacterSet(unsigned short idx)
	{
		return m_SpecificCharacterSet[idx];
	};
	unsigned short GetSpecificCharacterSetCount()
	{
		return m_SpecificCharacterSet.size();
	};
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

protected:
	std::string m_PatientName;
	std::string m_PatientId;
	unsigned short m_PatientBirthDate[3]; // Year, Month, Day
	ePatientSexEnum m_PatientSex;
	std::string m_StudyInstanceUid;
	unsigned short m_StudyDate[3]; // Year, Month, Day
	unsigned short m_StudyTime[3]; // Hour, Minute, Second
	std::string m_ReferringPhysicianName;
	std::string m_StudyId;
	std::string m_AccessionNumber;
	std::string m_SeriesInstanceUid;
	unsigned short m_SeriesNumber;
	std::vector<std::string> m_OperatorsName;
	std::string m_FrameOfReferenceUid;
	std::string m_PositionReferenceIndicator;
	std::string m_Manufacturer;
	unsigned short m_InstanceNumber;
	double m_PixelSpacing[2];
	double m_ImageOrientationPatient[6];
	double m_ImagePositionPatient[3];
	double m_SliceThickness;
	unsigned short m_Rows;
	unsigned short m_Columns;
	unsigned short m_BitsAllocated;
	float* m_PixelData;
	unsigned int m_NumberOfFrames;
	eDoseUnitsEnum m_DoseUnits;
	eDoseTypeEnum m_DoseType;
	eDoseSummationTypeEnum m_DoseSummationType;
	std::vector<std::string> m_ReferencedRtPlanSequence;
	double* m_GridFrameOffsetVector;
	std::string m_SopClassUid;
	std::string m_SopInstanceUid;
	std::vector<eCharacterSetEnum> m_SpecificCharacterSet;
};

} // namespace iseg
#pragma warning(pop)
