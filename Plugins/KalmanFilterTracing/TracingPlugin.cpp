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

#include "AutoTubePanel.h"

namespace iseg { namespace plugin {

namespace {
PanelAutoTracingPlugin register_addon3;
} // namespace

iseg::WidgetInterface* PanelAutoTracingPlugin::CreateWidget() const
{
	return new AutoTubePanel(SliceHandler());
}
}} // namespace iseg::plugin
