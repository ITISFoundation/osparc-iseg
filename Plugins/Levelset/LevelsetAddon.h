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

#include "Addon/Addon.h"

namespace iseg { namespace plugin {

class CLevelsetAddon : public CAddon
{
public:
	CLevelsetAddon();
	~CLevelsetAddon();

	virtual std::string Name() const { return "Levelset"; }

	virtual std::string Description() const { return "LevelSet Plugin"; }

	virtual QWidget1 *CreateWidget(QWidget *parent, const char *name,
																 Qt::WindowFlags wFlags) const;
};
}} // namespace iseg::plugin
