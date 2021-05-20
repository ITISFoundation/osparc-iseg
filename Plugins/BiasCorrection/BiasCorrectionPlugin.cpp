/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "BiasCorrectionPlugin.h"

#include "BiasCorrection.h"

namespace iseg { namespace plugin {

namespace {
BiasCorrectionPlugin register_addon;
}

BiasCorrectionPlugin::BiasCorrectionPlugin() = default;

BiasCorrectionPlugin::~BiasCorrectionPlugin() = default;

WidgetInterface* BiasCorrectionPlugin::CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new BiasCorrectionWidget(SliceHandler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
