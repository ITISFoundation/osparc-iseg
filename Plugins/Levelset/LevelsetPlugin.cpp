/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "LevelsetPlugin.h"

#include "Levelset.h"

namespace iseg { namespace plugin {

namespace {
LevelsetPlugin _register_addon;
}

LevelsetPlugin::LevelsetPlugin() {}

LevelsetPlugin::~LevelsetPlugin() {}

WidgetInterface* LevelsetPlugin::create_widget(QWidget* parent, const char* name,
		Qt::WindowFlags wFlags) const
{
	return new LevelsetWidget(slice_handler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
