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

#include "PickerWidget.h"

#include "Data/addLine.h"

#include "Interface/LayoutTools.h"

#include "Core/Pair.h"

#include <QFormLayout>
#include <QKeyEvent>

#include <fstream>

namespace iseg {

PickerWidget::PickerWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Copy and erase regions. Copying can be used to transfer "
										"segmented regions from one slice to another. All the	"
										"functions are based on the current region selection."));

	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Width = m_Height = 0;
	m_Mask = m_Currentselection = nullptr;
	m_Valuedistrib = nullptr;

	m_Hasclipboard = false;
	m_Shiftpressed = false;

	m_RbWork = new QRadioButton(tr("Target"));
	m_RbTissue = new QRadioButton(tr("Tissue"));
	auto worktissuegroup = new QButtonGroup(this);
	worktissuegroup->insert(m_RbWork);
	worktissuegroup->insert(m_RbTissue);
	m_RbTissue->setChecked(true);

	m_RbErase = new QRadioButton(tr("Erase"));
	m_RbErase->setToolTip(Format("The deleted regions will be left empty."));
	m_RbFill = new QRadioButton(tr("Fill"));
	m_RbFill->setToolTip(Format("Fill the resulting hole based on the neighboring regions."));

	auto erasefillgroup = new QButtonGroup(this);
	erasefillgroup->insert(m_RbErase);
	erasefillgroup->insert(m_RbFill);
	m_RbErase->setChecked(true);

	m_PbCopy = new QPushButton("Copy");
	m_PbCopy->setToolTip(Format("Copies an image on the clip-board."));
	m_PbPaste = new QPushButton("Paste");
	m_PbPaste->setToolTip(Format("Clip-board is pasted into the current slice"));
	m_PbCut = new QPushButton("Cut");
	m_PbCut->setToolTip(Format("Tissues or target image pixels inside the region "
														 "selection are erased but a copy "
														 "is placed on the clipboard."));
	m_PbDelete = new QPushButton("Delete");
	m_PbDelete->setToolTip(Format("Tissues (resp. target image pixels) inside the "
																"region selection are erased."));

	// layout
	auto layout = new QFormLayout;
	layout->addRow(m_RbWork, m_RbTissue);
	layout->addRow(m_RbErase, m_RbFill);
	layout->addRow(make_hbox({m_PbCopy, m_PbPaste, m_PbCut, m_PbDelete}));
	setLayout(layout);

	// initialize
	m_Selection.clear();
	UpdateActive();
	Showborder();

	// connections
	QObject_connect(worktissuegroup, SIGNAL(buttonClicked(int)), this, SLOT(WorktissueChanged(int)));
	QObject_connect(m_PbCopy, SIGNAL(clicked()), this, SLOT(CopyPressed()));
	QObject_connect(m_PbPaste, SIGNAL(clicked()), this, SLOT(PastePressed()));
	QObject_connect(m_PbCut, SIGNAL(clicked()), this, SLOT(CutPressed()));
	QObject_connect(m_PbDelete, SIGNAL(clicked()), this, SLOT(DeletePressed()));
}

PickerWidget::~PickerWidget()
{
	delete[] m_Mask;
	delete[] m_Currentselection;
	delete[] m_Valuedistrib;
}

void PickerWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;

	if (m_Width != bmph->ReturnWidth() || m_Height != bmph->ReturnHeight())
	{
		delete[] m_Mask;
		delete[] m_Currentselection;
		delete[] m_Valuedistrib;
		m_Width = bmph->ReturnWidth();
		m_Height = bmph->ReturnHeight();
		unsigned int area = bmph->ReturnArea();
		m_Mask = new bool[area];
		m_Currentselection = new bool[area];
		m_Valuedistrib = new float[area];
		for (unsigned int i = 0; i < area; i++)
		{
			m_Mask[i] = m_Currentselection[i] = false;
		}
		m_Hasclipboard = false;
		Showborder();
	}
}

void PickerWidget::OnSlicenrChanged()
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void PickerWidget::Init()
{
	BmphandChanged(m_Handler3D->GetActivebmphandler());
	Showborder();
}

void PickerWidget::NewLoaded()
{
	BmphandChanged(m_Handler3D->GetActivebmphandler());
	Showborder();
}

FILE* PickerWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 6)
	{
		int dummy;
		dummy = (int)(m_RbWork->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbTissue->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbErase->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbFill->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* PickerWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 6)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_RbWork->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbTissue->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbErase->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbFill->setChecked(dummy > 0);

		WorktissueChanged(0);
	}
	return fp;
}

void PickerWidget::OnMouseClicked(Point p)
{
	if (!m_Shiftpressed)
	{
		unsigned int area = m_Bmphand->ReturnArea();
		for (unsigned int i = 0; i < area; i++)
		{
			m_Currentselection[i] = false;
		}
	}
	bool addorsub = !(m_Currentselection[p.px + (unsigned long)(p.py) * m_Width]);
	if (m_RbWork->isChecked())
		m_Bmphand->Change2maskConnectedwork(m_Currentselection, p, addorsub);
	else
		m_Bmphand->Change2maskConnectedtissue(m_Handler3D->ActiveTissuelayer(), m_Currentselection, p, addorsub);

	Showborder();
}

void PickerWidget::Showborder()
{
	m_Selection.clear();
	Point dummy;

	unsigned long pos = 0;
	for (unsigned int y = 0; y + 1 < m_Height; y++)
	{
		for (unsigned int x = 0; x + 1 < m_Width; x++)
		{
			if (m_Currentselection[pos] != m_Currentselection[pos + 1])
			{
				dummy.py = y;
				if (m_Currentselection[pos])
					dummy.px = x;
				else
					dummy.px = x + 1;
				m_Selection.push_back(dummy);
			}
			if (m_Currentselection[pos] != m_Currentselection[pos + m_Width])
			{
				dummy.px = x;
				if (!m_Currentselection[pos])
					dummy.py = y;
				else
					dummy.py = y + 1;
				m_Selection.push_back(dummy);
			}
			pos++;
		}
		pos++;
	}
	pos = 0;
	for (unsigned int y = 0; y < m_Height; y++)
	{
		if (m_Currentselection[pos])
		{
			dummy.px = 0;
			dummy.py = y;
			m_Selection.push_back(dummy);
		}
		pos += m_Width - 1;
		if (m_Currentselection[pos])
		{
			dummy.px = m_Width - 1;
			dummy.py = y;
			m_Selection.push_back(dummy);
		}
		pos++;
	}
	pos = m_Width * (unsigned long)(m_Height - 1);
	for (unsigned int x = 0; x < m_Width; x++)
	{
		if (m_Currentselection[x])
		{
			dummy.px = x;
			dummy.py = 0;
			m_Selection.push_back(dummy);
		}
		if (m_Currentselection[pos])
		{
			dummy.px = x;
			dummy.py = m_Height - 1;
			m_Selection.push_back(dummy);
		}
		pos++;
	}

	emit Vp1Changed(&m_Selection);
}

void PickerWidget::WorktissueChanged(int) { UpdateActive(); }

void PickerWidget::Cleanup()
{
	m_Selection.clear();
	emit Vp1Changed(&m_Selection);
}

void PickerWidget::UpdateActive()
{
	if (m_Hasclipboard && (m_RbWork->isChecked() == m_Clipboardworkortissue))
	{
		m_PbPaste->show();
	}
	else
	{
		m_PbPaste->hide();
	}
}

void PickerWidget::CopyPressed()
{
	unsigned int area = m_Bmphand->ReturnArea();
	for (unsigned int i = 0; i < area; i++)
	{
		m_Mask[i] = m_Currentselection[i];
	}
	m_Clipboardworkortissue = m_RbWork->isChecked();
	if (m_Clipboardworkortissue)
	{
		m_Bmphand->CopyWork(m_Valuedistrib);
		m_Mode = m_Bmphand->ReturnMode(false);
	}
	else
	{
		auto tissues = m_Bmphand->ReturnTissues(m_Handler3D->ActiveTissuelayer());
		for (unsigned int i = 0; i < area; i++)
		{
			m_Valuedistrib[i] = (float)tissues[i];
		}
	}
	m_Hasclipboard = true;
	UpdateActive();
}

void PickerWidget::CutPressed()
{
	CopyPressed();
	DeletePressed();
}

void PickerWidget::PastePressed()
{
	if (m_Clipboardworkortissue != m_RbWork->isChecked())
		return;
	unsigned int area = m_Bmphand->ReturnArea();

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = m_Clipboardworkortissue;
	data_selection.tissues = !m_Clipboardworkortissue;
	emit BeginDatachange(data_selection, this);

	if (m_Clipboardworkortissue)
	{
		m_Bmphand->Copy2work(m_Valuedistrib, m_Mask, m_Mode);
	}
	else
	{
		tissues_size_t* valuedistrib2 = new tissues_size_t[area];
		for (unsigned int i = 0; i < area; i++)
		{
			valuedistrib2[i] = (tissues_size_t)m_Valuedistrib[i];
		}
		m_Bmphand->Copy2tissue(m_Handler3D->ActiveTissuelayer(), valuedistrib2, m_Mask);
		delete[] valuedistrib2;
	}

	emit EndDatachange(this);
}

void PickerWidget::DeletePressed()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = m_RbWork->isChecked();
	data_selection.tissues = !m_RbWork->isChecked();
	emit BeginDatachange(data_selection, this);

	if (m_RbWork->isChecked())
	{
		if (m_RbErase->isChecked())
			m_Bmphand->Erasework(m_Currentselection);
		else
			m_Bmphand->Floodwork(m_Currentselection);
	}
	else
	{
		if (m_RbErase->isChecked())
			m_Bmphand->Erasetissue(m_Handler3D->ActiveTissuelayer(), m_Currentselection);
		else
			m_Bmphand->Floodtissue(m_Handler3D->ActiveTissuelayer(), m_Currentselection);
	}

	emit EndDatachange(this);
}

void PickerWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Shift)
	{
		m_Shiftpressed = true;
	}
}

void PickerWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Shift)
	{
		m_Shiftpressed = false;
	}
}

} // namespace iseg
