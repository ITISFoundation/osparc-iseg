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

#include "Interface/Plugin.h"

namespace iseg { namespace plugin {

class PanelAutoTracingPlugin : public Plugin
{
public:
	PanelAutoTracingPlugin() = default;
	~PanelAutoTracingPlugin() = default;
	std::string Name() const override { return "Panel AutoTracing Plugin "; }

	std::string Description() const override { return "Panel AutoTracing Plugin"; }

	WidgetInterface* CreateWidget() const override;
};

}} // namespace iseg::plugin
