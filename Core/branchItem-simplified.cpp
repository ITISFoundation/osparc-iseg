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

#include "branchItem-simplified.h"

using namespace iseg;

std::vector<unsigned> BranchItem::availablelabels(0);
void BranchItem::init_availablelabels()
{
	availablelabels.clear();
	for (unsigned i = 60000; i > 0; i--)
		availablelabels.push_back(i);
}
