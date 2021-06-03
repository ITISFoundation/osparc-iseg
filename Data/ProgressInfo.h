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

#include "iSegData.h"

namespace iseg {

class ISEG_DATA_API ProgressInfo
{
public:
	virtual void SetNumberOfSteps(int N) {}

	virtual void Increment() {}

	virtual void SetValue(int /* percent */) {}

	virtual bool WasCanceled() const { return false; }
};

} // namespace iseg
