/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
ConfidencePlugin register_addon;
}

ConfidencePlugin::ConfidencePlugin() = default;

ConfidencePlugin::~ConfidencePlugin() = default;

WidgetInterface* ConfidencePlugin::CreateWidget() const
{
	return new ConfidenceWidget(SliceHandler());
}

}} // namespace iseg::plugin
