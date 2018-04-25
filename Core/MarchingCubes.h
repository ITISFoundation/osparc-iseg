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

#include "iSegCore.h"

#include "RGB.h"

#include "Data/Types.h"

#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace iseg {

//paul bourke
static int edgeTable[256] = {
		0x0, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f,
		0xb06, 0xc0a, 0xd03, 0xe09, 0xf00, 0x190, 0x99, 0x393, 0x29a, 0x596, 0x49f,
		0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90, 0x230,
		0x339, 0x33, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936,
		0xe3a, 0xf33, 0xc39, 0xd30, 0x3a0, 0x2a9, 0x1a3, 0xaa, 0x7a6, 0x6af, 0x5a5,
		0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0, 0x460, 0x569,
		0x663, 0x76a, 0x66, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a,
		0x963, 0xa69, 0xb60, 0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff, 0x3f5, 0x2fc,
		0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0, 0x650, 0x759, 0x453,
		0x55a, 0x256, 0x35f, 0x55, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53,
		0x859, 0x950, 0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc, 0xfcc,
		0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0, 0x8c0, 0x9c9, 0xac3, 0xbca,
		0xcc6, 0xdcf, 0xec5, 0xfcc, 0xcc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9,
		0x7c0, 0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x55,
		0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650, 0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6,
		0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0xff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f,
		0x66, 0x76a, 0x663, 0x569, 0x460, 0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af,
		0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa, 0x1a3, 0x2a9, 0x3a0, 0xd30,
		0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636,
		0x13a, 0x33, 0x339, 0x230, 0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895,
		0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99, 0x190, 0xf00, 0xe09,
		0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a,
		0x203, 0x109, 0x0};
static int triTable[256][16] = {
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 0, 9, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 7, 4, 3, 4, 0, 1, 10, 2, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 2, 9, 2, 0, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1},
		{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
		{8, 7, 4, 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{9, 1, 0, 8, 7, 4, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1},
		{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
		{3, 1, 10, 3, 10, 11, 7, 4, 8, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
		{4, 8, 7, 9, 11, 0, 9, 10, 11, 11, 3, 0, -1, -1, -1, -1},
		{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 5, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 2, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 8, 0, 1, 10, 2, 4, 5, 9, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
		{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 2, 11, 0, 11, 8, 4, 5, 9, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 5, 0, 5, 1, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1},
		{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
		{10, 11, 3, 10, 3, 1, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1},
		{4, 5, 9, 0, 1, 8, 8, 1, 10, 8, 10, 11, -1, -1, -1, -1},
		{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
		{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 7, 9, 7, 5, 10, 2, 1, -1, -1, -1, -1, -1, -1, -1},
		{10, 2, 1, 9, 0, 5, 5, 0, 3, 5, 3, 7, -1, -1, -1, -1},
		{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
		{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{7, 5, 9, 7, 9, 8, 3, 2, 11, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
		{2, 11, 3, 0, 8, 1, 1, 8, 7, 1, 7, 5, -1, -1, -1, -1},
		{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 5, 8, 7, 5, 10, 3, 1, 10, 11, 3, -1, -1, -1, -1},
		{5, 0, 7, 5, 9, 0, 7, 0, 11, 1, 10, 0, 11, 0, 10, -1},
		{11, 0, 10, 11, 3, 0, 10, 0, 5, 8, 7, 0, 5, 0, 7, -1},
		{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 1, 0, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 8, 1, 8, 9, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 6, 1, 6, 2, 3, 8, 0, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
		{2, 11, 3, 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 8, 0, 11, 0, 2, 10, 5, 6, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, 2, 11, 3, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1},
		{5, 6, 10, 1, 2, 9, 9, 2, 11, 9, 11, 8, -1, -1, -1, -1},
		{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
		{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
		{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{5, 6, 10, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 0, 3, 4, 3, 7, 6, 10, 5, -1, -1, -1, -1, -1, -1, -1},
		{1, 0, 9, 5, 6, 10, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1},
		{10, 5, 6, 1, 7, 9, 1, 3, 7, 7, 4, 9, -1, -1, -1, -1},
		{6, 2, 1, 6, 1, 5, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 2, 5, 6, 2, 3, 4, 0, 3, 7, 4, -1, -1, -1, -1},
		{8, 7, 4, 9, 5, 0, 0, 5, 6, 0, 6, 2, -1, -1, -1, -1},
		{7, 9, 3, 7, 4, 9, 3, 9, 2, 5, 6, 9, 2, 9, 6, -1},
		{3, 2, 11, 7, 4, 8, 10, 5, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 6, 10, 4, 2, 7, 4, 0, 2, 2, 11, 7, -1, -1, -1, -1},
		{0, 9, 1, 4, 8, 7, 2, 11, 3, 5, 6, 10, -1, -1, -1, -1},
		{9, 1, 2, 9, 2, 11, 9, 11, 4, 7, 4, 11, 5, 6, 10, -1},
		{8, 7, 4, 3, 5, 11, 3, 1, 5, 5, 6, 11, -1, -1, -1, -1},
		{5, 11, 1, 5, 6, 11, 1, 11, 0, 7, 4, 11, 0, 11, 4, -1},
		{0, 9, 5, 0, 5, 6, 0, 6, 3, 11, 3, 6, 8, 7, 4, -1},
		{6, 9, 5, 6, 11, 9, 4, 9, 7, 7, 9, 11, -1, -1, -1, -1},
		{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 6, 10, 4, 10, 9, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
		{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{3, 8, 0, 1, 9, 2, 2, 9, 4, 2, 4, 6, -1, -1, -1, -1},
		{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{10, 9, 4, 10, 4, 6, 11, 3, 2, -1, -1, -1, -1, -1, -1, -1},
		{0, 2, 8, 2, 11, 8, 4, 10, 9, 4, 6, 10, -1, -1, -1, -1},
		{3, 2, 11, 0, 6, 1, 0, 4, 6, 6, 10, 1, -1, -1, -1, -1},
		{6, 1, 4, 6, 10, 1, 4, 1, 8, 2, 11, 1, 8, 1, 11, -1},
		{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
		{8, 1, 11, 8, 0, 1, 11, 1, 6, 9, 4, 1, 6, 1, 4, -1},
		{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
		{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
		{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
		{2, 9, 6, 2, 1, 9, 6, 9, 7, 0, 3, 9, 7, 9, 3, -1},
		{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 11, 3, 10, 8, 6, 10, 9, 8, 8, 7, 6, -1, -1, -1, -1},
		{2, 7, 0, 2, 11, 7, 0, 7, 9, 6, 10, 7, 9, 7, 10, -1},
		{1, 0, 8, 1, 8, 7, 1, 7, 10, 6, 10, 7, 2, 11, 3, -1},
		{11, 1, 2, 11, 7, 1, 10, 1, 6, 6, 1, 7, -1, -1, -1, -1},
		{8, 6, 9, 8, 7, 6, 9, 6, 1, 11, 3, 6, 1, 6, 3, -1},
		{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 0, 8, 7, 6, 0, 3, 0, 11, 11, 0, 6, -1, -1, -1, -1},
		{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 8, 0, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 9, 1, 8, 1, 3, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 2, 1, 6, 7, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 2, 3, 8, 0, 6, 7, 11, -1, -1, -1, -1, -1, -1, -1},
		{2, 0, 9, 2, 9, 10, 6, 7, 11, -1, -1, -1, -1, -1, -1, -1},
		{6, 7, 11, 2, 3, 10, 10, 3, 8, 10, 8, 9, -1, -1, -1, -1},
		{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{2, 6, 7, 2, 7, 3, 0, 9, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
		{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
		{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{8, 11, 6, 8, 6, 4, 9, 1, 0, -1, -1, -1, -1, -1, -1, -1},
		{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
		{6, 4, 8, 6, 8, 11, 2, 1, 10, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 2, 3, 11, 0, 0, 11, 6, 0, 6, 4, -1, -1, -1, -1},
		{4, 8, 11, 4, 11, 6, 0, 9, 2, 2, 9, 10, -1, -1, -1, -1},
		{10, 3, 9, 10, 2, 3, 9, 3, 4, 11, 6, 3, 4, 3, 6, -1},
		{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 0, 9, 2, 4, 3, 2, 6, 4, 4, 8, 3, -1, -1, -1, -1},
		{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 6, 4, 8, 3, 6, 3, 10, 0, 9, 3, 10, 3, 9, -1},
		{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 5, 9, 7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, 4, 5, 9, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1},
		{5, 1, 0, 5, 0, 4, 7, 11, 6, -1, -1, -1, -1, -1, -1, -1},
		{11, 6, 7, 8, 4, 3, 3, 4, 5, 3, 5, 1, -1, -1, -1, -1},
		{9, 4, 5, 10, 2, 1, 7, 11, 6, -1, -1, -1, -1, -1, -1, -1},
		{6, 7, 11, 1, 10, 2, 0, 3, 8, 4, 5, 9, -1, -1, -1, -1},
		{7, 11, 6, 5, 10, 4, 4, 10, 2, 4, 2, 0, -1, -1, -1, -1},
		{3, 8, 4, 3, 4, 5, 3, 5, 2, 10, 2, 5, 11, 6, 7, -1},
		{7, 3, 2, 7, 2, 6, 5, 9, 4, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 5, 0, 6, 8, 0, 2, 6, 6, 7, 8, -1, -1, -1, -1},
		{3, 2, 6, 3, 6, 7, 1, 0, 5, 5, 0, 4, -1, -1, -1, -1},
		{6, 8, 2, 6, 7, 8, 2, 8, 1, 4, 5, 8, 1, 8, 5, -1},
		{9, 4, 5, 10, 6, 1, 1, 6, 7, 1, 7, 3, -1, -1, -1, -1},
		{1, 10, 6, 1, 6, 7, 1, 7, 0, 8, 0, 7, 9, 4, 5, -1},
		{4, 10, 0, 4, 5, 10, 0, 10, 3, 6, 7, 10, 3, 10, 7, -1},
		{7, 10, 6, 7, 8, 10, 5, 10, 4, 4, 10, 8, -1, -1, -1, -1},
		{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
		{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{1, 10, 2, 9, 11, 5, 9, 8, 11, 11, 6, 5, -1, -1, -1, -1},
		{0, 3, 11, 0, 11, 6, 0, 6, 9, 5, 9, 6, 1, 10, 2, -1},
		{11, 5, 8, 11, 6, 5, 8, 5, 0, 10, 2, 5, 0, 5, 2, -1},
		{6, 3, 11, 6, 5, 3, 2, 3, 10, 10, 3, 5, -1, -1, -1, -1},
		{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
		{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 5, 1, 0, 8, 5, 8, 6, 3, 2, 8, 6, 8, 2, -1},
		{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 3, 1, 10, 6, 3, 6, 8, 5, 9, 6, 8, 6, 9, -1},
		{10, 0, 1, 10, 6, 0, 9, 0, 5, 5, 0, 6, -1, -1, -1, -1},
		{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 10, 5, 11, 5, 7, 8, 0, 3, -1, -1, -1, -1, -1, -1, -1},
		{5, 7, 11, 5, 11, 10, 1, 0, 9, -1, -1, -1, -1, -1, -1, -1},
		{10, 5, 7, 10, 7, 11, 9, 1, 8, 8, 1, 3, -1, -1, -1, -1},
		{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, 1, 7, 2, 1, 5, 7, 7, 11, 2, -1, -1, -1, -1},
		{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{7, 2, 5, 7, 11, 2, 5, 2, 9, 3, 8, 2, 9, 2, 8, -1},
		{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
		{9, 1, 0, 5, 3, 10, 5, 7, 3, 3, 2, 10, -1, -1, -1, -1},
		{9, 2, 8, 9, 1, 2, 8, 2, 7, 10, 5, 2, 7, 2, 5, -1},
		{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{0, 9, 1, 8, 10, 4, 8, 11, 10, 10, 5, 4, -1, -1, -1, -1},
		{10, 4, 11, 10, 5, 4, 11, 4, 3, 9, 1, 4, 3, 4, 1, -1},
		{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
		{0, 11, 4, 0, 3, 11, 4, 11, 5, 2, 1, 11, 5, 11, 1, -1},
		{0, 5, 2, 0, 9, 5, 2, 5, 11, 4, 8, 5, 11, 5, 8, -1},
		{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
		{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{3, 2, 10, 3, 10, 5, 3, 5, 8, 4, 8, 5, 0, 9, 1, -1},
		{5, 2, 10, 5, 4, 2, 1, 2, 9, 9, 2, 4, -1, -1, -1, -1},
		{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 5, 4, 8, 3, 5, 9, 5, 0, 0, 5, 3, -1, -1, -1, -1},
		{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, 4, 7, 9, 9, 7, 11, 9, 11, 10, -1, -1, -1, -1},
		{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
		{3, 4, 1, 3, 8, 4, 1, 4, 10, 7, 11, 4, 10, 4, 11, -1},
		{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
		{9, 4, 7, 9, 7, 11, 9, 11, 1, 2, 1, 11, 0, 3, 8, -1},
		{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{11, 4, 7, 11, 2, 4, 8, 4, 3, 3, 4, 2, -1, -1, -1, -1},
		{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{9, 7, 10, 9, 4, 7, 10, 7, 2, 8, 0, 7, 2, 7, 0, -1},
		{3, 10, 7, 3, 2, 10, 7, 10, 4, 1, 0, 10, 4, 10, 0, -1},
		{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{4, 1, 9, 4, 7, 1, 0, 1, 8, 8, 1, 7, -1, -1, -1, -1},
		{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 9, 0, 3, 11, 9, 1, 9, 2, 2, 9, 11, -1, -1, -1, -1},
		{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 8, 3, 2, 10, 8, 0, 8, 1, 1, 8, 10, -1, -1, -1, -1},
		{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static int ptTable[16] = {0, 145, 50, 163, 100, 245, 86, 199,
		200, 89, 250, 107, 172, 61, 158, 15};
static int triTable_1[16][10] = {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 1, 1, 7, 5, -1, -1, -1, -1},
		{5, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{5, 6, 2, 0, 7, 4, -1, -1, -1, -1},
		{4, 2, 1, 4, 6, 2, -1, -1, -1, -1},
		{0, 7, 1, 7, 6, 1, 6, 2, 1, -1},
		{6, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{3, 4, 0, 6, 4, 3, -1, -1, -1, -1},
		{6, 7, 3, 4, 5, 1, -1, -1, -1, -1},
		{6, 0, 3, 6, 5, 0, 5, 1, 0, -1},
		{2, 7, 3, 2, 5, 7, -1, -1, -1, -1},
		{3, 2, 5, 3, 5, 4, 3, 4, 0, -1},
		{2, 7, 3, 2, 4, 7, 2, 1, 4, -1},
		{2, 0, 3, 2, 1, 0, -1, -1, -1, -1}};

typedef struct
{
	float x;
	float y;
	float z;
} Koord;
typedef struct
{
	float v[3];
} V3F;
typedef struct
{
	unsigned e1;
	unsigned e2;
	unsigned e3;
} Triang;
typedef struct
{
	float rgb[3];
	std::string name;
	std::vector<V3F>* vertex_array;
	std::vector<unsigned int>* index_array;
} tissuedescript;
class vectissuedescr
{
public:
	std::vector<tissuedescript> vtd;
	~vectissuedescr()
	{
		for (std::vector<tissuedescript>::iterator it = vtd.begin();
				 it != vtd.end(); it++)
		{
			delete it->index_array;
			//			delete it->vertex_array;
		}
		if (!vtd.empty())
			delete vtd.begin()->vertex_array;
	}
};

class MarchingCubes
{
public:
	void init(tissues_size_t** tissuearray1, unsigned short l1,
			unsigned short b1, unsigned short h1, float slicedist1, float dx1,
			float dy1)
	{
		tissuearray = tissuearray1;
		l = l1;
		b = b1;
		h = h1;
		slicedist = slicedist1;
		dx = dx1;
		dy = dy1;

		return;
	}

	void marchingcubeprint(const char* filename,
			std::vector<tissues_size_t>& tissuevec,
			std::vector<RGB>& colorvec,
			std::vector<std::string>& vstring)
	{
		if (h == 0)
			return;
		if (h == 1)
		{
			MarchingCubes mc;
			tissues_size_t* tissuearraytmp[2];
			tissuearraytmp[0] = tissuearray[0];
			tissuearraytmp[1] = tissuearray[0];
			mc.init(tissuearraytmp, l, b, h + 1, slicedist, dx, dy);
			mc.marchingcubeprint(filename, tissuevec, colorvec, vstring);
			return;
		}
		if (tissuevec.size() == colorvec.size() &&
				vstring.size() == colorvec.size())
		{
			std::vector<unsigned> trivec;
			float colordummy[3];
			tissues_size_t tissue;
			FILE* fp;
			if ((fp = fopen(filename, "wb")) == NULL)
				return;

			tissues_size_t* tissuearray1;
			tissues_size_t* tissuearray2;

			std::vector<Koord> vertices;
			Koord K;
			const unsigned area = unsigned(l) * b;
			unsigned long index = 0;
			unsigned long* xind =
					(unsigned long*)malloc(sizeof(unsigned long) * area);
			unsigned long* yind =
					(unsigned long*)malloc(sizeof(unsigned long) * area);
			unsigned long* zind =
					(unsigned long*)malloc(sizeof(unsigned long) * area);
			unsigned long* xindold =
					(unsigned long*)malloc(sizeof(unsigned long) * area);
			unsigned long* yindold =
					(unsigned long*)malloc(sizeof(unsigned long) * area);
			unsigned long* xedge1 =
					(unsigned long*)malloc(sizeof(unsigned long) * l);
			unsigned long* yedge1 =
					(unsigned long*)malloc(sizeof(unsigned long) * b);
			unsigned long* xedge1old =
					(unsigned long*)malloc(sizeof(unsigned long) * l);
			unsigned long* yedge1old =
					(unsigned long*)malloc(sizeof(unsigned long) * b);
			unsigned long* xedge2 =
					(unsigned long*)malloc(sizeof(unsigned long) * l);
			unsigned long* yedge2 =
					(unsigned long*)malloc(sizeof(unsigned long) * b);
			unsigned long* xedge2old =
					(unsigned long*)malloc(sizeof(unsigned long) * l);
			unsigned long* yedge2old =
					(unsigned long*)malloc(sizeof(unsigned long) * b);
			unsigned long* zcorner =
					(unsigned long*)malloc(sizeof(unsigned long) * area);
			unsigned long* dummy;
			unsigned long edgeindex[12];
			int cubeindex;
			unsigned pos, pos1, pos2;

			//	fprintf(fp,"tissues %u\n",tissuevec.size());
			unsigned dummy1 = (unsigned)tissuevec.size();
			fwrite(&dummy1, sizeof(unsigned), 1, fp);
			std::vector<RGB>::iterator itc = colorvec.begin();
			std::vector<std::string>::iterator itstr = vstring.begin();

			std::string name("A");

			for (std::vector<tissues_size_t>::iterator it = tissuevec.begin();
					 it != tissuevec.end(); it++)
			{
				trivec.clear();
				colordummy[0] = itc->r;
				colordummy[1] = itc->g;
				colordummy[2] = itc->b;
				//		cout << colordummy[0]<< " "<<colordummy[1]<<" "<<colordummy[2]<<endl;
				fwrite(colordummy, sizeof(float) * 3, 1, fp);
				//		fprintf(fp,"color %f %f %f\n",itc->r,itc->g,itc->b);
				itc++;

				name.append("A");
				//		int l1=name.size();
				int l2 = (int)itstr->size();
				fwrite(&l2, sizeof(int), 1, fp);
				//		fwrite(name.c_str(),sizeof(char)*(l1+1),1,fp);
				fwrite(itstr->c_str(), sizeof(char) * (l2 + 1), 1, fp);
				itstr++;

				pos = pos2 = 0;
				tissue = *it;

				for (unsigned short iz = 0; iz + 1 < h; ++iz)
				{
					tissuearray1 = tissuearray[iz];
					tissuearray2 = tissuearray[iz + 1];
					pos1 = 0;
					dummy = xind;
					xind = xindold;
					xindold = dummy;
					dummy = yind;
					yind = yindold;
					yindold = dummy;
					dummy = xedge1;
					xedge1 = xedge1old;
					xedge1old = dummy;
					dummy = yedge1;
					yedge1 = yedge1old;
					yedge1old = dummy;
					dummy = xedge2;
					xedge2 = xedge2old;
					xedge2old = dummy;
					dummy = yedge2;
					yedge2 = yedge2old;
					yedge2old = dummy;

					for (unsigned short iy = 0; iy + 1 < b; ++iy)
					{
						for (unsigned short ix = 0; ix + 1 < l; ++ix)
						{
							cubeindex = 0;
							if (tissuearray1[pos1] == tissue)
								cubeindex |= 1;
							if (tissuearray1[pos1 + 1] == tissue)
								cubeindex |= 2;
							if (tissuearray1[pos1 + 1 + l] == tissue)
								cubeindex |= 4;
							if (tissuearray1[pos1 + l] == tissue)
								cubeindex |= 8;
							if (tissuearray2[pos1] == tissue)
								cubeindex |= 16;
							if (tissuearray2[pos1 + 1] == tissue)
								cubeindex |= 32;
							if (tissuearray2[pos1 + 1 + l] == tissue)
								cubeindex |= 64;
							if (tissuearray2[pos1 + l] == tissue)
								cubeindex |= 128;

							if (edgeTable[cubeindex] & 1)
							{
								if (iz == 0 && iy == 0)
								{
									edgeindex[0] = index;
									K.x = (0.5f + ix) * dx;
									K.y = 0.0f;
									K.z = 0.0f;
									xindold[pos1] = index;
									vertices.push_back(K);
									index++;
								}
								else
									edgeindex[0] = xindold[pos1];
							}
							if (edgeTable[cubeindex] & 2)
							{
								if (iz == 0)
								{
									edgeindex[1] = index;
									K.x = (1.0f + ix) * dx;
									K.y = (0.5f + iy) * dy;
									K.z = 0.0f;
									vertices.push_back(K);
									yindold[pos1 + 1] = index;
									index++;
								}
								else
									edgeindex[1] = yindold[pos1 + 1];
							}
							if (edgeTable[cubeindex] & 4)
							{
								if (iz == 0)
								{
									edgeindex[2] = index;
									K.x = (0.5f + ix) * dx;
									K.y = (1.0f + iy) * dy;
									K.z = 0.0f;
									vertices.push_back(K);
									xindold[pos1 + l] = index;
									index++;
								}
								else
									edgeindex[2] = xindold[pos1 + l];
							}
							if (edgeTable[cubeindex] & 8)
							{
								if (iz == 0 && ix == 0)
								{
									edgeindex[3] = index;
									K.y = (0.5f + iy) * dy;
									K.x = 0.0f;
									K.z = 0.0f;
									vertices.push_back(K);
									yindold[pos1] = index;
									index++;
								}
								else
									edgeindex[3] = yindold[pos1];
							}
							if (edgeTable[cubeindex] & 16)
							{
								if (iy == 0)
								{
									edgeindex[4] = index;
									K.x = (0.5f + ix) * dx;
									K.y = 0.0f;
									K.z = (1.0f + iz) * slicedist;
									vertices.push_back(K);
									xind[pos1] = index;
									index++;
								}
								else
									edgeindex[4] = xind[pos1];
							}
							if (edgeTable[cubeindex] & 32)
							{
								edgeindex[5] = index;
								K.x = (1.0f + ix) * dx;
								K.y = (0.5f + iy) * dy;
								K.z = (1.0f + iz) * slicedist;
								vertices.push_back(K);
								yind[pos1 + 1] = index;
								index++;
							}
							if (edgeTable[cubeindex] & 64)
							{
								edgeindex[6] = index;
								K.x = (0.5f + ix) * dx;
								K.y = (1.0f + iy) * dy;
								K.z = (1.0f + iz) * slicedist;
								vertices.push_back(K);
								xind[pos1 + l] = index;
								index++;
							}
							if (edgeTable[cubeindex] & 128)
							{
								if (ix == 0)
								{
									edgeindex[7] = index;
									K.y = (0.5f + iy) * dy;
									K.x = 0.0f;
									K.z = (1.0f + iz) * slicedist;
									vertices.push_back(K);
									yind[pos1] = index;
									index++;
								}
								else
									edgeindex[7] = yind[pos1];
							}
							if (edgeTable[cubeindex] & 256)
							{
								if (ix == 0 && iy == 0)
								{
									edgeindex[8] = index;
									K.y = 0.0f;
									K.x = 0.0f;
									K.z = (0.5f + iz) * slicedist;
									vertices.push_back(K);
									zind[pos1] = index;
									index++;
								}
								else
									edgeindex[8] = zind[pos1];
							}
							if (edgeTable[cubeindex] & 512)
							{
								if (iy == 0)
								{
									edgeindex[9] = index;
									K.y = 0.0f;
									K.x = (1.0f + ix) * dx;
									K.z = (0.5f + iz) * slicedist;
									vertices.push_back(K);
									zind[pos1 + 1] = index;
									index++;
								}
								else
									edgeindex[9] = zind[pos1 + 1];
							}
							if (edgeTable[cubeindex] & 1024)
							{
								edgeindex[10] = index;
								K.x = (1.0f + ix) * dx;
								K.y = (1.0f + iy) * dy;
								K.z = (0.5f + iz) * slicedist;
								vertices.push_back(K);
								zind[pos1 + l + 1] = index;
								index++;
							}
							if (edgeTable[cubeindex] & 2048)
							{
								if (ix == 0)
								{
									edgeindex[11] = index;
									K.x = 0.0f;
									K.y = (1.0f + iy) * dy;
									K.z = (0.5f + iz) * slicedist;
									vertices.push_back(K);
									zind[pos1 + l] = index;
									index++;
								}
								else
									edgeindex[11] = zind[pos1 + l];
							}

							//						float x1,y1;
							for (short i = 0; triTable[cubeindex][i] != -1; i++)
							{
								trivec.push_back(
										edgeindex[triTable[cubeindex][i]]);
								//if(i%3==0) {
								//	x1=vertices[edgeindex[triTable[cubeindex][i]]].x;
								//	y1=vertices[edgeindex[triTable[cubeindex][i]]].y;
								//}
								//else {
								//	if(abs(x1-vertices[edgeindex[triTable[cubeindex][i]]].x)>1.5f
								//		||abs(y1-vertices[edgeindex[triTable[cubeindex][i]]].y)>1.5f
								//	)
								//		fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
								//}
							}
							//						fprintf(fp,"%u ",edgeindex[triTable[cubeindex][i]]);

							pos++;
							pos1++;
						}
						pos++;
						pos1++;
					}
					pos += l;

					//			xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
					K.x = 0.0f;
					pos2 = 0;
					for (unsigned short iy = 0; iy + 1 < b; ++iy)
					{
						cubeindex = 0;
						if (tissuearray1[pos2] == tissue)
							cubeindex |= 1;
						if (tissuearray1[pos2 + l] == tissue)
							cubeindex |= 2;
						if (tissuearray2[pos2 + l] == tissue)
							cubeindex |= 4;
						if (tissuearray2[pos2] == tissue)
							cubeindex |= 8;
						pos2 += l;

						if (ptTable[cubeindex] & 1)
						{
							if (iz == 0 && iy == 0)
							{
								edgeindex[0] = index;
								//						K.x=0.0f;
								K.y = 0.0f;
								K.z = 0.0f;
								vertices.push_back(K);
								yedge1old[0] = xedge1old[0] = index;
								index++;
							}
							else
								edgeindex[0] = yedge1old[iy];
						}
						if (ptTable[cubeindex] & 2)
						{
							if (iz == 0)
							{
								edgeindex[1] = index;
								//						K.x=0.0f;
								K.y = (1.0f + iy) * dy;
								K.z = 0.0f;
								vertices.push_back(K);
								yedge1old[iy + 1] = index;
								index++;
							}
							else
								edgeindex[1] = yedge1old[iy + 1];
						}
						if (ptTable[cubeindex] & 4)
						{
							edgeindex[2] = index;
							//					K.x=0.0f;
							K.y = (1.0f + iy) * dy;
							K.z = (1.0f + iz) * slicedist;
							vertices.push_back(K);
							yedge1[iy + 1] = index;
							index++;
						}
						if (ptTable[cubeindex] & 8)
						{
							if (iy == 0)
							{
								edgeindex[3] = index;
								K.y = 0.0f;
								//						K.x=0.0f;
								K.z = (1.0f + iz) * slicedist;
								vertices.push_back(K);
								yedge1[0] = index;
								index++;
							}
							else
								edgeindex[3] = yedge1[iy];
						}
						if (ptTable[cubeindex] & 16)
						{
							edgeindex[4] = yindold[iy * l];
						}
						if (ptTable[cubeindex] & 32)
						{
							edgeindex[5] = zind[(1 + iy) * l];
						}
						if (ptTable[cubeindex] & 64)
						{
							edgeindex[6] = yind[iy * l];
						}
						if (ptTable[cubeindex] & 128)
						{
							edgeindex[7] = zind[iy * l];
						}

						//					float x1,y1;
						for (short i = 0; triTable_1[cubeindex][i] != -1; i++)
						{
							trivec.push_back(
									edgeindex[triTable_1[cubeindex][i]]);
							/*if(i%3==0) {
								x1=vertices[edgeindex[triTable_1[cubeindex][i]]].x;
								y1=vertices[edgeindex[triTable_1[cubeindex][i]]].y;
							}
							else {
								if(abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x)>1.5f
									||abs(y1-vertices[edgeindex[triTable_1[cubeindex][i]]].y)>1.5f
								)
									fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
							}*/
							//fprintf(fp,"%u ",edgeindex[triTable_1[cubeindex][i]]);
						}
					}
					//yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy

					pos2 = l - 1;
					K.x = float(l - 1) * dx;
					for (unsigned short iy = 0; iy + 1 < b; ++iy)
					{
						cubeindex = 0;
						if (tissuearray1[pos2 + l] == tissue)
							cubeindex |= 1;
						if (tissuearray1[pos2] == tissue)
							cubeindex |= 2;
						if (tissuearray2[pos2] == tissue)
							cubeindex |= 4;
						if (tissuearray2[pos2 + l] == tissue)
							cubeindex |= 8;
						pos2 += l;

						if (ptTable[cubeindex] & 2)
						{
							if (iz == 0 && iy == 0)
							{
								edgeindex[1] = index;
								//						K.x=f;
								K.y = 0.0f;
								K.z = 0.0f;
								vertices.push_back(K);
								yedge2old[0] = xedge1old[l - 1] = index;
								index++;
							}
							else
								edgeindex[1] = yedge2old[iy];
						}
						if (ptTable[cubeindex] & 1)
						{
							if (iz == 0)
							{
								edgeindex[0] = index;
								//						K.x=f;
								K.y = (1.0f + iy) * dy;
								K.z = 0.0f;
								vertices.push_back(K);
								yedge2old[iy + 1] = index;
								index++;
							}
							else
								edgeindex[0] = yedge2old[iy + 1];
						}
						if (ptTable[cubeindex] & 8)
						{
							edgeindex[3] = index;
							//					K.x=f;
							K.y = (1.0f + iy) * dy;
							K.z = (1.0f + iz) * slicedist;
							vertices.push_back(K);
							yedge2[iy + 1] = index;
							index++;
						}
						if (ptTable[cubeindex] & 4)
						{
							if (iy == 0)
							{
								edgeindex[2] = index;
								K.y = 0.0f;
								//						K.x=f;
								K.z = (1.0f + iz) * slicedist;
								vertices.push_back(K);
								yedge2[0] = index;
								index++;
							}
							else
								edgeindex[2] = yedge2[iy];
						}
						if (ptTable[cubeindex] & 16)
						{
							edgeindex[4] = yindold[(1 + iy) * l - 1];
						}
						if (ptTable[cubeindex] & 128)
						{
							edgeindex[7] = zind[(2 + iy) * l - 1];
						}
						if (ptTable[cubeindex] & 64)
						{
							edgeindex[6] = yind[(1 + iy) * l - 1];
						}
						if (ptTable[cubeindex] & 32)
						{
							edgeindex[5] = zind[(1 + iy) * l - 1];
						}

						//					float x1,y1;
						for (short i = 0; triTable_1[cubeindex][i] != -1; i++)
						{
							trivec.push_back(
									edgeindex[triTable_1[cubeindex][i]]);
							/*if(i%3==0) {
								x1=vertices[edgeindex[triTable_1[cubeindex][i]]].x;
								y1=vertices[edgeindex[triTable_1[cubeindex][i]]].y;
							}
							else {
								if(abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x)>1.5f
									||abs(y1-vertices[edgeindex[triTable_1[cubeindex][i]]].y)>1.5f
								)
									fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
							}
							if(iy==28&&iz==3)
								fprintf(fpx,"%f\n",(float)edgeindex[triTable_1[cubeindex][i]]);*/
							//fprintf(fp,"%u ",edgeindex[triTable_1[cubeindex][i]]);
						}
					}

					//zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
					pos2 = 0;
					K.y = 0;
					for (unsigned short ix = 0; ix + 1 < l; ++ix)
					{
						cubeindex = 0;
						if (tissuearray1[pos2 + 1] == tissue)
							cubeindex |= 1;
						if (tissuearray1[pos2] == tissue)
							cubeindex |= 2;
						if (tissuearray2[pos2] == tissue)
							cubeindex |= 4;
						if (tissuearray2[pos2 + 1] == tissue)
							cubeindex |= 8;
						pos2++;

						if (ptTable[cubeindex] & 2)
						{
							edgeindex[1] = xedge1old[ix];
						}
						if (ptTable[cubeindex] & 1)
						{
							if (iz == 0)
							{
								edgeindex[0] = index;
								K.x = (1.0f + ix) * dx;
								//						K.y=0.0f;
								K.z = 0.0f;
								vertices.push_back(K);
								xedge1old[ix + 1] = index;
								index++;
							}
							else
								edgeindex[0] = xedge1old[ix + 1];
						}
						if (ptTable[cubeindex] & 8)
						{
							if (ix + 2 == l)
							{
								edgeindex[3] = xedge1[ix + 1] = yedge2[0];
							}
							else
							{
								edgeindex[3] = index;
								K.x = (1.0f + ix) * dx;
								//					K.y=0.0f;
								K.z = (1.0f + iz) * slicedist;
								vertices.push_back(K);
								xedge1[ix + 1] = index;
								index++;
							}
						}
						if (ptTable[cubeindex] & 4)
						{
							if (ix == 0)
							{
								edgeindex[2] = xedge1[0] = yedge1[0];
							}
							else
								edgeindex[2] = xedge1[ix];
						}
						if (ptTable[cubeindex] & 16)
						{
							edgeindex[4] = xindold[ix];
						}
						if (ptTable[cubeindex] & 128)
						{
							edgeindex[7] = zind[ix + 1];
						}
						if (ptTable[cubeindex] & 64)
						{
							edgeindex[6] = xind[ix];
						}
						if (ptTable[cubeindex] & 32)
						{
							edgeindex[5] = zind[ix];
						}

						//					float x1,y1;
						for (short i = 0; triTable_1[cubeindex][i] != -1; i++)
						{
							trivec.push_back(
									edgeindex[triTable_1[cubeindex][i]]);
							//if(i%3==0) {
							//	x1=vertices[edgeindex[triTable_1[cubeindex][i]]].x;
							//	y1=vertices[edgeindex[triTable_1[cubeindex][i]]].y;
							//}
							//else {
							////	abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x);
							//	if(abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x)>1.5f
							//		||abs(y1-vertices[edgeindex[triTable_1[cubeindex][i]]].y)>1.5f)
							//		fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
							//}
						}
						//fprintf(fp,"%u ",edgeindex[triTable_1[cubeindex][i]]);
					}
					//-------------------------------------------------------------------------------

					pos2 = area - l;
					K.y = float(b - 1) * dy;
					for (unsigned short ix = 0; ix + 1 < l; ++ix)
					{
						cubeindex = 0;
						if (tissuearray1[pos2] == tissue)
							cubeindex |= 1;
						if (tissuearray1[pos2 + 1] == tissue)
							cubeindex |= 2;
						if (tissuearray2[pos2 + 1] == tissue)
							cubeindex |= 4;
						if (tissuearray2[pos2] == tissue)
							cubeindex |= 8;
						pos2++;

						if (ptTable[cubeindex] & 1)
						{
							if (ix == 0)
							{
								edgeindex[0] = xedge2old[0] = yedge1old[b - 1];
							}
							else
								edgeindex[0] = xedge2old[ix];
						}
						if (ptTable[cubeindex] & 2)
						{
							if (ix + 2 == l)
							{
								edgeindex[1] = xedge2old[l - 1] =
										yedge2old[b - 1];
							}
							else if (iz == 0)
							{
								edgeindex[1] = index;
								K.x = (1.0f + ix) * dx;
								//						K.y=float(l-1);
								K.z = 0.0f;
								vertices.push_back(K);
								xedge2old[ix + 1] = index;
								index++;
							}
							else
								edgeindex[1] = xedge2old[ix + 1];
						}
						if (ptTable[cubeindex] & 4)
						{
							if (ix + 2 == l)
								edgeindex[2] = xedge2[l - 1] = yedge2[b - 1];
							else
							{
								edgeindex[2] = index;
								K.x = (1.0f + ix) * dx;
								//					K.y=float(l-1);
								K.z = (1.0f + iz) * slicedist;
								vertices.push_back(K);
								xedge2[ix + 1] = index;
								index++;
							}
						}
						if (ptTable[cubeindex] & 8)
						{
							if (ix == 0)
							{
								edgeindex[3] = xedge2[0] = yedge1[b - 1];
							}
							else
								edgeindex[3] = xedge2[ix];
						}
						if (ptTable[cubeindex] & 16)
						{
							edgeindex[4] = xindold[area - l + ix];
						}
						if (ptTable[cubeindex] & 32)
						{
							edgeindex[5] = zind[area - l + ix + 1];
						}
						if (ptTable[cubeindex] & 64)
						{
							edgeindex[6] = xind[area - l + ix];
						}
						if (ptTable[cubeindex] & 128)
						{
							edgeindex[7] = zind[area - l + ix];
						}

						//					float x1,y1;
						for (short i = 0; triTable_1[cubeindex][i] != -1; i++)
						{
							trivec.push_back(
									edgeindex[triTable_1[cubeindex][i]]);
							/*if(i%3==0) {
								x1=vertices[edgeindex[triTable_1[cubeindex][i]]].x;
								y1=vertices[edgeindex[triTable_1[cubeindex][i]]].y;
							}
							else {
								if(abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x)>1.5f
									||abs(y1-vertices[edgeindex[triTable_1[cubeindex][i]]].y)>1.5f
								)
									fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
							}*/
							//fprintf(fp,"%u ",edgeindex[triTable_1[cubeindex][i]]);
						}
					}

					K.z = 0.0f;
					if (iz == 0)
					{
						pos1 = 0;
						unsigned pos3 = 0;
						for (unsigned short iy = 0; iy < b; iy++)
						{
							zcorner[pos3] = yedge1old[iy];
							pos3 += l - 1;
							zcorner[pos3] = yedge2old[iy];
							pos3++;
						}
						for (unsigned short ix = 0; ix < l; ix++)
						{
							zcorner[ix] = xedge1old[ix];
							zcorner[ix + area - l] = xedge2old[ix];
						}

						for (unsigned short iy = 0; iy + 1 < b; ++iy)
						{
							for (unsigned short ix = 0; ix + 1 < l; ++ix)
							{
								cubeindex = 0;
								if (tissuearray1[pos1] == tissue)
									cubeindex |= 1;
								if (tissuearray1[pos1 + 1] == tissue)
									cubeindex |= 2;
								if (tissuearray1[pos1 + l + 1] == tissue)
									cubeindex |= 4;
								if (tissuearray1[pos1 + l] == tissue)
									cubeindex |= 8;

								if (ptTable[cubeindex] & 1)
								{
									edgeindex[0] = zcorner[pos1];
								}
								if (ptTable[cubeindex] & 2)
								{
									edgeindex[1] = zcorner[pos1 + 1];
								}
								if (ptTable[cubeindex] & 4)
								{
									if (ix + 2 == l || iy + 2 == b)
										edgeindex[2] = zcorner[pos1 + 1 + l];
									else
									{
										edgeindex[2] = index;
										K.x = (1.0f + ix) * dx;
										K.y = (1.0f + iy) * dy;
										//						K.z=0.0f;
										vertices.push_back(K);
										zcorner[pos1 + l + 1] = index;
										index++;
									}
								}
								if (ptTable[cubeindex] & 8)
								{
									edgeindex[3] = zcorner[pos1 + l];
								}
								if (ptTable[cubeindex] & 16)
								{
									edgeindex[4] = xindold[pos1];
								}
								if (ptTable[cubeindex] & 32)
								{
									edgeindex[5] = yindold[pos1 + 1];
								}
								if (ptTable[cubeindex] & 64)
								{
									edgeindex[6] = xindold[pos1 + l];
								}
								if (ptTable[cubeindex] & 128)
								{
									edgeindex[7] = yindold[pos1];
								}

								//							float x1,y1;
								for (short i = 0;
										 triTable_1[cubeindex][i] != -1; i++)
								{
									trivec.push_back(
											edgeindex[triTable_1[cubeindex][i]]);
									/*if(i%3==0) {
										x1=vertices[edgeindex[triTable_1[cubeindex][i]]].x;
										y1=vertices[edgeindex[triTable_1[cubeindex][i]]].y;
									}
									else {
										if(abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x)>1.5f
											||abs(y1-vertices[edgeindex[triTable_1[cubeindex][i]]].y)>1.5f
										)
											fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
									}*/
									//fprintf(fp,"%u ",edgeindex[triTable_1[cubeindex][i]]);
								}

								pos1++;
							}
							pos1++;
						}
					}
				}

				pos1 = area * (h - 1);
				K.z = (h - 1) * slicedist;

				pos2 = 0;
				for (unsigned short iy = 0; iy < b; iy++)
				{
					zcorner[pos2] = yedge1[iy];
					pos2 += l - 1;
					zcorner[pos2] = yedge2[iy];
					pos2++;
				}
				for (unsigned short ix = 0; ix < l; ix++)
				{
					zcorner[ix] = xedge1[ix];
					zcorner[ix + area - l] = xedge2[ix];
				}
				pos2 = 0;

				for (unsigned short iy = 0; iy + 1 < b; ++iy)
				{
					for (unsigned short ix = 0; ix + 1 < l; ++ix)
					{
						cubeindex = 0;
						if (tissuearray2[pos2 + 1] == tissue)
							cubeindex |= 1;
						if (tissuearray2[pos2] == tissue)
							cubeindex |= 2;
						if (tissuearray2[pos2 + l] == tissue)
							cubeindex |= 4;
						if (tissuearray2[pos2 + 1 + l] == tissue)
							cubeindex |= 8;

						if (ptTable[cubeindex] & 2)
						{
							edgeindex[1] = zcorner[pos2];
						}
						if (ptTable[cubeindex] & 1)
						{
							edgeindex[0] = zcorner[pos2 + 1];
						}
						if (ptTable[cubeindex] & 8)
						{
							if (ix + 2 == l || iy + 2 == b)
							{
								edgeindex[3] = zcorner[pos2 + 1 + l];
							}
							else
							{
								edgeindex[3] = index;
								K.x = (1.0f + ix) * dx;
								K.y = (1.0f + iy) * dy;
								//						K.z=0.0f;
								vertices.push_back(K);
								zcorner[pos2 + l + 1] = index;
								index++;
							}
						}
						if (ptTable[cubeindex] & 4)
						{
							edgeindex[2] = zcorner[pos2 + l];
						}
						if (ptTable[cubeindex] & 16)
						{
							edgeindex[4] = xind[pos2];
						}
						if (ptTable[cubeindex] & 128)
						{
							edgeindex[7] = yind[pos2 + 1];
						}
						if (ptTable[cubeindex] & 64)
						{
							edgeindex[6] = xind[pos2 + l];
						}
						if (ptTable[cubeindex] & 32)
						{
							edgeindex[5] = yind[pos2];
						}

						//					float x1,y1;
						//						xmin=xmax=vertices[edgeindex[triTable_1[cubeindex][0]]].x;
						for (short i = 0; triTable_1[cubeindex][i] != -1; i++)
						{
							trivec.push_back(
									edgeindex[triTable_1[cubeindex][i]]);
							//						xmin=min(xmin,vertices[edgeindex[triTable_1[cubeindex][i]]].x);
							//						xmax=max(xmax,vertices[edgeindex[triTable_1[cubeindex][i]]].x);
							//						if((xmax-xmin)>1.5f)
							//							fprintf(fpx,"%f %f %f %u\n",vertices[edgeindex[triTable_1[cubeindex][i]]].x,vertices[edgeindex[triTable_1[cubeindex][i]]].y,vertices[edgeindex[triTable_1[cubeindex][i]]].z,(unsigned)edgeindex[triTable_1[cubeindex][i]]);
							//fprintf(fp,"%u ",edgeindex[triTable_1[cubeindex][i]]);
							/*if(i%3==0) {
														x1=vertices[edgeindex[triTable_1[cubeindex][i]]].x;
														y1=vertices[edgeindex[triTable_1[cubeindex][i]]].y;
													}
													else {
														if(abs(x1-vertices[edgeindex[triTable_1[cubeindex][i]]].x)>1.5f
															||abs(y1-vertices[edgeindex[triTable_1[cubeindex][i]]].y)>1.5f
														)
															fprintf(fpx,"yyy %f %f %f %u\n",vertices[edgeindex[triTable[cubeindex][i]]].x,vertices[edgeindex[triTable[cubeindex][i]]].y,vertices[edgeindex[triTable[cubeindex][i]]].z,(unsigned)edgeindex[triTable[cubeindex][i]]);
													}*/
						}

						pos1++;
						pos2++;
					}
					pos1++;
					pos2++;
				}

				//		fprintf(fp,"0 0 0 \n");
				fp = triangprint(fp, trivec);
			}

			fp = verticeprint(fp, vertices);

			fclose(fp);
			//		fclose(fpx);

			free(xind);
			free(yind);
			free(zind);
			free(xindold);
			free(yindold);
			free(xedge1);
			free(yedge1);
			free(xedge2);
			free(yedge2);
			free(xedge1old);
			free(yedge1old);
			free(xedge2old);
			free(yedge2old);
			free(zcorner);
		}
		return;
	}

	void marchingcubeprint(const char* filename,
			std::vector<tissues_size_t>& tissuevec,
			std::vector<std::string>& vstring)
	{
		RGB rgb1;
		std::vector<RGB> colorvec;
		unsigned nr = (unsigned)tissuevec.size();
		if (nr > 1)
		{
			unsigned nr1 =
					(unsigned)ceil(pow(double(nr), 0.33333333) - 0.000000001);
			//		cout << nr << " " <<nr1<<endl;
			float step = 1.0f / (nr1 - 1);

			for (unsigned i = 0; i < nr; i++)
			{
				rgb1.r = step * (i / (nr1 * nr1));
				rgb1.g = step * ((i % (nr1 * nr1)) / nr1);
				rgb1.b = step * (i % nr1);
				colorvec.push_back(rgb1);
			}
		}
		else
		{
			rgb1.r = rgb1.g = rgb1.b = 1.0f;
			colorvec.push_back(rgb1);
		}

		return marchingcubeprint(filename, tissuevec, colorvec, vstring);
	}

	vectissuedescr* readin(const char* filename)
	{
		vectissuedescr* tissdescvec =
				new vectissuedescr; //(std::vector<tissuedescript> *)
		FILE* fp;
		if ((fp = fopen(filename, "rb")) == NULL)
			return NULL;

		unsigned nr;
		fread(&nr, sizeof(unsigned), 1, fp);

		if (nr == 0)
			return tissdescvec;

		tissuedescript ts;

		std::vector<V3F>* vertices = new std::vector<V3F>;

		for (unsigned i = 0; i < nr; i++)
		{
			fread(ts.rgb, 3 * sizeof(float), 1, fp);
			int l1;
			fread(&l1, sizeof(int), 1, fp);
			char* st = (char*)malloc(sizeof(char) * (l1 + 1));
			fread(st, sizeof(char) * (l1 + 1), 1, fp);
			ts.name.assign(st);
			free(st);
			ts.vertex_array = vertices; //(std::vector<V3F> *)
			ts.index_array =
					new std::vector<unsigned>; //(std::vector<unsigned> *)
			//			verticeread(fp,ts.vertex_array);
			triangread(fp, ts.index_array);
			tissdescvec->vtd.push_back(ts);
		}

		verticeread(fp, vertices);

		fclose(fp);
		return tissdescvec;
	}

	void vectissuedescr2bin(const char* filename, vectissuedescr* vtd)
	{
		FILE* fp;
		if ((fp = fopen(filename, "wb")) == NULL)
			return;
		unsigned dummy1 = (unsigned)vtd->vtd.size();
		fwrite(&dummy1, sizeof(unsigned), 1, fp);

		for (std::vector<tissuedescript>::iterator it = vtd->vtd.begin();
				 it != vtd->vtd.end(); it++)
		{
			float colordummy[3] = {1.0f, 1.0f, 1.0f};
			fwrite(colordummy, sizeof(float) * 3, 1, fp);

			int l1 = (int)it->name.size();
			fwrite(&l1, sizeof(int), 1, fp);
			fwrite(it->name.c_str(), sizeof(char) * (l1 + 1), 1, fp);

			fp = triangprint(fp, *(it->index_array));
		}

		unsigned nr = (unsigned)vtd->vtd.begin()->vertex_array->size();
		fwrite(&nr, sizeof(unsigned), 1, fp);

		float* koordarray = (float*)malloc(nr * 3 * sizeof(float));
		std::vector<V3F>::iterator it1 =
				vtd->vtd.begin()->vertex_array->begin();
		for (unsigned i = 0; i < nr; i++)
		{
			koordarray[3 * i] = it1->v[0];
			koordarray[3 * i + 1] = it1->v[1];
			koordarray[3 * i + 2] = it1->v[2];
			it1++;
		}

		fwrite(koordarray, nr * 3 * sizeof(float), 1, fp);
		free(koordarray);

		fclose(fp);
	}

private:
	tissues_size_t** tissuearray;
	unsigned short l;
	unsigned short b;
	unsigned short h;
	float slicedist;
	float dx;
	float dy;

	inline FILE* verticeprint(FILE* fp, std::vector<Koord>& vertices)
	{
		unsigned nr = (unsigned)vertices.size();
		fwrite(&nr, sizeof(unsigned), 1, fp);
		float* koordarray = (float*)malloc(nr * 3 * sizeof(float));
		std::vector<Koord>::iterator it = vertices.begin();
		for (unsigned i = 0; i < nr; i++)
		{
			koordarray[3 * i] = it->x;
			koordarray[3 * i + 1] = it->y;
			koordarray[3 * i + 2] = it->z;
			it++;
		}
		//	fprintf(fp,"%u: ",(unsigned)vertices.size());
		/*	for(std::vector<Koord>::iterator it=vertices.begin();it!=vertices.end();it++){
				fprintf(fp,"%f %f %f, ",it->x,it->y,it->z);
			}*/
		fwrite(koordarray, nr * 3 * sizeof(float), 1, fp);
		free(koordarray);
		return fp;
	}

	inline FILE* triangprint(FILE* fp, std::vector<unsigned>& trivec)
	{
		unsigned nr = (unsigned)trivec.size();
		fwrite(&nr, sizeof(unsigned), 1, fp);
		unsigned* triangarray = (unsigned*)malloc(nr * sizeof(unsigned));
		std::vector<unsigned>::iterator it = trivec.begin();
		for (unsigned i = 0; i < nr; i++)
		{
			triangarray[i] = *it;
			it++;
		}
		fwrite(triangarray, nr * sizeof(unsigned), 1, fp);
		free(triangarray);
		return fp;
	}

	inline FILE* verticeread(FILE* fp, std::vector<V3F>* vertices)
	{
		V3F vertex;
		unsigned nr;
		fread(&nr, sizeof(unsigned), 1, fp);
		float* koordarray = (float*)malloc(nr * 3 * sizeof(float));
		fread(koordarray, nr * 3 * sizeof(float), 1, fp);
		for (unsigned i = 0; i < nr; i++)
		{
			vertex.v[0] = koordarray[i * 3];
			vertex.v[1] = koordarray[i * 3 + 1];
			vertex.v[2] = koordarray[i * 3 + 2];
			vertices->push_back(vertex);
		}
		free(koordarray);
		return fp;
	}

	inline FILE* triangread(FILE* fp, std::vector<unsigned>* triangs)
	{
		unsigned nr;
		fread(&nr, sizeof(unsigned), 1, fp);
		unsigned* koordarray = (unsigned*)malloc(nr * sizeof(unsigned));
		fread(koordarray, nr * sizeof(unsigned), 1, fp);
		for (unsigned i = 0; i < nr; i++)
		{
			triangs->push_back(koordarray[i]);
		}
		free(koordarray);
		return fp;
	}
};

ISEG_CORE_API vectissuedescr* hypermeshascii_read(const char* filename);

} // namespace iseg
