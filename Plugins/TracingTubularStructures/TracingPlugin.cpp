/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
TracingPlugin register_addon;
AutoTracingPlugin register_addon2;
} // namespace

iseg::WidgetInterface* TracingPlugin::CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new TraceTubesWidget(SliceHandler(), parent, name, wFlags);
}

iseg::WidgetInterface* AutoTracingPlugin::CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new AutoTubeWidget(SliceHandler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
