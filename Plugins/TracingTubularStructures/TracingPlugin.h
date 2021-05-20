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

class TracingPlugin : public Plugin
{
public:
	TracingPlugin() = default;
	~TracingPlugin() = default;

	std::string Name() const override { return "Tracing Plugin "; }

	std::string Description() const override { return "Tracing Plugin"; }

	WidgetInterface* CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const override;
};

class AutoTracingPlugin : public Plugin
{
public:
	AutoTracingPlugin() = default;
	~AutoTracingPlugin() = default;

	std::string Name() const override { return "AutoTracing Plugin "; }

	std::string Description() const override { return "AutoTracing Plugin"; }

	WidgetInterface* CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const override;
};

}} // namespace iseg::plugin
