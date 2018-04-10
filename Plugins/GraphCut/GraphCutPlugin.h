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

	std::string name() const override { return "CT Auto-Bone"; }
	std::string description() const override;
	WidgetInterface* create_widget(QWidget* parent, const char* name,
			Qt::WindowFlags wFlags) const override;
};

class CGCTissueSeparatorPlugin : public Plugin
{
public:
	CGCTissueSeparatorPlugin();
	~CGCTissueSeparatorPlugin();

	std::string name() const override { return "Tissue Separator"; }
	std::string description() const override;
	WidgetInterface* create_widget(QWidget* parent, const char* name,
			Qt::WindowFlags wFlags) const override;
};

}} // namespace iseg::plugin
