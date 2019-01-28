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

#include "Interface/Plugin.h"

namespace iseg { namespace plugin {

class PanelAutoTracingPlugin : public Plugin
{
public:
	PanelAutoTracingPlugin() {}
	~PanelAutoTracingPlugin() {}
	virtual std::string name() const { return "Panel AutoTracing Plugin "; }

	virtual std::string description() const { return "Panel AutoTracing Plugin"; }

	virtual WidgetInterface* create_widget(QWidget* parent, const char* name,
			Qt::WindowFlags wFlags) const;
};

}} // namespace iseg::plugin
