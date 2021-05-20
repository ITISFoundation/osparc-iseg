/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
GCBoneSegmentationPlugin register_bone_seg;
GCTissueSeparatorPlugin register_tissue_sep;
} // namespace

GCBoneSegmentationPlugin::GCBoneSegmentationPlugin() = default;

GCBoneSegmentationPlugin::~GCBoneSegmentationPlugin() = default;

std::string GCBoneSegmentationPlugin::Description() const
{
	return "Automatic bone segmentation in CT images using graph-cut method.";
}

WidgetInterface* GCBoneSegmentationPlugin::CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new BoneSegmentationWidget(SliceHandler(), parent, name, wFlags);
}

//////////////////////////////////////////////////////////////////////////
GCTissueSeparatorPlugin::GCTissueSeparatorPlugin() = default;

GCTissueSeparatorPlugin::~GCTissueSeparatorPlugin() = default;

std::string GCTissueSeparatorPlugin::Description() const
{
	return "Separate tissue using only minor user-input.";
}

WidgetInterface* GCTissueSeparatorPlugin::CreateWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags) const
{
	return new TissueSeparatorWidget(SliceHandler(), parent, name, wFlags);
}

}} // namespace iseg::plugin
