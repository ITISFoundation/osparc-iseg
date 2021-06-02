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

#include "RadiotherapyStructureSetImporter.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"

#include "Interface/QtConnect.h"

#include "Core/fillcontour.h"

#include <QMessageBox>
#include <QString>
#include <QStringList>

namespace iseg {

RadiotherapyStructureSetImporter::RadiotherapyStructureSetImporter(QString loadfilename, SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Vbox1 = nullptr;

	if (loadfilename.isEmpty())
	{
		close();
		return;
	}

	m_Tissues.clear();
	gdcmvtk_rtstruct::RequestData_RTStructureSetStorage(loadfilename.ascii(), m_Tissues);

	m_Vecignore.resize(m_Tissues.size());
	m_Vecpriorities.resize(m_Tissues.size());
	m_Vectissuenames.resize(m_Tissues.size());
	m_Vectissuenrs.resize(m_Tissues.size());
	m_Vecnew.resize(m_Tissues.size());

	tissues_size_t tissuenr;
	std::string namedummy = std::string("");
	tissues_size_t nrnew = 1;
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		m_Vecignore[i] = false;
		m_Vecpriorities[i] = i + 1;
		if (m_Tissues[i]->name != namedummy)
		{
			m_Vectissuenames[i] = m_Tissues[i]->name;
		}
		else
		{
			QString sdummy;
			sdummy.setNum(nrnew);
			m_Vectissuenames[i] = std::string("tissue") + sdummy.toStdString();
			nrnew++;
		}

		for (tissuenr = 0;
				 tissuenr < TissueInfos::GetTissueCount() &&
				 m_Tissues[i]->name != TissueInfos::GetTissueName(tissuenr + 1);
				 tissuenr++)
		{}
		if (tissuenr == (tissues_size_t)TissueInfos::GetTissueCount())
		{
			m_Vecnew[i] = true;
			m_Vectissuenrs[i] = 0;
		}
		else
		{
			m_Vecnew[i] = false;
			m_Vectissuenrs[i] = tissuenr;
		}
	}

	m_Vbox1 = new Q3VBox(this);
	m_CbSolids = new QComboBox(m_Vbox1);
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		m_CbSolids->insertItem(QString(m_Tissues[i]->name.c_str()));
	}

	m_Currentitem = m_CbSolids->currentItem();

	m_CbIgnore = new QCheckBox("Ignore", m_Vbox1);
	m_CbIgnore->setChecked(m_Vecignore[m_CbSolids->currentItem()]);

	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_LbPriority = new QLabel(QString("Priority: "), m_Hbox1);
	m_SbPriority = new QSpinBox(1, m_Tissues.size(), 1, m_Hbox1);
	m_SbPriority->setValue(m_Vecpriorities[m_CbSolids->currentItem()]);
	QPushButton* info_button = new QPushButton(m_Hbox1);
	info_button->setText("Info");

	m_CbNew = new QCheckBox("New Tissue", m_Vbox1);
	m_CbNew->setChecked(m_Vecnew[m_CbSolids->currentItem()]);

	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_LbNamele = new QLabel(QString("Name: "), m_Hbox2);
	m_LeName =
			new QLineEdit(m_Vectissuenames[m_CbSolids->currentItem()].c_str(), m_Hbox2);

	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_LbNamecb = new QLabel(QString("Name: "), m_Hbox3);
	m_CbNames = new QComboBox(m_Hbox3);
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); i++)
	{
		m_CbNames->insertItem(ToQ(TissueInfos::GetTissueName(i)));
	}
	m_CbNames->setCurrentIndex(m_Vectissuenrs[m_CbSolids->currentItem()]);

	m_Hbox4 = new Q3HBox(m_Vbox1);
	m_PbOk = new QPushButton("OK", m_Hbox4);
	m_PbCancel = new QPushButton("Cancel", m_Hbox4);

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	setFixedSize(m_Vbox1->size());

	Updatevisibility();

	QObject_connect(m_CbSolids, SIGNAL(activated(int)), this, SLOT(SolidChanged(int)));
	QObject_connect(m_PbCancel, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_CbNew, SIGNAL(clicked()), this, SLOT(NewChanged()));
	QObject_connect(m_CbIgnore, SIGNAL(clicked()), this, SLOT(IgnoreChanged()));
	QObject_connect(m_PbOk, SIGNAL(clicked()), this, SLOT(OkPressed()));
	QObject_connect(info_button, SIGNAL(clicked()), this, SLOT(ShowPriorityInfo()));
}

RadiotherapyStructureSetImporter::~RadiotherapyStructureSetImporter() = default;

void RadiotherapyStructureSetImporter::SolidChanged(int i)
{
	Storeparams();
	QObject_disconnect(m_CbNew, SIGNAL(clicked()), this, SLOT(NewChanged()));
	QObject_disconnect(m_CbIgnore, SIGNAL(clicked()), this, SLOT(IgnoreChanged()));
	m_Currentitem = i;
	m_CbIgnore->setChecked(m_Vecignore[i]);
	m_SbPriority->setValue(m_Vecpriorities[i]);
	m_CbNew->setChecked(m_Vecnew[i]);
	m_LeName->setText(m_Vectissuenames[i].c_str());
	m_CbNames->setCurrentIndex(m_Vectissuenrs[i]);
	QObject_connect(m_CbNew, SIGNAL(clicked()), this, SLOT(NewChanged()));
	QObject_connect(m_CbIgnore, SIGNAL(clicked()), this, SLOT(IgnoreChanged()));
	Updatevisibility();
}

void RadiotherapyStructureSetImporter::NewChanged()
{
	Storeparams();
	Updatevisibility();
}

void RadiotherapyStructureSetImporter::OkPressed()
{
	Storeparams();

	bool* mask = new bool[m_Handler3D->ReturnArea()];
	if (mask == nullptr)
	{
		QMessageBox::about(this, "Warning", "Not enough memory");
		return;
	}

	tissues_size_t nrnew = 0;
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		if (m_Vecignore[i] == false && !m_Vectissuenames[i].empty() && m_Vecnew[i] == true)
			nrnew++;
	}
	if (nrnew >= TISSUES_SIZE_MAX - 1 - TissueInfos::GetTissueCount())
	{
		QMessageBox::about(this, "Warning", "Max number of solids exceeded");
		return;
	}

	Pair p;
	p = m_Handler3D->GetPixelsize();
	float thick = m_Handler3D->GetSlicethickness();
	float disp[3];
	m_Handler3D->GetDisplacement(disp);
	float dc[6];
	m_Handler3D->GetDirectionCosines(dc);
	unsigned short pixel_extents[2] = {m_Handler3D->Width(), m_Handler3D->Height()};
	float pixel_size[2] = {p.high, p.low};

	ISEG_INFO("RTStruct import: thickness = " << thick);

	if (std::abs(dc[0]) != 1.0f || std::abs(dc[4]) != 1.0f)
	{
		QMessageBox::warning(this, "Warning", "Arbitrary rotations of image orientation (patient) not supported");
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	bool error = false;
	tissues_size_t tissuenr;
	std::string namedummy;
	for (int i = 1; i <= m_Tissues.size(); i++) // i is priority
	{
		if (error)
			break;

		gdcmvtk_rtstruct::tissuevec::iterator it = m_Tissues.begin();
		int j = 0;
		while (m_Vecpriorities[j] != i) // j is tissue index, processed in order of priority
			j++, it++;

		if (m_Vecignore[j] == false && ((!m_Vecnew[j]) || (!m_Vectissuenames[j].empty())))
		{
			const auto& rtstruct_i = *it;

			if (m_Vecnew[j])
			{
				TissueInfo tissue_info;
				tissue_info.m_Name = m_Vectissuenames[j];
				tissue_info.m_Locked = false;
				tissue_info.m_Opac = 0.5f;
				tissue_info.m_Color[0] = rtstruct_i->color[0];
				tissue_info.m_Color[1] = rtstruct_i->color[1];
				tissue_info.m_Color[2] = rtstruct_i->color[2];
				TissueInfos::AddTissue(tissue_info);
				tissuenr = TissueInfos::GetTissueCount();
			}
			else
			{
				tissuenr = m_Vectissuenrs[j] + 1;
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

				const int start_sl = m_Handler3D->StartSlice();
				const int end_sl = m_Handler3D->EndSlice();

				float swap_z = dc[0] * dc[4];
				int slicenr = round(swap_z * (disp[2] - zcoord) / thick);

				if (slicenr < 0 && swap_z > 0.f)
				{
					ISEG_WARNING("RTStruct import: strange slice value " << slicenr);
					slicenr = end_sl + slicenr; // TODO this is strange!
				}

				ISEG_INFO("RTStruct import: importing slice " << slicenr)

				if (slicenr >= start_sl && slicenr < end_sl)
				{
					try
					{
						fillcontours::fill_contour(mask, pixel_extents, disp, pixel_size, dc, &(points[0]), nrpoints, points.size(), clockwisefill);
					}
					catch (std::exception& e)
					{
						QMessageBox::warning(this, "An Exception Occurred", e.what());
						error = true;
						break;
					}
					m_Handler3D->Add2tissue(tissuenr, mask, static_cast<unsigned short>(slicenr), true);
				}
				else
				{
					ISEG_WARNING("RTStruct import: invalid slice " << slicenr);
				}
			}
		}
	}

	emit EndDatachange(this);

	delete[] mask;

	close();
}

void RadiotherapyStructureSetImporter::ShowPriorityInfo()
{
	QMessageBox::information(this, "Priority Information", "1) Tissues have been sorted so that higher priority is given to the "
																												 "smallest tissues.<br>"
																												 "2) The higher number of priority, higher priority will have.");
}

void RadiotherapyStructureSetImporter::IgnoreChanged()
{
	Storeparams();
	Updatevisibility();
}

void RadiotherapyStructureSetImporter::Updatevisibility()
{
	if (m_Vecignore[m_CbSolids->currentItem()])
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_CbNew->hide();
	}
	else
	{
		m_Hbox1->show();
		m_CbNew->show();
		if (m_Vecnew[m_CbSolids->currentItem()])
		{
			m_Hbox2->show();
			m_Hbox3->hide();
		}
		else
		{
			m_Hbox2->hide();
			m_Hbox3->show();
		}
	}
}

void RadiotherapyStructureSetImporter::Storeparams()
{
	int dummy = m_Vecpriorities[m_Currentitem];

	m_Vecnew[m_Currentitem] = m_CbNew->isChecked();
	m_Vecignore[m_Currentitem] = m_CbIgnore->isChecked();
	m_Vectissuenrs[m_Currentitem] = m_CbNames->currentItem();
	m_Vectissuenames[m_Currentitem] = m_LeName->text().toAscii().data();

	if (dummy != m_SbPriority->value())
	{
		m_Vecpriorities[m_Currentitem] = m_SbPriority->value();
		int pos = 0;
		if (m_Currentitem == 0)
			pos = 1;
		while (m_Vecpriorities[pos] != m_Vecpriorities[m_Currentitem])
		{
			pos++;
			if (pos == m_Currentitem)
				pos++;
		}
		m_Vecpriorities[pos] = dummy;
	}
}

} // namespace iseg
