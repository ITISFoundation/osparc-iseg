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

#include "ProjectVersion.h"

int iseg::CombineTissuesVersion(const int version, const int tissuesVersion)
{
	return (tissuesVersion << 8) + version;
}

void iseg::ExtractTissuesVersion(const int combinedVersion, int& version,
								 int& tissuesVersion)
{
	tissuesVersion = combinedVersion >> 8;
	version = combinedVersion & 0xFF;
}
