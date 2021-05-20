/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "LevelsetPlugin.h"

#include "LevelsetWidget.h"

namespace iseg { namespace plugin {

namespace {
LevelsetPlugin register_addon;
}

LevelsetPlugin::LevelsetPlugin() = default;

LevelsetPlugin::~LevelsetPlugin() = default;

WidgetInterface* LevelsetPlugin::CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new LevelsetWidget(SliceHandler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
