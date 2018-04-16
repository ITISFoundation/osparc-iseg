/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "ConfidencePlugin.h"

#include "ConfidenceWidget.h"

namespace iseg { namespace plugin {

namespace {
ConfidencePlugin _register_addon;
}

ConfidencePlugin::ConfidencePlugin() {}

ConfidencePlugin::~ConfidencePlugin() {}

WidgetInterface* ConfidencePlugin::create_widget(QWidget* parent, const char* name,
		Qt::WindowFlags wFlags) const
{
	return new ConfidenceWidget(slice_handler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
