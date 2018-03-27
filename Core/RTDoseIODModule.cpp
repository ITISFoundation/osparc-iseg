/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "RTDoseIODModule.h"
#include "Precompiled.h"
#include "gdcmAttribute.h"
#include "gdcmUIDGenerator.h"
#include <string>

RTDoseIODModule::RTDoseIODModule()
{
	PatientName = "";
	PatientID = "";
	PatientBirthDate[0] = 0;
	PatientBirthDate[1] = 0;
	PatientBirthDate[2] = 0;
	PatientSex = Unspecified;
	StudyInstanceUID = GenerateUID();
	StudyDate[0] = 0;
	StudyDate[1] = 0;
	StudyDate[2] = 0;
	StudyTime[0] = 0;
	StudyTime[1] = 0;
	StudyTime[2] = 0;
	ReferringPhysicianName = "";
	StudyID = "";
	AccessionNumber = "";
	SeriesInstanceUID = GenerateUID();
	SeriesNumber = 0;
	FrameOfReferenceUID = GenerateUID();
	PositionReferenceIndicator = "";
	Manufacturer = "";
	InstanceNumber = 0;
	PixelSpacing[0] = 0;
	PixelSpacing[1] = 0;
	ImageOrientationPatient[0] = 0.0;
	ImageOrientationPatient[1] = 0.0;
	ImageOrientationPatient[2] = 0.0;
	ImageOrientationPatient[3] = 0.0;
	ImageOrientationPatient[4] = 0.0;
	ImageOrientationPatient[5] = 0.0;
	ImagePositionPatient[0] = 0.0;
	ImagePositionPatient[1] = 0.0;
	ImagePositionPatient[2] = 0.0;
	SliceThickness = 0.0;
	Rows = 0;
	Columns = 0;
	BitsAllocated = 0;
	PixelData = NULL;
	NumberOfFrames = 0;
	DoseUnits = Gray;
	DoseType = Physical;
	DoseSummationType = Plan;
	GridFrameOffsetVector = NULL;
	SOPInstanceUID = GenerateUID();
}

RTDoseIODModule::~RTDoseIODModule() {}

std::string RTDoseIODModule::GenerateUID()
{
	gdcm::UIDGenerator uidGen;
	return std::string(uidGen.Generate());
}

double RTDoseIODModule::GetDoseGridScaling()
{
	float maxVal = 0.0f;
	float *ptr = &PixelData[0];
	for (unsigned long i = 0; i < Rows * Columns * NumberOfFrames; ++i)
	{
		maxVal = std::max(maxVal, *ptr++);
	}

	if (BitsAllocated == 16)
	{
		return maxVal / UINT16_MAX;
	}
	else
	{ // BitsAllocated == 32
		return maxVal / UINT32_MAX;
	}
}