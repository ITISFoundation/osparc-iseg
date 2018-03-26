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


#define TISSUES_SIZE_TYPEDEF // TODO: Used to mark file IO where change of tissues size type still needs to be handled.

/*
* Tissues size type
*/
#ifdef TISSUES_SIZE_TYPEDEF

#define TISSUES_SIZE_MAX 65535
typedef unsigned short tissues_size_t;

#else // TISSUES_SIZE_TYPEDEF

#define TISSUES_SIZE_MAX 255
typedef unsigned char tissues_size_t; // Original tissues size type

#endif // TISSUES_SIZE_TYPEDEF


/*
* Tissue layers size type
*/
#define TISSUELAYERS_SIZE_MAX 255
typedef unsigned char tissuelayers_size_t;
