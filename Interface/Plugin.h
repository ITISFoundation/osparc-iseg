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

#include "iSegInterface.h"

#include "WidgetInterface.h"

#include "Data/SlicesHandlerInterface.h"

#include <string>
#include <vector>

namespace iseg {
class SlicesHandlerInterface;
}

namespace iseg { namespace plugin {

class ISEG_INTERFACE_API Plugin
{
public:
	Plugin();
	~Plugin() = default;

	void InstallSliceHandler(SlicesHandlerInterface* slice_handler);
	SlicesHandlerInterface* SliceHandler() const { return m_SliceHandler; }

	virtual std::string Name() const = 0;

	virtual std::string Description() const = 0;

	virtual WidgetInterface* CreateWidget() const = 0;

private:
	SlicesHandlerInterface* m_SliceHandler;
};

class ISEG_INTERFACE_API PluginRegistry
{
public:
	static std::vector<Plugin*> RegisteredPlugins();
};

}} // namespace iseg::plugin
