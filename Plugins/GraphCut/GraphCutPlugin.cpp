/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "GraphCutPlugin.h"

#include "BoneSegmentationWidget.h"
#include "TissueSeparatorWidget.h"

namespace iseg { namespace plugin {

namespace {
GCBoneSegmentationPlugin _register_bone_seg;
GCTissueSeparatorPlugin _register_tissue_sep;
} // namespace

GCBoneSegmentationPlugin::GCBoneSegmentationPlugin() {}

GCBoneSegmentationPlugin::~GCBoneSegmentationPlugin() {}

std::string GCBoneSegmentationPlugin::description() const
{
	return "Automatic bone segmentation in CT images using graph-cut method.";
}

WidgetInterface* GCBoneSegmentationPlugin::create_widget(QWidget* parent,
		const char* name,
		Qt::WindowFlags wFlags) const
{
	return new BoneSegmentationWidget(slice_handler(), parent, name, wFlags);
}

//////////////////////////////////////////////////////////////////////////
GCTissueSeparatorPlugin::GCTissueSeparatorPlugin() {}

GCTissueSeparatorPlugin::~GCTissueSeparatorPlugin() {}

std::string GCTissueSeparatorPlugin::description() const
{
	return "Separate tissue using only minor user-input.";
}

WidgetInterface* GCTissueSeparatorPlugin::create_widget(QWidget* parent,
		const char* name,
		Qt::WindowFlags wFlags) const
{
	return new TissueSeparatorWidget(slice_handler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
