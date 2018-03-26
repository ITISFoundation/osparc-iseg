/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SHEWCHUK_PREDICATES_H
#define SHEWCHUK_PREDICATES_H

double exactinit();
double getepsilon();
double orient2d(double *pa, double *pb, double *pc);
double orient3d(double *pa, double *pb, double *pc, double *pd);

#endif
