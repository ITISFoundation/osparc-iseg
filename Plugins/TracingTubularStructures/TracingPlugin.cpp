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

iseg::WidgetInterface* TracingPlugin::CreateWidget() const
{
	return new TraceTubesWidget(SliceHandler());
}

iseg::WidgetInterface* AutoTracingPlugin::CreateWidget() const
{
	return new AutoTubeWidget(SliceHandler());
}

}} // namespace iseg::plugin
