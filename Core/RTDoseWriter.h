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

#include "RTDoseIODModule.h"

#include "gdcmVR.h"

#include <map>

namespace gdcm {
class Tag;
class DataSet;
class SequenceOfItems;
} // namespace gdcm

namespace iseg {

class ISEG_CORE_API RTDoseWriter
{
public:
	RTDoseWriter();
	~RTDoseWriter() = default;

	bool Write(const char* filename, RTDoseIODModule* rtDose);

protected:
	void AdjustVRTypeString(gdcm::VR::VRType& vr, unsigned int vm, std::string& value);
	void FormatDate(unsigned short& year, unsigned short& month, unsigned short& day, std::string& result);
	void FormatTime(unsigned short& hours, unsigned short& minutes, unsigned short& seconds, std::string& result);
	template<typename T>
	void StringEncodeNumber(T number, std::string& value);
	template<typename T>
	void ByteEncodeNumber(T number, std::string& value);
	template<typename T>
	void StringEncodeVector(const T* offsets, unsigned int count, std::string& value);
	template<typename T>
	void ByteEncodeVector(const T* offsets, unsigned int count, std::string& value);
	void ReplaceStringSection(std::string& text, const std::string oldSection, const std::string newSection);
	void InsertDataElement(gdcm::DataSet& dataset, const std::string keyword, std::string value, unsigned int vm = 1);
	void ReplaceDataElement(gdcm::DataSet& dataset, const std::string keyword, std::string value, unsigned int vm = 1);
	void InsertSequence(gdcm::DataSet& dataset, const std::string keyword, gdcm::SequenceOfItems* sequence);

protected:
	using TagVRTypePair = std::pair<gdcm::Tag, gdcm::VR::VRType>;
	static std::map<gdcm::VR::VRType, unsigned short>
			vr_type_max_length; // Maps a VR type to its maximal byte length
	static std::map<std::string, TagVRTypePair>
			attribute_tag_vr_type; // Maps an attribute keyword to its tag and VR type
};

} // namespace iseg
