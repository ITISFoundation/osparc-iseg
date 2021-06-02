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

class ConfidencePlugin : public Plugin
{
public:
	ConfidencePlugin();
	~ConfidencePlugin();

	std::string Name() const override { return "Example Addon 1"; }
	std::string Description() const override { return "Hello World"; }
	WidgetInterface* CreateWidget() const override;
};
}} // namespace iseg::plugin
