/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "HDF5Reader.h"
#include "HDF5Writer.h"

using namespace std;
using namespace HDF5;

int main(int argc, char **argv)
{
	int is = HDF5Reader::existsValidHdf5(argv[1]);

	if (!is)
	{
		cerr << argv[1] << " does not exist or is not a valid hdf5 file" << endl;
		return 0;
	}
	else
		cerr << argv[1] << " exists and is a valid hdf5 file" << endl;

	return 1;
}
