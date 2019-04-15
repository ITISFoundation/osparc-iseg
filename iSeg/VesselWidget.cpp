/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#define UNREFERENCED_PARAMETER(P) (P)

namespace iseg {

VesselWidget::VesselWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	txt_info = new QLabel("Thresholds: 1000,1150,1250,1300", vbox1);
	pb_exec = new QPushButton("Execute", vbox1);
	pb_store = new QPushButton("Save...", vbox1);
	pb_store->setEnabled(false);

	txt_start = new QLabel("Start Pt.: ", hbox1);
	cbb_lb1 = new QComboBox(hbox1);
	txt_nrend = new QLabel("Nr. of Pts.: ", hbox2);
	sb_nrend = new QSpinBox(1, 20, 1, hbox2);
	sb_nrend->setValue(1);
	txt_endnr = new QLabel("Pt. Nr.: ", hbox3);
	sb_endnr = new QSpinBox(1, sb_nrend->value(), 1, hbox3);
	sb_endnr->setValue(1);
	txt_end = new QLabel(" End Pt.: ", hbox3);
	cbb_lb2 = new QComboBox(hbox3);

	hbox1->setFixedSize(hbox1->sizeHint());
	hbox2->setFixedSize(hbox2->sizeHint());
	hbox3->setFixedSize(hbox3->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());
	//	setFixedSize(vbox1->size());

	augmentedmark am;
	am.mark = 0;
	am.name = "";
	am.p.px = 12345;
	am.p.py = 12345;
	am.slicenr = 12345;
	selectedlabels.push_back(am);
	selectedlabels.push_back(am);
	marks_changed();

	QObject::connect(sb_nrend, SIGNAL(valueChanged(int)), this,
			SLOT(nrend_changed(int)));
	QObject::connect(sb_endnr, SIGNAL(valueChanged(int)), this,
			SLOT(endnr_changed(int)));
	QObject::connect(pb_exec, SIGNAL(clicked()), this, SLOT(execute()));
	QObject::connect(pb_store, SIGNAL(clicked()), this, SLOT(savevessel()));
	QObject::connect(cbb_lb1, SIGNAL(activated(int)), this,
			SLOT(cbb1_changed(int)));
	QObject::connect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb2_changed(int)));
}

VesselWidget::~VesselWidget() { delete vbox1; }

QSize VesselWidget::sizeHint() const { return vbox1->sizeHint(); }

void VesselWidget::init()
{
	getlabels();

	branchTree.resetIterator();
	vp.clear();
	if (branchTree.getSize() > 0)
	{
		branchTree.getItem()->getCenterListSlice_inclchildren(
				handler3D->active_slice(), vp);
	}
	emit vp1_changed(&vp);
}

FILE* VesselWidget::SaveParams(FILE* fp, int version)
{
	UNREFERENCED_PARAMETER(version);
	return fp;
}

FILE* VesselWidget::LoadParams(FILE* fp, int version)
{
	UNREFERENCED_PARAMETER(version);
	return fp;
}

void VesselWidget::getlabels()
{
	handler3D->get_labels(&labels);
	QObject::disconnect(cbb_lb1, SIGNAL(activated(int)), this,
			SLOT(cbb1_changed(int)));
	QObject::disconnect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb2_changed(int)));
	cbb_lb1->clear();
	cbb_lb2->clear();

	for (size_t i = 0; i < labels.size(); i++)
	{
		cbb_lb1->insertItem(QString(labels[i].name.c_str()));
		cbb_lb2->insertItem(QString(labels[i].name.c_str()));
	}

	for (size_t i = 0; i < selectedlabels.size(); i++)
	{
		size_t j = 0;
		while ((j < labels.size()) && (labels[j] != selectedlabels[i]))
			j++;
		if (j == labels.size())
		{
			if (labels.empty())
			{
				selectedlabels[i].mark = 0;
				selectedlabels[i].name = "";
				selectedlabels[i].p.px = 12345;
				selectedlabels[i].p.py = 12345;
				selectedlabels[i].slicenr = 12345;
			}
			else
			{
				selectedlabels[i] = labels[0];
				if (i == 0)
				{
					cbb_lb1->setCurrentItem(0);
				}
				else if ((int)i == sb_endnr->value())
				{
					cbb_lb2->setCurrentItem(0);
				}
			}
		}
		else
		{
			if (i == 0)
			{
				cbb_lb1->setCurrentItem(j);
			}
			else if ((int)i == sb_endnr->value())
			{
				cbb_lb2->setCurrentItem(j);
			}
		}
	}

	QObject::connect(cbb_lb1, SIGNAL(activated(int)), this,
			SLOT(cbb1_changed(int)));
	QObject::connect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb2_changed(int)));

	if (labels.empty())
	{
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
		txt_info->setText("No labels exist.");
		pb_exec->hide();
	}
	else
	{
		hbox1->show();
		hbox2->show();
		hbox3->show();
		txt_info->setText("Thresholds: 1000,1150,1250,1300");
		pb_exec->show();
	}
}

void VesselWidget::marks_changed() { getlabels(); }

void VesselWidget::execute()
{
	World _world;
	reset_branchTree();

	// end point for dijkstra in voxel coordinates
	Vec3 end;
	end[0] = selectedlabels[0].p.px;
	end[1] = selectedlabels[0].p.py;
	end[2] = selectedlabels[0].slicenr;

	std::vector<Vec3> seeds;		// only distal seeds
	std::vector<Vec3> allSeeds; // seeds + end point

	// save all start seeds in s
	for (int i = 1; i <= sb_nrend->value(); i++)
	{
		int j = 0;
		while ((j < i) && (selectedlabels[j] != selectedlabels[i]))
			j++;
		if (j == i)
		{
			Vec3 start;
			start[0] = selectedlabels[i].p.px;
			start[1] = selectedlabels[i].p.py;
			start[2] = selectedlabels[i].slicenr;
			allSeeds.push_back(start);
			seeds.push_back(start);
		}
	}

	allSeeds.push_back(end);

	if (!seeds.empty() && (handler3D->num_slices() > 0) &&
			(handler3D->width() > 0) && (handler3D->height() > 0))
	{
		Vec3 tmpbbStart;
		//tmpbbStart[0]=0;
		//tmpbbStart[1]=0;
		//tmpbbStart[2]=0;
		tmpbbStart[0] = 60;
		tmpbbStart[1] = 130;
		tmpbbStart[2] = 60;
		Vec3 tmpbbEnd;
		//tmpbbEnd[0]=handler3D->return_width()-1;
		//tmpbbEnd[1]=handler3D->return_height()-1;
		//tmpbbEnd[2]=handler3D->return_nrslices()-1;
		tmpbbEnd[0] = 400;
		tmpbbEnd[1] = 450;
		tmpbbEnd[2] = 180;

		// TODO compute bounding box for algorithm
		// otherwise the memory consumption is too large
		_world.getSeedBoundingBox(&allSeeds, tmpbbStart, tmpbbEnd, handler3D);

		_world.setBBStart(tmpbbStart);
		_world.setBBEnd(tmpbbEnd);
		if (_world.init(tmpbbStart, tmpbbEnd, handler3D))
		{
			_world.dijkstra(seeds, end, &branchTree);
			pb_store->setEnabled(true);
			on_slicenr_changed(); // BL Why?
		}
	}

	return;
}

void VesselWidget::nrend_changed(int newval)
{
	if (newval >= (int)selectedlabels.size())
	{
		augmentedmark am;
		am.mark = 0;
		am.name = "";
		am.p.px = 12345;
		am.p.py = 12345;
		am.slicenr = 12345;
		for (int i = (int)selectedlabels.size(); i <= newval; i++)
		{
			selectedlabels.push_back(am);
		}
	}
	else
	{
		selectedlabels.resize(newval + 1);
	}
	sb_endnr->setMaxValue(newval);
	sb_endnr->setValue(1);
	//	endnr_changed(1);

	return;
}

void VesselWidget::endnr_changed(int newval)
{
	QObject::disconnect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb2_changed(int)));

	size_t i = 0;
	while ((i < labels.size()) && (labels[i] != selectedlabels[newval]))
		i++;
	cbb_lb2->setCurrentItem(i);

	QObject::connect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb2_changed(int)));
}

void VesselWidget::cbb1_changed(int newval)
{
	selectedlabels[0] = labels[newval];
}

void VesselWidget::cbb2_changed(int newval)
{
	selectedlabels[sb_endnr->value()] = labels[newval];
}

void VesselWidget::newloaded() { reset_branchTree(); }

void VesselWidget::reset_branchTree()
{
	branchTree.clear();
	vp.clear();
	emit vp1_changed(&vp);
	pb_store->setEnabled(false);
}

void VesselWidget::on_slicenr_changed()
{
	branchTree.resetIterator();
	vp.clear();
	if (branchTree.getSize() > 0)
	{
		branchTree.getItem()->getCenterListSlice_inclchildren(
				handler3D->active_slice(), vp);
	}
	emit vp1_changed(&vp);
}

void VesselWidget::savevessel()
{
	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Vessel-Tracks (*.txt)\n", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		std::vector<std::vector<Vec3>> vp;
		Pair pair1 = handler3D->get_pixelsize();
		float thick = handler3D->get_slicethickness();
		float epsilon = std::max(std::max(pair1.high, pair1.low), thick);
		branchTree.getItem()->doug_peuck_inclchildren(epsilon, pair1.high, pair1.low, thick, vp);
		FILE* fp = fopen(savefilename.ascii(), "w");
		int version = 2;
		unsigned short w = handler3D->width();
		unsigned short h = handler3D->height();
		fprintf(fp, "V%i\n", version);
		fprintf(fp, "NS%i\n", (int)handler3D->num_slices());
		fprintf(fp, "VoxelSize: %f %f %f\n", pair1.high / 2, pair1.low / 2, thick);
		fprintf(fp, "N%i\n", (int)vp.size());
		for (size_t i = 0; i < vp.size(); i++)
		{
			fprintf(fp, "PTS%i: ", (int)vp[i].size());
			for (size_t j = 0; j < vp[i].size(); j++)
			{
				fprintf(fp, "%i,%i,%i ", (int)w - 2 * (int)(vp[i][j][0]),
						2 * (int)(vp[i][j][1]) - h,
						(int)(vp[i][j][2])); //xmirrored
			}
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
}

void VesselWidget::cleanup()
{
	vp.clear();
	emit vp1_changed(&vp);
}

QIcon iseg::VesselWidget::GetIcon(QDir picdir)
{
	return QIcon(picdir.absFilePath(QString("vessel.png")));
}

} // namespace iseg