/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef GDCMRTDOSEWRITER_H
#define GDCMRTDOSEWRITER_H

#include "gdcmImageWriter.h"
#include <map>

namespace gdcm
{

enum PatientSexEnum {
  Male,
  Female,
  Other,
  Unspecified
};

enum DoseUnitsEnum {
  Gray,
  Relative
};

enum DoseTypeEnum {
  Physical,
  Effective,
  Error
};

enum DoseSummationTypeEnum {
  Plan,
  MultiPlan,
  Fraction,
  Beam,
  Brachy,
  ControlPoint
};

enum CharacterSetEnum {
  Latin1
  // TODO: Support other character sets
};

/**
* \brief RTDoseWriter
*/
//class GDCM_EXPORT RTDoseWriter : public ImageWriter // TODO: Uncomment!!
class RTDoseWriter : public ImageWriter
{
public:
  RTDoseWriter();
  ~RTDoseWriter();

  /// Write
  bool Write(); // Execute()

  std::string GenerateUID();

  // Only mandatory modules included (see Table A.18.3-1 - RT Dose IOD Modules)

  // Patient Module (C7.1.1)
  // --------------------------------------------------------------

  // Patient's Name (Type = 2, VM = 1)
  void SetPatientName(std::string value) { PatientName = value; };
  std::string GetPatientName() { return PatientName; };
  // Patient ID (Type = 2, VM = 1)
  void SetPatientID(std::string value) { PatientID = value; };
  std::string GetPatientID() { return PatientID; };
  // Issuer of Patient ID Macro (Table 10-18)
  // Patient's Birth Date (Type = 2, VM = 1)
  void SetPatientBirthDate(const unsigned short* values) {
	  for (unsigned short i = 0; i < 3; ++i) {
		  PatientBirthDate[i] = values[i];
		}
	};
  unsigned short* GetPatientBirthDate() { return PatientBirthDate; };
  // Patient's Sex (Type = 2, VM = 1)
  void SetPatientSex(PatientSexEnum value) { PatientSex = value; };
  PatientSexEnum GetPatientSex() { return PatientSex; };
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
  void SetStudyInstanceUID(std::string value) { StudyInstanceUID = value; };
  std::string GetStudyInstanceUID() { return StudyInstanceUID; };
  // Study Date (Type = 2, VM = 1)
  void SetStudyDate(const unsigned short* values) {
	  for (unsigned short i = 0; i < 3; ++i) {
		  StudyDate[i] = values[i];
		}
	};
  unsigned short* GetStudyDate() { return StudyDate; };
  // Study Time (Type = 2, VM = 1)
  void SetStudyTime(const unsigned short* values) {
	  for (unsigned short i = 0; i < 3; ++i) {
		  StudyTime[i] = values[i];
		}
	};
  unsigned short* GetStudyTime() { return StudyTime; };
  // Referring Physician's Name (Type = 2, VM = 1)
  void SetReferringPhysicianName(std::string value) { ReferringPhysicianName = value; };
  std::string GetReferringPhysicianName() { return ReferringPhysicianName; };
  // Referring Physician Identification Sequence (Type = 3, VM = ?)
  // > Person Identification Macro (Table 10-1)
  // Study ID (Type = 1, VM = 1)
  void SetStudyID(std::string value) { StudyID = value; };
  std::string GetStudyID() { return StudyID; };
  // Accession Number (Type = 2, VM = 1)
  void SetAccessionNumber(std::string value) { AccessionNumber = value; };
  std::string GetAccessionNumber() { return AccessionNumber; };
  // Issuer of Accession Number Sequence (Type = 3, VM = ?)
  // > HL7v2 Hierarchic Designator Macro (Table 10-17)
  // Study Description (Type = 3, VM = 1)
  void SetStudyDescription(std::string value) {
    StudyDescription = value;
    StudyDescriptionExists = true;
  };
  std::string GetStudyDescription() { return StudyDescription; };
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
  std::string GetReferencedStudyClassUID() { return std::string("1.2.840.10008.3.1.2.3.2"); }; // Study Componenet Management SOP Class
  void SetReferencedStudyInstanceUID(unsigned short idx, std::string value) { ReferencedStudySequence[idx] = value; };
  void AddReferencedStudyInstanceUID(std::string value) { ReferencedStudySequence.push_back(value); };
  std::string GetReferencedStudyInstanceUID(unsigned short idx) { return ReferencedStudySequence[idx]; };
  unsigned short GetReferencedStudyInstanceUIDCount() { return ReferencedStudySequence.size(); };
  // Procedure Code Sequence (Type = 3, VM = ?)
  // > Code Sequence Macro (Table 8.8-1)
  // Reason For Performed Procedure Code Sequence (Type = 3, VM = ?)
  // > Code Sequence Macro (Table 8.8-1)

  // RT Series Module (C8.8.1)
  // --------------------------------------------------------------
	
  // Modality (Type = 1, VM = 1) // Fixed value "RTDOSE"
  std::string GetModality() { return std::string("RTDOSE"); };
  // Series Instance UID (Type = 1, VM = 1)
  void SetSeriesInstanceUID(std::string value) { SeriesInstanceUID = value; };
  std::string GetSeriesInstanceUID() { return SeriesInstanceUID; };
  // Series Number (Type = 2, VM = 1)
  void SetSeriesNumber(unsigned short value) { SeriesNumber = value; };
  unsigned short GetSeriesNumber() { return SeriesNumber; };
  // Series Description (Type = 3, VM = ?)
  // Series Description Code Sequence (Type = 3, VM = ?)
  // > Code Sequence Macro (Table 8.8-1)
  // Operators' Name (Type = 2, VM = 1-n)
  void SetOperatorsName(unsigned short idx, std::string value) { OperatorsName[idx] = value; };
  void AddOperatorsName(std::string value) { OperatorsName.push_back(value); };
  std::string GetOperatorsName(unsigned short idx) { return OperatorsName[idx]; };
  unsigned short GetOperatorsNameCount() { return OperatorsName.size(); };
  // Referenced Performed Procedure Step Sequence (Type = 3, VM = ?)
  // > SOP Instance Reference Macro (Table 10-11)
  // Request Attributes Sequence (Type = 3, VM = ?)
  // > Request Attributes Macro (Table 10-11)

  // Frame Of Reference Module (C7.4.1)
  // --------------------------------------------------------------

  // Frame of Reference UID (Type = 1, VM = 1)
  void SetFrameOfReferenceUID(std::string value) { FrameOfReferenceUID = value; };
  std::string GetFrameOfReferenceUID() { return FrameOfReferenceUID; };
  // Position Reference Indicator (Type = 2, VM = 1)
  void SetPositionReferenceIndicator(std::string value) { PositionReferenceIndicator = value; };
  std::string GetPositionReferenceIndicator() { return PositionReferenceIndicator; };

  // General Equipment Module (C7.5.1)
  // --------------------------------------------------------------

  // Manufacturer (Type = 2, VM = 1)
  void SetManufacturer(std::string value) { Manufacturer = value; };
  std::string GetManufacturer() { return Manufacturer; };
  // Institution Name (Type = 3, VM = ?)
  // Institution Address (Type = 3, VM = ?)
  // Station Name (Type = 3, VM = 1)
  void SetStationName(std::string value) {
    StationName = value;
    StationNameExists = true;
  };
  std::string GetStationName() { return StationName; };
  // Institutional Department Name (Type = 3, VM = ?)
  // Manufacturer's Model Name (Type = 3, VM = 1)
  void SetManufacturersModelName(std::string value) {
    ManufacturersModelName = value;
    ManufacturersModelNameExists = true;
  };
  std::string GetManufacturersModelName() { return ManufacturersModelName; };
  // Device Serial Number (Type = 3, VM = ?)
  // Software Versions (Type = 3, VM = 1-n)
  void SetSoftwareVersions(unsigned short idx, std::string value) { SoftwareVersions[idx] = value; };
  void AddSoftwareVersions(std::string value) { SoftwareVersions.push_back(value); };
  std::string GetSoftwareVersions(unsigned short idx) { return SoftwareVersions[idx]; };
  unsigned short GetSoftwareVersionsCount() { return SoftwareVersions.size(); };
  // Gantry ID (Type = 3, VM = ?)
  // Spatial Resolution (Type = 3, VM = ?)
  // Date of Last Calibration (Type = 3, VM = ?)
  // Time of Last Calibration (Type = 3, VM = ?)
  // Pixel Padding Value (Type = 1C, VM = ?)

  // General Image Module (C7.6.1)
  // --------------------------------------------------------------

  // Instance Number (Type = 2, VM = 1)
  void SetInstanceNumber(unsigned short value) { InstanceNumber = value; };
  unsigned short GetInstanceNumber() { return InstanceNumber; };
  // Patient Orientation (Type = 2C, VM = ?)
  // Content Date (Type = 2C, VM = 1)
  void SetContentDate(const unsigned short* values) {
	  for (unsigned short i = 0; i < 3; ++i) {
		  ContentDate[i] = values[i];
		}
    ContentDateExists = true;
  };
  unsigned short* GetContentDate() { return ContentDate; };
  // Content Time (Type = 2C, VM = 1)
  void SetContentTime(const unsigned short* values) {
    for (unsigned short i = 0; i < 3; ++i) {
		  ContentTime[i] = values[i];
		}
    ContentTimeExists = true;
  };
  unsigned short* GetContentTime() { return ContentTime; };
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
  void SetPixelSpacing(const double* values) {
	  for (unsigned short i = 0; i < 2; ++i) {
		  PixelSpacing[i] = values[i];
		}
	};
  double* GetPixelSpacing() { return PixelSpacing; };
  // Image Orientation (Patient) (Type = 1, VM = 6) // TODO
  void SetImageOrientationPatient(const double* values) {
	  for (unsigned short i = 0; i < 6; ++i) {
		  ImageOrientationPatient[i] = values[i];
		}
	};
  double* GetImageOrientationPatient() { return ImageOrientationPatient; };
  // Image Position (Patient) (Type = 1, VM = 3) // TODO
  void SetImagePositionPatient(const double* values) {
	  for (unsigned short i = 0; i < 3; ++i) {
		  ImagePositionPatient[i] = values[i];
		}
	};
  double* GetImagePositionPatient() { return ImagePositionPatient; };
  // Slice Thickness (Type = 2, VM = 1)
  void SetSliceThickness(double value) { SliceThickness = value; };
  double GetSliceThickness() { return SliceThickness; };
  // Slice Location (Type = 3, VM = ?)

  // Image Pixel Module (C7.6.3)
  // --------------------------------------------------------------
	
  // Samples per Pixel (Type = 1, VM = 1) // Fixed value 1 (monochrome)
  unsigned short GetSamplesPerPixel() { return 1; };
  // Photometric Interpretation (Type = 1, VM = 1) // Fixed value "MONOCHROME2"
  std::string GetPhotometricInterpretation() { return std::string("MONOCHROME2"); };
  // Rows (Type = 1, VM = 1)
  void SetRows(unsigned int value) { Rows = value; };
  unsigned int GetRows() { return Rows; };
  // Columns (Type = 1, VM = 1)
  void SetColumns(unsigned int value) { Columns = value; };
  unsigned int GetColumns() { return Columns; };
  // Bits Allocated (Type = 1, VM = 1)
  void SetBitsAllocated(unsigned short value) { BitsAllocated = value; };
  unsigned short GetBitsAllocated() { return BitsAllocated; };
  // Bits Stored (Type = 1, VM = 1)
  unsigned short GetBitsStored() { return BitsAllocated; };
  // High Bit (Type = 1, VM = 1)
  unsigned short GetHighBit() { return BitsAllocated-1; };
  // Pixel Representation (Type = 1, VM = 1)
  unsigned short GetPixelRepresentation() {
    if(DoseType == Error) {
      return 1;
    } else {
      return 0;
    }
  };
  // Pixel Data (Type = 1C, VM = 1)
  void SetPixelData(float* values) {
    PixelData = values;
    DoseGridScaling = 0.0;
  };
  float* GetPixelData() { return PixelData; };
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
  void SetNumberOfFrames(unsigned int value) { NumberOfFrames = value; };
  unsigned int GetNumberOfFrames() { return NumberOfFrames; };
  // Frame Increment Pointer (Type = 1, VM = 1) // Fixed value Tag(3004,000c)
  std::string GetFrameIncrementPointer() {
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

  // Samples per Pixel (Type = 1C, VM = 1)          // See Image Pixel Module (C7.6.3)
  // Photometric Interpretation (Type = 1C, VM = 1) // See Image Pixel Module (C7.6.3)
  // Bits Allocated (Type = 1C, VM = 1)             // See Image Pixel Module (C7.6.3)
  // Bits Stored (Type = 1C, VM = 1)                // See Image Pixel Module (C7.6.3)
  // High Bit (Type = 1C, VM = 1)                   // See Image Pixel Module (C7.6.3)
  // Pixel Representation (Type = 1C, VM = 1)       // See Image Pixel Module (C7.6.3)
  // Dose Units (Type = 1, VM = 1)
  void SetDoseUnits(DoseUnitsEnum value) { DoseUnits = value; };
  DoseUnitsEnum GetDoseUnits() { return DoseUnits; };
  // Dose Type (Type = 1, VM = 1)
  void SetDoseType(DoseTypeEnum value) { DoseType = value; };
  DoseTypeEnum GetDoseType() { return DoseType; };
  // Instance Number (Type = 3, VM = ?)
  // Dose Comment (Type = 3, VM = ?)
  // Normalization Point (Type = 3, VM = 3)
  void SetNormalizationPoint(const double* values) {
    for (unsigned short i = 0; i < 3; ++i) {
		  NormalizationPoint[i] = values[i];
		}
    NormalizationPointExists = true;
  };
  double* GetNormalizationPoint() { return NormalizationPoint; };
  // Dose Summation Type (Type = 1, VM = 1)
  void SetDoseSummationType(DoseSummationTypeEnum value) { DoseSummationType = value; };
  DoseSummationTypeEnum GetDoseSummationType() { return DoseSummationType; };
  // Referenced RT Plan Sequence (Type = 1C, VM = 1-n)
  // > SOP Instance Reference Macro (Table 10-11)
  std::string GetReferencedRTPlanClassUID() { return std::string("1.2.840.10008.5.1.4.1.1.481.5"); }; // Radiation Therapy Plan Storage
  void SetReferencedRTPlanInstanceUID(unsigned short idx, std::string value) { ReferencedRTPlanSequence[idx] = value; };
  void AddReferencedRTPlanInstanceUID(std::string value) { ReferencedRTPlanSequence.push_back(value); };
  std::string GetReferencedRTPlanInstanceUID(unsigned short idx) { return ReferencedRTPlanSequence[idx]; };
  unsigned short GetReferencedRTPlanInstanceUIDCount() { return ReferencedRTPlanSequence.size(); };
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
  void SetGridFrameOffsetVector(double* values) { GridFrameOffsetVector = values; };
  double* GetGridFrameOffsetVector() { return GridFrameOffsetVector; };
  // Dose Grid Scaling (Type = 1C, VM = 1)
  double GetDoseGridScaling();
  // Tissue Heterogeneity Correction (Type = 3, VM = ?)

  // SOP Common Module (C12.1)
  // --------------------------------------------------------------

  // SOP Class UID (Type = 1, VM = 1) // Radiation Therapy Dose Storage
  std::string GetSOPClassUID() { return std::string("1.2.840.10008.5.1.4.1.1.481.2"); };
  // SOP Instance UID (Type = 1, VM = 1)
  void SetSOPInstanceUID(std::string value) { SOPInstanceUID = value; };
  std::string GetSOPInstanceUID() { return SOPInstanceUID; };
  // Specific Character Set (Type = 1C, VM = 1-n)
  void SetSpecificCharacterSet(unsigned short idx, CharacterSetEnum value) { SpecificCharacterSet[idx] = value; };
  void AddSpecificCharacterSet(CharacterSetEnum value) { SpecificCharacterSet.push_back(value); };
  CharacterSetEnum GetSpecificCharacterSet(unsigned short idx) { return SpecificCharacterSet[idx]; };
  unsigned short GetSpecificCharacterSetCount() { return SpecificCharacterSet.size(); };
  // Instance Creation Date (Type = 3, VM = 1)
  void SetInstanceCreationDate(const unsigned short* values) {
    for (unsigned short i = 0; i < 3; ++i) {
		  InstanceCreationDate[i] = values[i];
		}
    InstanceCreationDateExists = true;
  };
  unsigned short* GetInstanceCreationDate() { return InstanceCreationDate; };
  // Instance Creation Time (Type = 3, VM = 1)
  void SetInstanceCreationTime(const unsigned short* values) {
    for (unsigned short i = 0; i < 3; ++i) {
		  InstanceCreationTime[i] = values[i];
		}
    InstanceCreationTimeExists = true;
  };
  unsigned short* GetInstanceCreationTime() { return InstanceCreationTime; };
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
  void AdjustVRTypeString(VR::VRType &vr, unsigned int vm, std::string &value);
  void FormatDate(unsigned short &year, unsigned short &month, unsigned short &day, std::string &result);
  void FormatTime(unsigned short &hours, unsigned short &minutes, unsigned short &seconds, std::string &result);
  template <typename T> void StringEncodeNumber(T number, std::string &value);
  template <typename T> void ByteEncodeNumber(T number, std::string &value);
  template <typename T> void StringEncodeVector(const T *offsets, unsigned int count, std::string &value);
  template <typename T> void ByteEncodeVector(const T *offsets, unsigned int count, std::string &value);
  void ReplaceStringSection(std::string &text, const std::string oldSection, const std::string newSection);
  void InsertDataElement(DataSet &dataset, const std::string keyword, std::string value, unsigned int vm = 1);
  void ReplaceDataElement(DataSet &dataset, const std::string keyword, std::string value, unsigned int vm = 1);
  void InsertSequence(DataSet &dataset, const std::string keyword, SequenceOfItems *sequence);

private:
  void CreateVRTypeMaxLengthMap();
  void CreateAttributeTagVRTypeMap();

  void WritePatientModule(DataSet &ds);
  void WriteGeneralStudyModule(DataSet &ds);
  void WriteRTSeriesModule(DataSet &ds);
  void WriteFrameOfReferenceModule(DataSet &ds);
  void WriteGeneralEquipmentModule(DataSet &ds);
  void WriteGeneralImageModule(DataSet &ds);
  void WriteImagePlaneModule(DataSet &ds);
  void WriteImagePixelModule(DataSet &ds, Image & pixeldata);
  void WriteMultiFrameModule(DataSet &ds);
  void WriteRTDoseModule(DataSet &ds);
  void WriteSOPCommonModule(DataSet &ds);

protected:
  typedef std::pair<Tag, VR::VRType> TagVRTypePair;
  std::map<VR::VRType, unsigned short> _vrTypeMaxLength;    // Maps a VR type to its maximal byte length
  std::map<std::string, TagVRTypePair> _attributeTagVRType; // Maps an attribute keyword to its tag and VR type

private:
  char *QuantizedPixelData;

  std::string PatientName;
  std::string PatientID;
  unsigned short PatientBirthDate[3]; // Year, Month, Day
  PatientSexEnum PatientSex;
  std::string StudyInstanceUID;
  unsigned short StudyDate[3]; // Year, Month, Day
  unsigned short StudyTime[3]; // Hour, Minute, Second
  std::string ReferringPhysicianName;
  std::string StudyID;
  std::string AccessionNumber;
  std::string StudyDescription;
  std::vector<std::string> ReferencedStudySequence;
  bool StudyDescriptionExists;
  std::string SeriesInstanceUID;
  unsigned short SeriesNumber;
  std::vector<std::string> OperatorsName;
  std::string FrameOfReferenceUID;
  std::string PositionReferenceIndicator;
  std::string Manufacturer;
  std::string StationName;
  bool StationNameExists;
  std::string ManufacturersModelName;
  std::vector<std::string> SoftwareVersions;
  bool ManufacturersModelNameExists;
  unsigned short InstanceNumber;
  unsigned short ContentDate[3]; // Year, Month, Day
  bool ContentDateExists;
  unsigned short ContentTime[3]; // Hour, Minute, Second
  bool ContentTimeExists;
  double PixelSpacing[2];
  double ImageOrientationPatient[6];
  double ImagePositionPatient[3];
  double SliceThickness;
  unsigned short Rows;
  unsigned short Columns;
  unsigned short BitsAllocated;
  float* PixelData;
  unsigned int NumberOfFrames;
  DoseUnitsEnum DoseUnits;
  DoseTypeEnum DoseType;
  double NormalizationPoint[3];
  bool NormalizationPointExists;
  DoseSummationTypeEnum DoseSummationType;
  std::vector<std::string> ReferencedRTPlanSequence;
  double* GridFrameOffsetVector;
  double DoseGridScaling;
  std::string SOPClassUID;
  std::string SOPInstanceUID;
  std::vector<CharacterSetEnum> SpecificCharacterSet;
  unsigned short InstanceCreationDate[3]; // Year, Month, Day
  bool InstanceCreationDateExists;
  unsigned short InstanceCreationTime[3]; // Hour, Minute, Second
  bool InstanceCreationTimeExists;
};

} // end namespace gdcm

#endif //GDCMRTDOSEWRITER_H