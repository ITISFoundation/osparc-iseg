/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "GraphCutAddon.h"

#include "GraphCutBoneSegmentation.h"
#include "GraphCutTissueSeparator.h"

namespace iseg { namespace plugin {

namespace {
CGCBoneSegmentationAddon _register_bone_seg;
//CGCTissueSeparatorAddon  _register_tissue_sep;
} // namespace

CGCBoneSegmentationAddon::CGCBoneSegmentationAddon() {}

CGCBoneSegmentationAddon::~CGCBoneSegmentationAddon() {}

QWidget1 *CGCBoneSegmentationAddon::CreateWidget(QWidget *parent,
																								 const char *name,
																								 Qt::WindowFlags wFlags) const
{
	return new CGraphCutBoneSegmentation(SliceHandler(), parent, name, wFlags);
}

//////////////////////////////////////////////////////////////////////////
CGCTissueSeparatorAddon::CGCTissueSeparatorAddon() {}

CGCTissueSeparatorAddon::~CGCTissueSeparatorAddon() {}

QWidget1 *CGCTissueSeparatorAddon::CreateWidget(QWidget *parent,
																								const char *name,
																								Qt::WindowFlags wFlags) const
{
	return new CGraphCutTissueSeparator(SliceHandler(), parent, name, wFlags);
}

}} // namespace iseg::plugin