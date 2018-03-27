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

class CGCBoneSegmentationAddon : public CAddon
{
public:
	CGCBoneSegmentationAddon();
	~CGCBoneSegmentationAddon();

	virtual std::string Name() const { return "CT Auto-Bone"; }

	virtual std::string Description() const
	{
		return "Automatic bone segmentation in CT images using graph-cut method.";
	}

	virtual QWidget1 *CreateWidget(QWidget *parent, const char *name,
																 Qt::WindowFlags wFlags) const;
};

class CGCTissueSeparatorAddon : public CAddon
{
public:
	CGCTissueSeparatorAddon();
	~CGCTissueSeparatorAddon();

	virtual std::string Name() const { return "Tissue Separator"; }

	virtual std::string Description() const
	{
		return "Separate tissue using only minor user-input.";
	}

	virtual QWidget1 *CreateWidget(QWidget *parent, const char *name,
																 Qt::WindowFlags wFlags) const;
};
}} // namespace iseg::plugin
