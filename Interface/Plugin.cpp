/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
std::vector<Plugin*> _addons;
}

std::vector<Plugin*> PluginRegistry::registered_plugins() { return _addons; }

Plugin::Plugin() { _addons.push_back(this); }

Plugin::~Plugin() {}

void Plugin::install_slice_handler(SliceHandlerInterface* slice_handler)
{
	_slice_handler = slice_handler;
}

}} // namespace iseg::plugin
