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

#include "SlicesHandler.h"
#include "World.h"

#include "Interface/WidgetInterface.h"

#include "Data/Property.h"

namespace iseg {

class VesselWidget : public WidgetInterface
{
	Q_OBJECT
public:
	VesselWidget(SlicesHandler* hand3D);
	~VesselWidget() override {}
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void Init() override;
	void NewLoaded() override;
	std::string GetName() override { return std::string("Vessel"); }
	QIcon GetIcon(QDir picdir) override;
	void Cleanup() override;

private:
	void OnSlicenrChanged() override;
	void MarksChanged() override;

	void NrendChanged(int);
	void EndnrChanged(int);
	void Cbb1Changed(int);
	void Cbb2Changed(int);
	void Execute();
	void Savevessel();

	void Getlabels();
	void ResetBranchTree();

	BranchTree m_BranchTree;
	SlicesHandler* m_Handler3D;
	std::vector<AugmentedMark> m_Labels;
	std::vector<AugmentedMark> m_Selectedlabels;
	std::vector<Point> m_Vp;

	std::shared_ptr<PropertyString> m_TxtInfo;
	std::shared_ptr<PropertyEnum> m_CbbLb1;
	std::shared_ptr<PropertyEnum> m_CbbLb2;
	std::shared_ptr<PropertyInt> m_SbNrend;
	std::shared_ptr<PropertyInt> m_SbEndnr;

	std::shared_ptr<PropertyButton> m_PbExec;
	std::shared_ptr<PropertyButton> m_PbStore;

	boost::signals2::scoped_connection m_CbbLb1Connection;
	boost::signals2::scoped_connection m_CbbLb2Connection;

signals:
	void Vp1Changed(std::vector<Point>* vp1);
};

} // namespace iseg
