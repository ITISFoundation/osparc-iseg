/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "LevelsetAddon.h"

#include "Levelset.h"

namespace iseg { namespace plugin {

namespace {
CLevelsetAddon _register_addon;
}

CLevelsetAddon::CLevelsetAddon() {}

CLevelsetAddon::~CLevelsetAddon() {}

QWidget1 *CLevelsetAddon::CreateWidget(QWidget *parent, const char *name,
																			 Qt::WindowFlags wFlags) const
{
	return new CLevelset(SliceHandler(), parent, name, wFlags);
}

}} // namespace iseg::plugin