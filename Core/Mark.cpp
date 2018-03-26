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
#include "Mark.h"

bool operator!=(augmentedmark a,augmentedmark b) {
	return (a.p.px!=b.p.px)||(a.p.py!=b.p.py)||
		(a.slicenr!=b.slicenr)||(a.mark!=b.mark)||
		(a.name!=b.name);
};