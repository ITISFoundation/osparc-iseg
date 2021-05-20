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

#include "Data/Vec3.h"

#include "Core/Pair.h"

#include <qfiledialog.h>
#include <qlabel.h>

#include <vector>

namespace iseg {

VesselWidget::VesselWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	m_Vbox1 = new Q3VBox(this);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_TxtInfo = new QLabel("Thresholds: 1000,1150,1250,1300", m_Vbox1);
	m_PbExec = new QPushButton("Execute", m_Vbox1);
	m_PbStore = new QPushButton("Save...", m_Vbox1);
	m_PbStore->setEnabled(false);

	m_TxtStart = new QLabel("Start Pt.: ", m_Hbox1);
	m_CbbLb1 = new QComboBox(m_Hbox1);
	m_TxtNrend = new QLabel("Nr. of Pts.: ", m_Hbox2);
	m_SbNrend = new QSpinBox(1, 20, 1, m_Hbox2);
	m_SbNrend->setValue(1);
	m_TxtEndnr = new QLabel("Pt. Nr.: ", m_Hbox3);
	m_SbEndnr = new QSpinBox(1, m_SbNrend->value(), 1, m_Hbox3);
	m_SbEndnr->setValue(1);
	m_TxtEnd = new QLabel(" End Pt.: ", m_Hbox3);
	m_CbbLb2 = new QComboBox(m_Hbox3);

	m_Hbox1->setFixedSize(m_Hbox1->sizeHint());
	m_Hbox2->setFixedSize(m_Hbox2->sizeHint());
	m_Hbox3->setFixedSize(m_Hbox3->sizeHint());
	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	//	setFixedSize(vbox1->size());

	AugmentedMark am;
	am.mark = 0;
	am.name = "";
	am.p.px = 12345;
	am.p.py = 12345;
	am.slicenr = 12345;
	m_Selectedlabels.push_back(am);
	m_Selectedlabels.push_back(am);

	QObject_connect(m_SbNrend, SIGNAL(valueChanged(int)), this, SLOT(NrendChanged(int)));
	QObject_connect(m_SbEndnr, SIGNAL(valueChanged(int)), this, SLOT(EndnrChanged(int)));
	QObject_connect(m_PbExec, SIGNAL(clicked()), this, SLOT(Execute()));
	QObject_connect(m_PbStore, SIGNAL(clicked()), this, SLOT(Savevessel()));
	QObject_connect(m_CbbLb1, SIGNAL(activated(int)), this, SLOT(Cbb1Changed(int)));
	QObject_connect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(Cbb2Changed(int)));

	MarksChanged();
}

VesselWidget::~VesselWidget() { delete m_Vbox1; }

QSize VesselWidget::sizeHint() const { return m_Vbox1->sizeHint(); }

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
	QObject_disconnect(m_CbbLb1, SIGNAL(activated(int)), this, SLOT(Cbb1Changed(int)));
	QObject_disconnect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(Cbb2Changed(int)));
	m_CbbLb1->clear();
	m_CbbLb2->clear();

	for (size_t i = 0; i < m_Labels.size(); i++)
	{
		m_CbbLb1->insertItem(QString(m_Labels[i].name.c_str()));
		m_CbbLb2->insertItem(QString(m_Labels[i].name.c_str()));
	}

	for (size_t i = 0; i < m_Selectedlabels.size(); i++)
	{
		size_t j = 0;
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
					m_CbbLb1->setCurrentItem(0);
				}
				else if ((int)i == m_SbEndnr->value())
				{
					m_CbbLb2->setCurrentItem(0);
				}
			}
		}
		else
		{
			if (i == 0)
			{
				m_CbbLb1->setCurrentItem(j);
			}
			else if ((int)i == m_SbEndnr->value())
			{
				m_CbbLb2->setCurrentItem(j);
			}
		}
	}

	QObject_connect(m_CbbLb1, SIGNAL(activated(int)), this, SLOT(Cbb1Changed(int)));
	QObject_connect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(Cbb2Changed(int)));

	if (m_Labels.empty())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_TxtInfo->setText("No labels exist.");
		m_PbExec->hide();
	}
	else
	{
		m_Hbox1->show();
		m_Hbox2->show();
		m_Hbox3->show();
		m_TxtInfo->setText("Thresholds: 1000,1150,1250,1300");
		m_PbExec->show();
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
	for (int i = 1; i <= m_SbNrend->value(); i++)
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
			m_PbStore->setEnabled(true);
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
	m_SbEndnr->setMaxValue(newval);
	m_SbEndnr->setValue(1);
}

void VesselWidget::EndnrChanged(int newval)
{
	QObject_disconnect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(Cbb2Changed(int)));

	size_t i = 0;
	while ((i < m_Labels.size()) && (m_Labels[i] != m_Selectedlabels[newval]))
		i++;
	m_CbbLb2->setCurrentItem(i);

	QObject_connect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(Cbb2Changed(int)));
}

void VesselWidget::Cbb1Changed(int newval)
{
	m_Selectedlabels[0] = m_Labels[newval];
}

void VesselWidget::Cbb2Changed(int newval)
{
	m_Selectedlabels[m_SbEndnr->value()] = m_Labels[newval];
}

void VesselWidget::NewLoaded() { ResetBranchTree(); }

void VesselWidget::ResetBranchTree()
{
	m_BranchTree.Clear();
	m_Vp.clear();
	emit Vp1Changed(&m_Vp);
	m_PbStore->setEnabled(false);
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
	QString savefilename = QFileDialog::getSaveFileName(QString::null, "Vessel-Tracks (*.txt)\n", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		std::vector<std::vector<Vec3>> vp;
		Pair pair1 = m_Handler3D->GetPixelsize();
		float thick = m_Handler3D->GetSlicethickness();
		float epsilon = std::max(std::max(pair1.high, pair1.low), thick);
		m_BranchTree.GetItem()->DougPeuckInclchildren(epsilon, pair1.high, pair1.low, thick, vp);
		FILE* fp = fopen(savefilename.ascii(), "w");
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
	return QIcon(picdir.absFilePath(QString("vessel.png")));
}

} // namespace iseg
