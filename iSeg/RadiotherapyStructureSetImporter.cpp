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

#include "RadiotherapyStructureSetImporter.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"

#include "Core/fillcontour.h"

#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qstring.h>
#include <qstringlist.h>

namespace iseg {

RadiotherapyStructureSetImporter::RadiotherapyStructureSetImporter(QString loadfilename, SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	vbox1 = nullptr;

	if (loadfilename.isEmpty())
	{
		close();
		return;
	}

	tissues.clear();
	gdcmvtk_rtstruct::RequestData_RTStructureSetStorage(loadfilename.ascii(), tissues);

	vecignore.resize(tissues.size());
	vecpriorities.resize(tissues.size());
	vectissuenames.resize(tissues.size());
	vectissuenrs.resize(tissues.size());
	vecnew.resize(tissues.size());

	tissues_size_t tissuenr;
	std::string namedummy = std::string("");
	tissues_size_t nrnew = 1;
	for (size_t i = 0; i < tissues.size(); i++)
	{
		vecignore[i] = false;
		vecpriorities[i] = i + 1;
		if (tissues[i]->name != namedummy)
		{
			vectissuenames[i] = tissues[i]->name;
		}
		else
		{
			QString sdummy;
			sdummy.setNum(nrnew);
			vectissuenames[i] = std::string("tissue") + sdummy.toAscii().data();
			nrnew++;
		}

		for (tissuenr = 0;
				 tissuenr < TissueInfos::GetTissueCount() &&
				 tissues[i]->name != TissueInfos::GetTissueName(tissuenr + 1);
				 tissuenr++)
		{}
		if (tissuenr == (tissues_size_t)TissueInfos::GetTissueCount())
		{
			vecnew[i] = true;
			vectissuenrs[i] = 0;
		}
		else
		{
			vecnew[i] = false;
			vectissuenrs[i] = tissuenr;
		}
	}

	vbox1 = new Q3VBox(this);
	cb_solids = new QComboBox(vbox1);
	for (size_t i = 0; i < tissues.size(); i++)
	{
		cb_solids->insertItem(QString(tissues[i]->name.c_str()));
	}

	currentitem = cb_solids->currentItem();

	cb_ignore = new QCheckBox("Ignore", vbox1);
	cb_ignore->setChecked(vecignore[cb_solids->currentItem()]);

	hbox1 = new Q3HBox(vbox1);
	lb_priority = new QLabel(QString("Priority: "), hbox1);
	sb_priority = new QSpinBox(1, tissues.size(), 1, hbox1);
	sb_priority->setValue(vecpriorities[cb_solids->currentItem()]);
	QPushButton* infoButton = new QPushButton(hbox1);
	infoButton->setText("Info");

	cb_new = new QCheckBox("New Tissue", vbox1);
	cb_new->setChecked(vecnew[cb_solids->currentItem()]);

	hbox2 = new Q3HBox(vbox1);
	lb_namele = new QLabel(QString("Name: "), hbox2);
	le_name =
			new QLineEdit(vectissuenames[cb_solids->currentItem()].c_str(), hbox2);

	hbox3 = new Q3HBox(vbox1);
	lb_namecb = new QLabel(QString("Name: "), hbox3);
	cb_names = new QComboBox(hbox3);
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); i++)
	{
		cb_names->insertItem(ToQ(TissueInfos::GetTissueName(i)));
	}
	cb_names->setCurrentItem(vectissuenrs[cb_solids->currentItem()]);

	hbox4 = new Q3HBox(vbox1);
	pb_ok = new QPushButton("OK", hbox4);
	pb_cancel = new QPushButton("Cancel", hbox4);

	vbox1->setFixedSize(vbox1->sizeHint());
	setFixedSize(vbox1->size());

	updatevisibility();

	connect(cb_solids, SIGNAL(activated(int)), this, SLOT(solid_changed(int)));
	connect(pb_cancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(cb_new, SIGNAL(clicked()), this, SLOT(new_changed()));
	connect(cb_ignore, SIGNAL(clicked()), this, SLOT(ignore_changed()));
	connect(pb_ok, SIGNAL(clicked()), this, SLOT(ok_pressed()));
	connect(infoButton, SIGNAL(clicked()), this, SLOT(show_priorityInfo()));
}

RadiotherapyStructureSetImporter::~RadiotherapyStructureSetImporter() {}

void RadiotherapyStructureSetImporter::solid_changed(int i)
{
	storeparams();
	disconnect(cb_new, SIGNAL(clicked()), this, SLOT(new_changed()));
	disconnect(cb_ignore, SIGNAL(clicked()), this, SLOT(ignore_changed()));
	currentitem = i;
	cb_ignore->setChecked(vecignore[i]);
	sb_priority->setValue(vecpriorities[i]);
	cb_new->setChecked(vecnew[i]);
	le_name->setText(vectissuenames[i].c_str());
	cb_names->setCurrentItem(vectissuenrs[i]);
	connect(cb_new, SIGNAL(clicked()), this, SLOT(new_changed()));
	connect(cb_ignore, SIGNAL(clicked()), this, SLOT(ignore_changed()));
	updatevisibility();
}

void RadiotherapyStructureSetImporter::new_changed()
{
	storeparams();
	updatevisibility();
}

void RadiotherapyStructureSetImporter::ok_pressed()
{
	storeparams();

	bool* mask = new bool[handler3D->return_area()];
	if (mask == 0)
	{
		QMessageBox::about(this, "Warning", "Not enough memory");
		return;
	}

	tissues_size_t nrnew = 0;
	for (size_t i = 0; i < tissues.size(); i++)
	{
		if (vecignore[i] == false && !vectissuenames[i].empty() && vecnew[i] == true)
			nrnew++;
	}
	if (nrnew >= TISSUES_SIZE_MAX - 1 - TissueInfos::GetTissueCount())
	{
		QMessageBox::about(this, "Warning", "Max number of solids exceeded");
		return;
	}

	Pair p;
	p = handler3D->get_pixelsize();
	float thick = handler3D->get_slicethickness();
	float disp[3];
	handler3D->get_displacement(disp);
	float dc[6];
	handler3D->get_direction_cosines(dc);
	unsigned short pixel_extents[2] = {handler3D->width(), handler3D->height()};
	float pixel_size[2] = {p.high, p.low};

	ISEG_INFO("RTStruct import: thickness = " << thick);

	if (std::abs(dc[0]) != 1.0f || std::abs(dc[4]) != 1.0f)
	{
		QMessageBox::warning(this, "Warning", "Arbitrary rotations of image orientation (patient) not supported");
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	bool error = false;
	tissues_size_t tissuenr;
	std::string namedummy = std::string("");
	for (int i = 1; i <= tissues.size(); i++) // i is priority
	{
		if (error)
			break;

		gdcmvtk_rtstruct::tissuevec::iterator it = tissues.begin();
		int j = 0;
		while (vecpriorities[j] != i) // j is tissue index, processed in order of priority
			j++, it++;

		if (vecignore[j] == false && ((!vecnew[j]) || (!vectissuenames[j].empty())))
		{
			const auto& rtstruct_i = *it;

			if (vecnew[j])
			{
				TissueInfo tissueInfo;
				tissueInfo.name = vectissuenames[j].c_str();
				tissueInfo.locked = false;
				tissueInfo.opac = 0.5f;
				tissueInfo.color[0] = rtstruct_i->color[0];
				tissueInfo.color[1] = rtstruct_i->color[1];
				tissueInfo.color[2] = rtstruct_i->color[2];
				TissueInfos::AddTissue(tissueInfo);
				tissuenr = TissueInfos::GetTissueCount();
			}
			else
			{
				tissuenr = vectissuenrs[j] + 1;
			}

			size_t pospoints = 0;
			size_t posoutlines = 0;
			std::vector<float*> points;
			bool clockwisefill = false;
			while (posoutlines < rtstruct_i->outlinelength.size())
			{
				points.clear();
				unsigned int* nrpoints = &(rtstruct_i->outlinelength[posoutlines]);
				float zcoord = rtstruct_i->points[pospoints + 2];
				points.push_back(&(rtstruct_i->points[pospoints]));
				pospoints += rtstruct_i->outlinelength[posoutlines] * 3;
				posoutlines++;
				while (posoutlines < rtstruct_i->outlinelength.size() && zcoord == rtstruct_i->points[pospoints + 2])
				{
					points.push_back(&(rtstruct_i->points[pospoints]));
					pospoints += rtstruct_i->outlinelength[posoutlines] * 3;
					posoutlines++;
				}

				const int startSL = handler3D->start_slice();
				const int endSL = handler3D->end_slice();

				float swap_z = dc[0] * dc[4];
				int slicenr = ceil(swap_z * (disp[2] - zcoord) / thick);

				if (slicenr < 0 && swap_z > 0.f)
				{
					ISEG_WARNING("RTStruct import: strange slice value " << slicenr);
					slicenr = endSL + slicenr; // TODO this is strange!
				}

				if (slicenr >= startSL && slicenr < endSL)
				{
					try
					{
						fillcontours::fill_contour(mask, pixel_extents, disp,
								pixel_size, dc, &(points[0]),
								nrpoints, points.size(), clockwisefill);
					}
					catch (std::exception& e)
					{
						QMessageBox::warning(this, "An Exception Occurred", e.what());
						error = true;
						break;
					}
					handler3D->add2tissue(tissuenr, mask, static_cast<unsigned short>(slicenr), true);
				}
				else
				{
					ISEG_WARNING("RTStruct import: invalid slice " << slicenr);
				}
			}
		}
	}

	emit end_datachange(this);

	delete[] mask;

	close();
}

void RadiotherapyStructureSetImporter::show_priorityInfo()
{
	QMessageBox::information(
			this, "Priority Information",
			"1) Tissues have been sorted so that higher priority is given to the "
			"smallest tissues.<br>"
			"2) The higher number of priority, higher priority will have.");
}

void RadiotherapyStructureSetImporter::ignore_changed()
{
	storeparams();
	updatevisibility();
}

void RadiotherapyStructureSetImporter::updatevisibility()
{
	if (vecignore[cb_solids->currentItem()])
	{
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
		cb_new->hide();
	}
	else
	{
		hbox1->show();
		cb_new->show();
		if (vecnew[cb_solids->currentItem()])
		{
			hbox2->show();
			hbox3->hide();
		}
		else
		{
			hbox2->hide();
			hbox3->show();
		}
	}
}

void RadiotherapyStructureSetImporter::storeparams()
{
	int dummy = vecpriorities[currentitem];

	vecnew[currentitem] = cb_new->isChecked();
	vecignore[currentitem] = cb_ignore->isChecked();
	vectissuenrs[currentitem] = cb_names->currentItem();
	vectissuenames[currentitem] = le_name->text().toAscii().data();

	if (dummy != sb_priority->value())
	{
		vecpriorities[currentitem] = sb_priority->value();
		int pos = 0;
		if (currentitem == 0)
			pos = 1;
		while (vecpriorities[pos] != vecpriorities[currentitem])
		{
			pos++;
			if (pos == currentitem)
				pos++;
		}
		vecpriorities[pos] = dummy;
	}
}

}// namespace iseg