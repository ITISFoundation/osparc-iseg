/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef avw_19Feb08
#define avw_19Feb08

#include <string>

namespace avw {
enum datatype { ushort = 0, uchar = 1, sshort = 2, schar = 3 };
bool ReadHeader(const char *filename, unsigned short &w, unsigned short &h,
								unsigned short &nrofslices, float &dx1, float &dy1,
								float &thickness1, datatype &type);
void *ReadData(const char *filename, unsigned short slicenr, unsigned short &w,
							 unsigned short &h, datatype &type);

template<typename T>
static T ReadValueFromLine(const std::string &line, const char separator)
{
	T result;
	unsigned int pos = line.find_last_of(separator) + 1;
	std::stringstream T_stream(line.substr(pos, line.size()));
	T_stream >> result;
	return result;
};
static std::string ReadNameFromLine(const std::string &line,
																		const char separator)
{
	std::string result;
	unsigned int pos = (unsigned int)line.find_last_of(separator);
	if (pos == std::string::npos)
	{
		return line;
	}
	return line.substr(0, pos);
};
} // namespace avw

#endif