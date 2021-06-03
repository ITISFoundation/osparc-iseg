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

#if defined(WIN32)
#if defined(vtkGDCM_EXPORTS)
#define vtkGDCM_API __declspec(dllexport)
#else
#define vtkGDCM_API __declspec(dllimport)
#endif
#else
#define vtkGDCM_API
#endif
