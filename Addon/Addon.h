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

#include "AddonApi.h"

#include "SlicesHandlerInterface.h"
#include "qwidget1.h"

#include <string>
#include <vector>

namespace iseg {
class CSliceHandlerInterface;
}

namespace iseg { namespace plugin {

class ADDON_API CAddon
{
public:
	CAddon();
	~CAddon();

	void SetSliceHandler(CSliceHandlerInterface *slice_handler)
	{
		_slice_handler = slice_handler;
	}
	CSliceHandlerInterface *SliceHandler() const { return _slice_handler; }

	virtual std::string Name() const = 0;

	virtual std::string Description() const = 0;

	virtual QWidget1 *CreateWidget(QWidget *parent, const char *name,
																 Qt::WindowFlags wFlags) const = 0;

private:
	CSliceHandlerInterface *_slice_handler;
};

class ADDON_API CAddonRegistry
{
public:
	static std::vector<CAddon *> GetAllAddons();
};
}} // namespace iseg::plugin
