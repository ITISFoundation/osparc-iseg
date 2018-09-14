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

#include "HDF5Blosc.h"

#include "../Data/Logger.h"

namespace iseg {

#ifdef USE_HDF5_BLOSC
bool _blosc_enabled = true;
#else
bool _blosc_enabled = false;
#endif

bool BloscEnabled()
{
	return _blosc_enabled;
}

void SetBloscEnabled(bool on)
{
#ifdef USE_HDF5_BLOSC
	_blosc_enabled = on;
#else
	ISEG_WARNING("Trying to activate Blosc filter, but iSEG was not built with Blosc support.");
#endif
}

} // namespace iseg
