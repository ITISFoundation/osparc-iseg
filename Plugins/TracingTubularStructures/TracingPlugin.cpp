/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "TracingPlugin.h"

#include "AutoTubeWidget.h"
#include "TraceTubesWidget.h"

namespace iseg { namespace plugin {

namespace {
TracingPlugin _register_addon;
AutoTracingPlugin _register_addon2;
} // namespace

iseg::WidgetInterface* TracingPlugin::create_widget(QWidget* parent, const char* name,
		Qt::WindowFlags wFlags) const
{
	return new TraceTubesWidget(slice_handler(), parent, name, wFlags);
}

iseg::WidgetInterface* AutoTracingPlugin::create_widget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new AutoTubeWidget(slice_handler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
