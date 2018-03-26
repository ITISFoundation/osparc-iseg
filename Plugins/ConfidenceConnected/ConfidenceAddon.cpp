/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "ConfidenceAddon.h"

#include "Confidence.h"

namespace iseg { namespace plugin {

namespace
{
	CConfidenceAddon _register_addon;
}

CConfidenceAddon::CConfidenceAddon()
{

}

CConfidenceAddon::~CConfidenceAddon()
{

}

QWidget1* CConfidenceAddon::CreateWidget(QWidget *parent, const char *name, Qt::WindowFlags wFlags) const
{
	return new CConfidence(SliceHandler(), parent, name, wFlags);
}

}}