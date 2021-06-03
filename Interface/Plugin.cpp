/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#include "Plugin.h"

namespace iseg { namespace plugin {

namespace {
std::vector<Plugin*> addons;
}

std::vector<Plugin*> PluginRegistry::RegisteredPlugins() { return addons; }

Plugin::Plugin() { addons.push_back(this); }



void Plugin::InstallSliceHandler(SlicesHandlerInterface* slice_handler)
{
	m_SliceHandler = slice_handler;
}

}} // namespace iseg::plugin
