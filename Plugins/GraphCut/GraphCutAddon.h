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

class GCBoneSegmentationPlugin : public Plugin
{
public:
	GCBoneSegmentationPlugin();
	~GCBoneSegmentationPlugin();

	virtual std::string name() const { return "CT Auto-Bone"; }

	virtual std::string description() const;

	virtual WidgetInterface* create_widget(QWidget* parent, const char* name,
										   Qt::WindowFlags wFlags) const;
};

class CGCTissueSeparatorPlugin : public Plugin
{
public:
	CGCTissueSeparatorPlugin();
	~CGCTissueSeparatorPlugin();

	virtual std::string name() const { return "Tissue Separator"; }

	virtual std::string description() const;

	virtual WidgetInterface* create_widget(QWidget* parent, const char* name,
										   Qt::WindowFlags wFlags) const;
};

}} // namespace iseg::plugin
