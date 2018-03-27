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

class CConfidenceAddon : public CAddon
{
public:
	CConfidenceAddon();
	~CConfidenceAddon();

	virtual std::string Name() const { return "Example Addon 1"; }

	virtual std::string Description() const { return "Hello World"; }

	virtual QWidget1 *CreateWidget(QWidget *parent, const char *name,
																 Qt::WindowFlags wFlags) const;
};
}} // namespace iseg::plugin
