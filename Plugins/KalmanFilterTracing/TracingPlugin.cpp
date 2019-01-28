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

#include "AutoTubePanel.h"

namespace iseg { namespace plugin {

namespace {
PanelAutoTracingPlugin _register_addon3;
} // namespace

iseg::WidgetInterface* PanelAutoTracingPlugin::create_widget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new AutoTubePanel(slice_handler(), parent, name, wFlags);
}
}} // namespace iseg::plugin
