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

#include "RTDoseIODModule.h"

#include "gdcmAttribute.h"
#include "gdcmUIDGenerator.h"

#include <string>

namespace iseg {

RTDoseIODModule::RTDoseIODModule()
{
	m_PatientName = "";
	m_PatientId = "";
	m_PatientBirthDate[0] = 0;
	m_PatientBirthDate[1] = 0;
	m_PatientBirthDate[2] = 0;
	m_PatientSex = Unspecified;
	m_StudyInstanceUid = GenerateUID();
	m_StudyDate[0] = 0;
	m_StudyDate[1] = 0;
	m_StudyDate[2] = 0;
	m_StudyTime[0] = 0;
	m_StudyTime[1] = 0;
	m_StudyTime[2] = 0;
	m_ReferringPhysicianName = "";
	m_StudyId = "";
	m_AccessionNumber = "";
	m_SeriesInstanceUid = GenerateUID();
	m_SeriesNumber = 0;
	m_FrameOfReferenceUid = GenerateUID();
	m_PositionReferenceIndicator = "";
	m_Manufacturer = "";
	m_InstanceNumber = 0;
	m_PixelSpacing[0] = 0;
	m_PixelSpacing[1] = 0;
	m_ImageOrientationPatient[0] = 0.0;
	m_ImageOrientationPatient[1] = 0.0;
	m_ImageOrientationPatient[2] = 0.0;
	m_ImageOrientationPatient[3] = 0.0;
	m_ImageOrientationPatient[4] = 0.0;
	m_ImageOrientationPatient[5] = 0.0;
	m_ImagePositionPatient[0] = 0.0;
	m_ImagePositionPatient[1] = 0.0;
	m_ImagePositionPatient[2] = 0.0;
	m_SliceThickness = 0.0;
	m_Rows = 0;
	m_Columns = 0;
	m_BitsAllocated = 0;
	m_PixelData = nullptr;
	m_NumberOfFrames = 0;
	m_DoseUnits = Gray;
	m_DoseType = Physical;
	m_DoseSummationType = Plan;
	m_GridFrameOffsetVector = nullptr;
	m_SopInstanceUid = GenerateUID();
}

RTDoseIODModule::~RTDoseIODModule() = default;

std::string RTDoseIODModule::GenerateUID()
{
	gdcm::UIDGenerator uid_gen;
	return std::string(uid_gen.Generate());
}

double RTDoseIODModule::GetDoseGridScaling()
{
	float max_val = 0.0f;
	float* ptr = &m_PixelData[0];
	for (unsigned long i = 0; i < m_Rows * m_Columns * m_NumberOfFrames; ++i)
	{
		max_val = std::max(max_val, *ptr++);
	}

	if (m_BitsAllocated == 16)
	{
		return max_val / UINT16_MAX;
	}
	else
	{ // BitsAllocated == 32
		return max_val / UINT32_MAX;
	}
}

} // namespace iseg