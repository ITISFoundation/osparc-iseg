/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "SlicesHandler.h"
#include "VesselWidget.h"

#include "Interface/PropertyWidget.h"

#include "Core/Pair.h"

#include "Data/Vec3.h"

#include <QFileDialog>
#include <QHBoxLayout>

#include <vector>

namespace iseg {

VesselWidget::VesselWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	AugmentedMark am;
	am.mark = 0;
	am.name = "";
	am.p.px = 12345;
	am.p.py = 12345;
	am.slicenr = 12345;
	m_Selectedlabels.push_back(am);
	m_Selectedlabels.push_back(am);

	// settings
	auto group = PropertyGroup::Create("Settings");

	m_TxtInfo = group->Add("TxtInfo", PropertyString::Create("1000,1150,1250,1300"));
	m_TxtInfo->SetDescription("Thresholds");

	m_CbbLb1 = group->Add("CbbLb1", PropertyEnum::Create());
	m_CbbLb1->SetDescription("Start Point");

	m_SbNrend = group->Add("SbNrend", PropertyInt::Create(1, 1, 20));
	m_SbNrend->SetDescription("Number of Points");

	m_SbEndnr = group->Add("SbEndnr", PropertyInt::Create(1, 1, m_SbNrend->Value()));
	m_SbEndnr->SetDescription("Point Number");

	m_CbbLb2 = group->Add("CbbLb2", PropertyEnum::Create());
	m_CbbLb2->SetDescription("End Point");

	m_PbExec = group->Add("Execute", PropertyButton::Create([this]() { Execute(); }, "Execute"));

	m_PbStore = group->Add("Save", PropertyButton::Create([this]() { Savevessel(); }, "Save..."));
	m_PbStore->SetEnabled(false);

	// connections
	m_SbNrend->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			NrendChanged(m_SbNrend->Value());
	});

	m_SbEndnr->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			EndnrChanged(m_SbEndnr->Value());
	});

	m_CbbLb1->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			Cbb1Changed(m_CbbLb1->Value());
	});

	m_CbbLb2->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			Cbb1Changed(m_CbbLb2->Value());
	});

	// add property view
	auto property_view = new PropertyWidget(group);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view, 2);
	layout->addStretch(1);
	setLayout(layout);

	MarksChanged();
}

void VesselWidget::Init()
{
	Getlabels();

	m_BranchTree.ResetIterator();
	m_Vp.clear();
	if (m_BranchTree.GetSize() > 0)
	{
		m_BranchTree.GetItem()->GetCenterListSliceInclchildren(m_Handler3D->ActiveSlice(), m_Vp);
	}
	emit Vp1Changed(&m_Vp);
}

FILE* VesselWidget::SaveParams(FILE* fp, int /* version */)
{
	return fp;
}

FILE* VesselWidget::LoadParams(FILE* fp, int /* version */)
{
	return fp;
}

void VesselWidget::Getlabels()
{
	m_Handler3D->GetLabels(&m_Labels);

	boost::signals2::shared_connection_block cb1_block(m_CbbLb1Connection);
	boost::signals2::shared_connection_block cb2_block(m_CbbLb2Connection);

	PropertyEnum::descriptions_type labels;
	for (size_t i = 0; i < m_Labels.size(); i++)
	{
		labels.push_back(m_Labels[i].name);
	}
	m_CbbLb1->ReplaceDescriptions(labels);
	m_CbbLb2->ReplaceDescriptions(labels);

	for (PropertyEnum::value_type i = 0; i < m_Selectedlabels.size(); i++)
	{
		PropertyEnum::value_type j = 0;
		while ((j < m_Labels.size()) && (m_Labels[j] != m_Selectedlabels[i]))
			j++;
		if (j == m_Labels.size())
		{
			if (m_Labels.empty())
			{
				m_Selectedlabels[i].mark = 0;
				m_Selectedlabels[i].name = "";
				m_Selectedlabels[i].p.px = 12345;
				m_Selectedlabels[i].p.py = 12345;
				m_Selectedlabels[i].slicenr = 12345;
			}
			else
			{
				m_Selectedlabels[i] = m_Labels[0];
				if (i == 0)
				{
					m_CbbLb1->SetValue(0);
				}
				else if ((int)i == m_SbEndnr->Value())
				{
					m_CbbLb2->SetValue(0);
				}
			}
		}
		else
		{
			if (i == 0)
			{
				m_CbbLb1->SetValue(j);
			}
			else if ((int)i == m_SbEndnr->Value())
			{
				m_CbbLb2->SetValue(j);
			}
		}
	}

	m_CbbLb1->SetVisible(!m_Labels.empty());
	m_CbbLb2->SetVisible(!m_Labels.empty());
	m_SbNrend->SetVisible(!m_Labels.empty());
	m_SbEndnr->SetVisible(!m_Labels.empty());
	m_PbExec->SetVisible(!m_Labels.empty());

	if (m_Labels.empty())
	{
		m_TxtInfo->SetValue("No labels exist.");
	}
	else
	{
		m_TxtInfo->SetValue("Thresholds: 1000,1150,1250,1300");
	}
}

void VesselWidget::MarksChanged() { Getlabels(); }

void VesselWidget::Execute()
{
	World world;
	ResetBranchTree();

	// end point for dijkstra in voxel coordinates
	Vec3 end;
	end[0] = m_Selectedlabels[0].p.px;
	end[1] = m_Selectedlabels[0].p.py;
	end[2] = m_Selectedlabels[0].slicenr;

	std::vector<Vec3> seeds;		 // only distal seeds
	std::vector<Vec3> all_seeds; // seeds + end point

	// save all start seeds in s
	for (int i = 1; i <= m_SbNrend->Value(); i++)
	{
		int j = 0;
		while ((j < i) && (m_Selectedlabels[j] != m_Selectedlabels[i]))
			j++;
		if (j == i)
		{
			Vec3 start;
			start[0] = m_Selectedlabels[i].p.px;
			start[1] = m_Selectedlabels[i].p.py;
			start[2] = m_Selectedlabels[i].slicenr;
			all_seeds.push_back(start);
			seeds.push_back(start);
		}
	}

	all_seeds.push_back(end);

	if (!seeds.empty() && (m_Handler3D->NumSlices() > 0) &&
			(m_Handler3D->Width() > 0) && (m_Handler3D->Height() > 0))
	{
		Vec3 tmpbb_start;
		//tmpbbStart[0]=0;
		//tmpbbStart[1]=0;
		//tmpbbStart[2]=0;
		tmpbb_start[0] = 60;
		tmpbb_start[1] = 130;
		tmpbb_start[2] = 60;
		Vec3 tmpbb_end;
		//tmpbbEnd[0]=handler3D->return_width()-1;
		//tmpbbEnd[1]=handler3D->return_height()-1;
		//tmpbbEnd[2]=handler3D->return_nrslices()-1;
		tmpbb_end[0] = 400;
		tmpbb_end[1] = 450;
		tmpbb_end[2] = 180;

		// TODO compute bounding box for algorithm
		// otherwise the memory consumption is too large
		world.GetSeedBoundingBox(&all_seeds, tmpbb_start, tmpbb_end, m_Handler3D);

		world.SetBbStart(tmpbb_start);
		world.SetBbEnd(tmpbb_end);
		if (world.Init(tmpbb_start, tmpbb_end, m_Handler3D))
		{
			world.Dijkstra(seeds, end, &m_BranchTree);
			m_PbStore->SetEnabled(true);
			OnSlicenrChanged(); // BL Why?
		}
	}

}

void VesselWidget::NrendChanged(int newval)
{
	if (newval >= (int)m_Selectedlabels.size())
	{
		AugmentedMark am;
		am.mark = 0;
		am.name = "";
		am.p.px = 12345;
		am.p.py = 12345;
		am.slicenr = 12345;
		for (int i = (int)m_Selectedlabels.size(); i <= newval; i++)
		{
			m_Selectedlabels.push_back(am);
		}
	}
	else
	{
		m_Selectedlabels.resize(newval + 1);
	}
	m_SbEndnr->SetMaximum(newval);
	m_SbEndnr->SetValue(1);
}

void VesselWidget::EndnrChanged(int newval)
{
	boost::signals2::shared_connection_block cb2_block(m_CbbLb2Connection);

	PropertyEnum::value_type i = 0;
	while ((i < m_Labels.size()) && (m_Labels[i] != m_Selectedlabels[newval]))
		i++;
	m_CbbLb2->SetValue(i);
}

void VesselWidget::Cbb1Changed(int newval)
{
	m_Selectedlabels[0] = m_Labels[newval];
}

void VesselWidget::Cbb2Changed(int newval)
{
	m_Selectedlabels[m_SbEndnr->Value()] = m_Labels[newval];
}

void VesselWidget::NewLoaded() { ResetBranchTree(); }

void VesselWidget::ResetBranchTree()
{
	m_BranchTree.Clear();
	m_Vp.clear();
	emit Vp1Changed(&m_Vp);
	m_PbStore->SetEnabled(false);
}

void VesselWidget::OnSlicenrChanged()
{
	m_BranchTree.ResetIterator();
	m_Vp.clear();
	if (m_BranchTree.GetSize() > 0)
	{
		m_BranchTree.GetItem()->GetCenterListSliceInclchildren(m_Handler3D->ActiveSlice(), m_Vp);
	}
	emit Vp1Changed(&m_Vp);
}

void VesselWidget::Savevessel()
{
	QString savefilename = QFileDialog::getSaveFileName(this, "Save As", QString::null, "Vessel-Tracks (*.txt)\n");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		std::vector<std::vector<Vec3>> vp;
		Pair pair1 = m_Handler3D->GetPixelsize();
		float thick = m_Handler3D->GetSlicethickness();
		float epsilon = std::max(std::max(pair1.high, pair1.low), thick);
		m_BranchTree.GetItem()->DougPeuckInclchildren(epsilon, pair1.high, pair1.low, thick, vp);
		FILE* fp = fopen(savefilename.toAscii(), "w");
		int version = 2;
		unsigned short w = m_Handler3D->Width();
		unsigned short h = m_Handler3D->Height();
		fprintf(fp, "V%i\n", version);
		fprintf(fp, "NS%i\n", (int)m_Handler3D->NumSlices());
		fprintf(fp, "VoxelSize: %f %f %f\n", pair1.high / 2, pair1.low / 2, thick);
		fprintf(fp, "N%i\n", (int)vp.size());
		for (size_t i = 0; i < vp.size(); i++)
		{
			fprintf(fp, "PTS%i: ", (int)vp[i].size());
			for (size_t j = 0; j < vp[i].size(); j++)
			{
				fprintf(fp, "%i,%i,%i ", (int)w - 2 * (int)(vp[i][j][0]), 2 * (int)(vp[i][j][1]) - h, (int)(vp[i][j][2])); //xmirrored
			}
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
}

void VesselWidget::Cleanup()
{
	m_Vp.clear();
	emit Vp1Changed(&m_Vp);
}

QIcon iseg::VesselWidget::GetIcon(QDir picdir)
{
	return QIcon(picdir.absoluteFilePath("vessel.png"));
}

} // namespace iseg
