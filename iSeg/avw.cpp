/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "avw.h"
#include "Precompiled.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>

#ifndef _WINDOWS
// #include <linux/types.h>
#	include <stdint.h>
#else
#	define uint64_t __int64
#endif

std::ifstream &ReadLineIntoIStringStream(std::ifstream &file,
																				 std::istringstream &line)
{
	// Read a line from file into stream. Consider
	// the following possibilities for newline:
	//	\r\n for Windoof
	//	\n for Unix
	//	\r for Mac
	// The file must be opened in binary mode.
	//
	std::string buffer = "";
	bool backslash_r = false;

	while (!file.eof())
	{
		char c = file.get();
		if ((c != '\r') && (c != '\n'))
		{
			buffer.push_back(c);
		}
		else
		{
			if (c == '\r')
			{
				backslash_r = true;
			}
			break;
		}
	}

	//	Skip line feed in case of DOS file.
	//
	if (backslash_r == true)
	{
		char p = file.peek();
		if (p == '\n')
		{
			file.get();
		}
	}
	line.str(buffer);
	line.seekg(0, std::ios::beg);
	line.clear();
	line.sync();
	return file;
}

bool avw::ReadHeader(const char *filename, unsigned short &w, unsigned short &h,
										 unsigned short &nrofslices, float &dx1, float &dy1,
										 float &thickness1, datatype &type)
{
	bool ok = false;

	w = h = nrofslices = 0;
	dx1 = dy1 = thickness1 = 1.0f;
	type = uchar;
	std::ifstream file;
	file.open(filename, std::ios::binary);
	if (file.good())
	{
		std::istringstream line;
		std::string dummy_str;

		file.seekg(0, std::ios::beg);
		ReadLineIntoIStringStream(file, line);
		line >> dummy_str;
		if (dummy_str == "AVW_ImageFile")
		{
			ReadLineIntoIStringStream(file, line);
			dummy_str = ReadValueFromLine<std::string>(line.str(), '=');

			if (dummy_str == "AVW_UNSIGNED_CHAR")
			{
				type = uchar;
			}
			else if (dummy_str == "AVW_SIGNED_CHAR")
			{
				type = schar;
			}
			else if (dummy_str == "AVW_UNSIGNED_SHORT")
			{
				type = ushort;
			}
			else if (dummy_str == "AVW_SIGNED_SHORT")
			{
				type = sshort;
			}

			//	Find width, length, height...
			//
			ReadLineIntoIStringStream(file, line);
			w = ReadValueFromLine<int>(line.str(), '=');
			ReadLineIntoIStringStream(file, line);
			h = ReadValueFromLine<int>(line.str(), '=');
			ReadLineIntoIStringStream(file, line);
			nrofslices = ReadValueFromLine<int>(line.str(), '=');

			ReadLineIntoIStringStream(file, line);
			std::string name;
			while ((name = ReadNameFromLine(line.str(), '=')) != "EndInformation")
			{
				if (name == "VoxelDepth")
					thickness1 = ReadValueFromLine<float>(line.str(), '=');
				if (name == "VoxelWidth")
					dx1 = ReadValueFromLine<float>(line.str(), '=');
				if (name == "VoxelHeight")
					dy1 = ReadValueFromLine<float>(line.str(), '=');
				ReadLineIntoIStringStream(file, line);
			}

			ok = true;
		}
		file.close();
	}

	return ok;
}

void *avw::ReadData(const char *filename, unsigned short slicenr,
										unsigned short &w, unsigned short &h, datatype &type)
{
	void *returnval = NULL;

	w = h = 0;
	type = uchar;
	std::ifstream file;
	file.open(filename, std::ios::binary);
	if (file.good())
	{
		std::istringstream line;
		std::string dummy_str;

		file.seekg(0, std::ios::beg);
		ReadLineIntoIStringStream(file, line);
		line >> dummy_str;
		if (dummy_str == "AVW_ImageFile")
		{
			uint64_t data_section_start =
					ReadValueFromLine<uint64_t>(line.str(), ' ');

			ReadLineIntoIStringStream(file, line);
			dummy_str = ReadValueFromLine<std::string>(line.str(), '=');

			if (dummy_str == "AVW_UNSIGNED_CHAR")
			{
				type = uchar;
			}
			else if (dummy_str == "AVW_SIGNED_CHAR")
			{
				type = schar;
			}
			else if (dummy_str == "AVW_UNSIGNED_SHORT")
			{
				type = ushort;
			}
			else if (dummy_str == "AVW_SIGNED_SHORT")
			{
				type = sshort;
			}

			//	Find width, length, height...
			//
			ReadLineIntoIStringStream(file, line);
			w = ReadValueFromLine<int>(line.str(), '=');
			ReadLineIntoIStringStream(file, line);
			h = ReadValueFromLine<int>(line.str(), '=');

			unsigned slicedim = 0;
			if (type == uchar || type == schar)
			{
				slicedim = (unsigned)w * h * sizeof(char);
			}
			else if (type == ushort || type == sshort)
			{
				slicedim = (unsigned)w * h * sizeof(short);
			}

			returnval = malloc(slicedim + 1);
			data_section_start += slicedim * slicenr;

			file.seekg(data_section_start);

			if (returnval != NULL)
			{
				char *data = (char *)returnval;
				file.read(data, slicedim);
				if (file.fail())
				{
					free(returnval);
					returnval = NULL;
				}
			}
		}
		file.close();
	}

	return returnval;
}
