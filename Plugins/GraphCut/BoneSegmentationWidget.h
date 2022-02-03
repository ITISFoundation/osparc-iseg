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

#include "Interface/WidgetInterface.h"

#include "Data/Property.h"
#include "Data/SlicesHandlerInterface.h"

namespace itk {
class ProcessObject;
}

class BoneSegmentationWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	BoneSegmentationWidget(iseg::SlicesHandlerInterface* hand3D);
	~BoneSegmentationWidget() override;
	void Init() override;
	void NewLoaded() override;
	std::string GetName() override { return std::string("CT Auto-Bone"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("graphcut.png"))); }

private:
	void OnSlicenrChanged() override;

	void Showsliders();

	iseg::SlicesHandlerInterface* m_Handler3D;
	unsigned short m_CurrentSlice;

	std::shared_ptr<iseg::PropertyEnum> m_MaxFlowAlgorithm;
	std::shared_ptr<iseg::PropertyBool> m_M6Connectivity;
	std::shared_ptr<iseg::PropertyBool> m_UseSliceRange;
	std::shared_ptr<iseg::PropertyInt> m_Start;
	std::shared_ptr<iseg::PropertyInt> m_End;

	itk::ProcessObject* m_CurrentFilter;

private slots:
	void DoWork();
	void Cancel();
};
