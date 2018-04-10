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

class BiasCorrectionPlugin : public Plugin
{
public:
	BiasCorrectionPlugin();
	~BiasCorrectionPlugin();

	std::string name() const override { return "Bias Addon "; }
	std::string description() const override { return "Bias Plugin"; }
	WidgetInterface* create_widget(QWidget* parent, const char* name,
			Qt::WindowFlags wFlags) const override;
};
}} // namespace iseg::plugin
