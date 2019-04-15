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
	virtual void setNumberOfSteps(int N) {}

	virtual void increment() {}

	virtual void setValue(int /* percent */) {}

	virtual bool wasCanceled() const { return false; }
};

} // namespace iseg
