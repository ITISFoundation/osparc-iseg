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

class GCBoneSegmentationPlugin : public Plugin
{
public:
	GCBoneSegmentationPlugin();
	~GCBoneSegmentationPlugin();

	std::string Name() const override { return "CT Auto-Bone"; }
	std::string Description() const override;
	WidgetInterface* CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const override;
};

class GCTissueSeparatorPlugin : public Plugin
{
public:
	GCTissueSeparatorPlugin();
	~GCTissueSeparatorPlugin();

	std::string Name() const override { return "Tissue Separator"; }
	std::string Description() const override;
	WidgetInterface* CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const override;
};

}} // namespace iseg::plugin
