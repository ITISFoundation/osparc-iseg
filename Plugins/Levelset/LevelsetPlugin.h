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

class LevelsetPlugin : public Plugin
{
public:
	LevelsetPlugin();
	~LevelsetPlugin();

	std::string Name() const override { return "Levelset"; }
	std::string Description() const override { return "LevelSet Plugin"; }
	WidgetInterface* CreateWidget() const override;
};
}} // namespace iseg::plugin
