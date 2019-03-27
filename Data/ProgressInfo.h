/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#pragma once

#include "iSegData.h"

namespace iseg {

class ISEG_DATA_API ProgressInfo
{
public:
	virtual void setNumberOfSteps(int N) = 0;

	virtual void increment() = 0;

	virtual void setValue(int percent) = 0;

	virtual bool wasCanceled() const = 0;
};

} // namespace iseg
