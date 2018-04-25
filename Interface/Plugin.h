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

#include "InterfaceApi.h"

#include "WidgetInterface.h"

#include "Data/SlicesHandlerInterface.h"

#include <string>
#include <vector>

namespace iseg {
class SliceHandlerInterface;
}

namespace iseg { namespace plugin {

class ISEG_INTERFACE_API Plugin
{
public:
	Plugin();
	~Plugin();

	void install_slice_handler(SliceHandlerInterface* slice_handler);
	SliceHandlerInterface* slice_handler() const { return _slice_handler; }

	virtual std::string name() const = 0;

	virtual std::string description() const = 0;

	virtual WidgetInterface* create_widget(QWidget* parent,
			const char* name, Qt::WindowFlags wFlags) const = 0;

private:
	SliceHandlerInterface* _slice_handler;
};

class ISEG_INTERFACE_API PluginRegistry
{
public:
	static std::vector<Plugin*> registered_plugins();
};

}} // namespace iseg::plugin
