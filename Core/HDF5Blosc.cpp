
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
