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

#include "MeasurementWidget.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Data/Point.h"
#include "Data/addLine.h"

#include "Interface/LayoutTools.h"

#include "Core/Pair.h"

#include <QFormLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <fstream>
#include <initializer_list>

namespace iseg {

MeasurementWidget::MeasurementWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Perform different types of distance, angle or volume measurements"));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_State = 0;

	// properties
	m_TxtDisplayer = new QLabel(" ");
	//txt_displayer->setWordWrap(true);

	m_RbVector = new QRadioButton("Vector");
	m_RbDist = new QRadioButton("Distance");
	m_RbThick = new QRadioButton("Thickness");
	m_RbAngle = new QRadioButton("Angle");
	m_Rb4ptangle = new QRadioButton("4pt-Angle");
	m_RbVol = new QRadioButton("Volume");
	auto modegroup = new QButtonGroup(this);
	modegroup->insert(m_RbVector);
	modegroup->insert(m_RbDist);
	modegroup->insert(m_RbThick);
	modegroup->insert(m_RbAngle);
	modegroup->insert(m_Rb4ptangle);
	modegroup->insert(m_RbVol);
	m_RbDist->setChecked(true);

	m_RbPts = new QRadioButton("Clicks");
	m_RbLbls = new QRadioButton("Labels");
	auto inputgroup = new QButtonGroup(this);
	inputgroup->insert(m_RbPts);
	inputgroup->insert(m_RbLbls);
	m_RbPts->setChecked(true);

	m_CbbLb1 = new QComboBox;
	m_CbbLb2 = new QComboBox;
	m_CbbLb3 = new QComboBox;
	m_CbbLb4 = new QComboBox;

	// layout
	auto method_layout = make_vbox({m_RbVector, m_RbDist, m_RbThick, m_RbAngle, m_Rb4ptangle, m_RbVol});
	method_layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
	auto method_area = new QFrame;
	method_area->setLayout(method_layout);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	m_InputArea = new QWidget;
	m_InputArea->setLayout(make_hbox({m_RbPts, m_RbLbls}));
	m_InputArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	auto labels_layout = new QFormLayout;
	labels_layout->addRow("Point 1", m_CbbLb1);
	labels_layout->addRow("Point 2", m_CbbLb2);
	labels_layout->addRow("Point 3", m_CbbLb3);
	labels_layout->addRow("Point 4", m_CbbLb4);

	m_LabelsArea = new QWidget;
	m_LabelsArea->setLayout(labels_layout);

	auto params_layout = new QVBoxLayout;
	params_layout->addWidget(m_InputArea);
	params_layout->addWidget(m_TxtDisplayer);
	params_layout->setAlignment(m_TxtDisplayer, Qt::AlignHCenter | Qt::AlignCenter);

	m_StackedWidget = new QStackedWidget;
	m_StackedWidget->addWidget(new QLabel(" "));
	m_StackedWidget->addWidget(m_LabelsArea);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addLayout(params_layout);
	top_layout->addWidget(m_StackedWidget);

	setLayout(top_layout);

	m_Drawing = false;
	m_Established.clear();
	m_Dynamic.clear();

	emit Vp1dynChanged(&m_Established, &m_Dynamic);

	// connections
	QObject_connect(modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
	QObject_connect(inputgroup, SIGNAL(buttonClicked(int)), this, SLOT(InputtypeChanged(int)));
	QObject_connect(m_CbbLb1, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_connect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_connect(m_CbbLb3, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_connect(m_CbbLb4, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));

	// initialize
	MarksChanged();
}

void MeasurementWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;

	Getlabels();
}

void MeasurementWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		BmphandChanged(m_Handler3D->GetActivebmphandler());
	}
	else
	{
		Getlabels();
	}
}

void MeasurementWidget::SetActiveLabels(eActiveLabels act)
{
	bool b1 = false, b2 = false, b3 = false, b4 = false;
	switch (act)
	{
	case kP4:
		b4 = true;
	case kP3:
		b3 = true;
	case kP2:
		b2 = true;
	case kP1:
		b1 = true;
	}
	m_CbbLb1->setEnabled(b1);
	m_CbbLb2->setEnabled(b2);
	m_CbbLb3->setEnabled(b3);
	m_CbbLb4->setEnabled(b4);
}

void MeasurementWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

FILE* MeasurementWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 4)
	{
		int dummy;
		dummy = (int)(m_RbVector->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbDist->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbAngle->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Rb4ptangle->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbVol->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbPts->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbLbls->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* MeasurementWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 3)
	{
		int dummy;
		if (version >= 4)
		{
			fread(&dummy, sizeof(int), 1, fp);
			m_RbVector->setChecked(dummy > 0);
		}
		fread(&dummy, sizeof(int), 1, fp);
		m_RbDist->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbAngle->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Rb4ptangle->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbVol->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbPts->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbLbls->setChecked(dummy > 0);

		MethodChanged(0);
		InputtypeChanged(0);
	}
	return fp;
}

void MeasurementWidget::OnMouseClicked(Point p)
{
	if (m_RbPts->isChecked())
	{
		if (m_RbVector->isChecked())
		{
			if (m_State == 0)
			{
				m_P1 = p;
				m_Drawing = true;
				m_TxtDisplayer->setText("Mark end point.");
				SetCoord(0, p, m_Handler3D->ActiveSlice());
				m_State = 1;
				m_Dynamic.clear();
				m_Established.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 1)
			{
				m_Drawing = false;
				SetCoord(1, p, m_Handler3D->ActiveSlice());
				m_State = 0;
				addLine(&m_Established, m_P1, p);
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
				m_TxtDisplayer->setText(QString("(") + QString::number(Calculatevec(0), 'g', 3) +
																QString(",") + QString::number(Calculatevec(1), 'g', 3) +
																QString(",") + QString::number(Calculatevec(2), 'g', 3) +
																QString(") mm (Mark new start point.)"));
			}
		}
		else if (m_RbDist->isChecked())
		{
			if (m_State == 0)
			{
				m_P1 = p;
				m_Drawing = true;
				m_TxtDisplayer->setText("Mark end point.");
				SetCoord(0, p, m_Handler3D->ActiveSlice());
				m_State = 1;
				m_Dynamic.clear();
				m_Established.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic, true);
			}
			else if (m_State == 1)
			{
				m_Drawing = false;
				SetCoord(1, p, m_Handler3D->ActiveSlice());
				m_State = 0;
				addLine(&m_Established, m_P1, p);
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic, true);
				m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) +
																QString(" mm (Mark new start point.)"));
			}
		}
		else if (m_RbThick->isChecked())
		{
			if (m_State == 0)
			{
				m_P1 = p;
				m_Drawing = true;
				m_TxtDisplayer->setText("Mark end point.");
				SetCoord(0, p, m_Handler3D->ActiveSlice());
				m_State = 1;
				m_Dynamic.clear();
				m_Established.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 1)
			{
				m_Drawing = false;
				SetCoord(1, p, m_Handler3D->ActiveSlice());
				m_State = 0;
				addLine(&m_Established, m_P1, p);
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
				m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) +
																QString(" mm Mark new start point.)"));
			}
		}
		else if (m_RbAngle->isChecked())
		{
			if (m_State == 0)
			{
				m_TxtDisplayer->setText("Mark second point.");
				SetCoord(0, p, m_Handler3D->ActiveSlice());
				m_State = 1;
				m_Drawing = true;
				m_P1 = p;
				m_Established.clear();
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 1)
			{
				m_TxtDisplayer->setText("Mark third point.");
				SetCoord(1, p, m_Handler3D->ActiveSlice());
				m_State = 2;
				addLine(&m_Established, m_P1, p);
				m_P1 = p;
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 2)
			{
				m_Drawing = false;
				SetCoord(2, p, m_Handler3D->ActiveSlice());
				m_State = 0;
				addLine(&m_Established, m_P1, p);
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
				m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) +
																QString(" deg (Mark new first point.)"));
			}
		}
		else if (m_Rb4ptangle->isChecked())
		{
			if (m_State == 0)
			{
				m_TxtDisplayer->setText("Mark second point.");
				SetCoord(0, p, m_Handler3D->ActiveSlice());
				m_State = 1;
				m_Drawing = true;
				m_P1 = p;
				m_Established.clear();
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 1)
			{
				m_TxtDisplayer->setText("Mark third point.");
				SetCoord(1, p, m_Handler3D->ActiveSlice());
				m_State = 2;
				addLine(&m_Established, m_P1, p);
				m_Drawing = true;
				m_P1 = p;
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 2)
			{
				m_TxtDisplayer->setText("Mark third point.");
				SetCoord(2, p, m_Handler3D->ActiveSlice());
				m_State = 3;
				addLine(&m_Established, m_P1, p);
				m_Drawing = true;
				m_P1 = p;
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_State == 3)
			{
				SetCoord(3, p, m_Handler3D->ActiveSlice());
				m_State = 0;
				m_Drawing = false;
				addLine(&m_Established, m_P1, p);
				m_Dynamic.clear();
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
				m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) +
																QString(" deg (Mark new first point.)"));
			}
		}
		else if (m_RbVol->isChecked())
		{
			m_State = 0;
			m_Established.clear();
			m_Dynamic.clear();
			emit Vp1dynChanged(&m_Established, &m_Dynamic);
			QString tissuename1;
			tissues_size_t tnr =
					m_Handler3D->GetTissuePt(p, m_Handler3D->ActiveSlice());
			if (tnr == 0)
				tissuename1 = QString("-");
			else
				tissuename1 = ToQ(TissueInfos::GetTissueName(tnr));
			QString note = QString("");
			if (!(m_Handler3D->StartSlice() == 0 && m_Handler3D->EndSlice() == m_Handler3D->NumSlices()))
			{
				note = QString("\nCalculated for active slices only.");
			}
			m_TxtDisplayer->setText(QString("Tissue '") + tissuename1 + QString("': ") +
															QString::number(m_Handler3D->CalculateTissuevolume(p, m_Handler3D->ActiveSlice()), 'g', 3) +
															QString(" mm^3\n") + QString("'Target': ") +
															QString::number(m_Handler3D->CalculateVolume(p, m_Handler3D->ActiveSlice()), 'g', 3) +
															QString(" mm^3") + note + QString("\n(Select new object.)"));
		}
	}
	else if (m_RbLbls->isChecked())
	{
		if (m_RbVol->isChecked())
		{
			m_State = 0;
			m_Established.clear();
			m_Dynamic.clear();
			emit Vp1dynChanged(&m_Established, &m_Dynamic);
			QString tissuename1;
			tissues_size_t tnr = m_Handler3D->GetTissuePt(p, m_Handler3D->ActiveSlice());
			if (tnr == 0)
				tissuename1 = QString("-");
			else
				tissuename1 = ToQ(TissueInfos::GetTissueName(tnr));
			QString note = QString("");
			if (!(m_Handler3D->StartSlice() == 0 && m_Handler3D->EndSlice() == m_Handler3D->NumSlices()))
			{
				note = QString("\nCalculated for active slices only.");
			}
			m_TxtDisplayer->setText(QString("Tissue '") + tissuename1 + QString("': ") +
															QString::number(m_Handler3D->CalculateTissuevolume(p, m_Handler3D->ActiveSlice()), 'g', 3) +
															QString(" mm^3\n") + QString("'Target': ") +
															QString::number(m_Handler3D->CalculateVolume(p, m_Handler3D->ActiveSlice()), 'g', 3) +
															QString(" mm^3") + note + QString("\n(Select new object.)"));
		}
	}
}

void MeasurementWidget::OnMouseMoved(Point p)
{
	if (m_Drawing)
	{
		m_Dynamic.clear();
		addLine(&m_Dynamic, m_P1, p);
		emit VpdynChanged(&m_Dynamic);
	}
}

void MeasurementWidget::SetCoord(unsigned short Posit, Point p, unsigned short slicenr)
{
	m_Pt[Posit][0] = (int)p.px;
	m_Pt[Posit][1] = (int)p.py;
	m_Pt[Posit][2] = (int)slicenr;
}

void MeasurementWidget::CbbChanged(int)
{
	if (m_RbLbls->isChecked() && !m_RbVol->isChecked())
	{
		m_State = 0;
		m_Drawing = false;
		m_Dynamic.clear();
		m_Established.clear();
		if (m_RbDist->isChecked())
		{
			Point pc;
			pc.px = (short)m_Handler3D->Width() / 2;
			pc.py = (short)m_Handler3D->Height() / 2;
			Point p1, p2;
			if (m_CbbLb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = m_Labels[m_CbbLb1->currentItem() - 1].p;
			if (m_CbbLb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = m_Labels[m_CbbLb2->currentItem() - 1].p;
			addLine(&m_Established, p1, p2);
			m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) + QString(" mm"));
		}
		else if (m_RbThick->isChecked())
		{
			Point pc;
			pc.px = (short)m_Handler3D->Width() / 2;
			pc.py = (short)m_Handler3D->Height() / 2;
			Point p1, p2;
			if (m_CbbLb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = m_Labels[m_CbbLb1->currentItem() - 1].p;
			if (m_CbbLb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = m_Labels[m_CbbLb2->currentItem() - 1].p;
			addLine(&m_Established, p1, p2);
			m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) + QString(" mm"));
		}
		else if (m_RbVector->isChecked())
		{
			Point pc;
			pc.px = (short)m_Handler3D->Width() / 2;
			pc.py = (short)m_Handler3D->Height() / 2;
			Point p1, p2;
			if (m_CbbLb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = m_Labels[m_CbbLb1->currentItem() - 1].p;
			if (m_CbbLb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = m_Labels[m_CbbLb2->currentItem() - 1].p;
			addLine(&m_Established, p1, p2);
			m_TxtDisplayer->setText(QString("(") + QString::number(Calculatevec(0), 'g', 3) +
															QString(",") + QString::number(Calculatevec(1), 'g', 3) +
															QString(",") + QString::number(Calculatevec(2), 'g', 3) +
															QString(") mm (Mark new start point.)"));
		}
		else if (m_RbAngle->isChecked())
		{
			Point pc;
			pc.px = (short)m_Handler3D->Width() / 2;
			pc.py = (short)m_Handler3D->Height() / 2;
			Point p1, p2, p3;
			if (m_CbbLb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = m_Labels[m_CbbLb1->currentItem() - 1].p;
			if (m_CbbLb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = m_Labels[m_CbbLb2->currentItem() - 1].p;
			if (m_CbbLb3->currentItem() == 0)
				p3 = pc;
			else
				p3 = m_Labels[m_CbbLb3->currentItem() - 1].p;
			addLine(&m_Established, p1, p2);
			addLine(&m_Established, p2, p3);
			m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) + QString(" deg"));
		}
		else if (m_Rb4ptangle->isChecked())
		{
			Point pc;
			pc.px = (short)m_Handler3D->Width() / 2;
			pc.py = (short)m_Handler3D->Height() / 2;
			Point p1, p2, p3, p4;
			if (m_CbbLb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = m_Labels[m_CbbLb1->currentItem() - 1].p;
			if (m_CbbLb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = m_Labels[m_CbbLb2->currentItem() - 1].p;
			if (m_CbbLb3->currentItem() == 0)
				p3 = pc;
			else
				p3 = m_Labels[m_CbbLb3->currentItem() - 1].p;
			if (m_CbbLb4->currentItem() == 0)
				p4 = pc;
			else
				p4 = m_Labels[m_CbbLb4->currentItem() - 1].p;
			addLine(&m_Established, p1, p2);
			addLine(&m_Established, p2, p3);
			addLine(&m_Established, p3, p4);
			m_TxtDisplayer->setText(QString::number(Calculate(), 'g', 3) + QString(" deg"));
		}
		emit Vp1dynChanged(&m_Established, &m_Dynamic);
	}
}

void MeasurementWidget::MethodChanged(int) { UpdateVisualization(); }

void MeasurementWidget::InputtypeChanged(int)
{
	UpdateVisualization();
}

void MeasurementWidget::UpdateVisualization()
{
	if (m_Labels.empty())
	{
		//BLdisconnect(inputgroup, SIGNAL(buttonClicked(int)), this, SLOT(InputtypeChanged(int)));
		m_InputArea->hide();
		m_RbPts->setChecked(true);
		m_RbLbls->setChecked(false);
		//BLconnect(inputgroup, SIGNAL(buttonClicked(int)), this, SLOT(InputtypeChanged(int)));
	}
	else
	{
		m_InputArea->show();
	}

	if (m_RbPts->isChecked() || m_RbVol->isChecked())
	{
		m_State = 0;
		m_Drawing = false;

		m_Established.clear();
		m_Dynamic.clear();
		emit Vp1dynChanged(&m_Established, &m_Dynamic);
		m_StackedWidget->setCurrentIndex(0);
		if (m_RbDist->isChecked() || m_RbThick->isChecked() || m_RbVector->isChecked())
		{
			m_TxtDisplayer->setText("Mark start point.");
		}
		else if (m_RbAngle->isChecked())
		{
			m_TxtDisplayer->setText("Mark first point.");
		}
		else if (m_Rb4ptangle->isChecked())
		{
			m_TxtDisplayer->setText("Mark first point.");
		}
		else if (m_RbVol->isChecked())
		{
			m_TxtDisplayer->setText("Select object.");
		}
	}
	else if (m_RbLbls->isChecked())
	{
		m_StackedWidget->setCurrentIndex(1);
		if (m_RbDist->isChecked() || m_RbThick->isChecked() || m_RbVector->isChecked())
		{
			SetActiveLabels(kP2);
		}
		else if (m_RbAngle->isChecked())
		{
			SetActiveLabels(kP3);
		}
		else if (m_Rb4ptangle->isChecked())
		{
			SetActiveLabels(kP4);
		}
		CbbChanged(0);
	}
}

void MeasurementWidget::Getlabels()
{
	m_Handler3D->GetLabels(&m_Labels);
	m_CbbLb1->clear();
	m_CbbLb2->clear();
	m_CbbLb3->clear();
	m_CbbLb4->clear();

	QObject_disconnect(m_CbbLb1, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_disconnect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_disconnect(m_CbbLb3, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_disconnect(m_CbbLb4, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));

	m_CbbLb1->insertItem(QString("Center"));
	m_CbbLb2->insertItem(QString("Center"));
	m_CbbLb3->insertItem(QString("Center"));
	m_CbbLb4->insertItem(QString("Center"));

	for (size_t i = 0; i < m_Labels.size(); i++)
	{
		m_CbbLb1->insertItem(QString(m_Labels[i].name.c_str()));
		m_CbbLb2->insertItem(QString(m_Labels[i].name.c_str()));
		m_CbbLb3->insertItem(QString(m_Labels[i].name.c_str()));
		m_CbbLb4->insertItem(QString(m_Labels[i].name.c_str()));
	}

	QObject_connect(m_CbbLb1, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_connect(m_CbbLb2, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_connect(m_CbbLb3, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));
	QObject_connect(m_CbbLb4, SIGNAL(activated(int)), this, SLOT(CbbChanged(int)));

	UpdateVisualization();
}

void MeasurementWidget::MarksChanged() { Getlabels(); }

float MeasurementWidget::Calculate()
{
	float thick = m_Handler3D->GetSlicethickness();
	Pair p1 = m_Handler3D->GetPixelsize();

	float value = 0;
	if (m_RbLbls->isChecked())
	{
		if (m_CbbLb1->currentItem() == 0)
		{
			m_Pt[0][0] = (int)m_Handler3D->Width() / 2;
			m_Pt[0][1] = (int)m_Handler3D->Height() / 2;
			m_Pt[0][2] = (int)m_Handler3D->NumSlices() / 2;
		}
		else
		{
			m_Pt[0][0] = (int)m_Labels[m_CbbLb1->currentItem() - 1].p.px;
			m_Pt[0][1] = (int)m_Labels[m_CbbLb1->currentItem() - 1].p.py;
			m_Pt[0][2] = (int)m_Labels[m_CbbLb1->currentItem() - 1].slicenr;
		}
		if (m_CbbLb2->currentItem() == 0)
		{
			m_Pt[1][0] = (int)m_Handler3D->Width() / 2;
			m_Pt[1][1] = (int)m_Handler3D->Height() / 2;
			m_Pt[1][2] = (int)m_Handler3D->NumSlices() / 2;
		}
		else
		{
			m_Pt[1][0] = (int)m_Labels[m_CbbLb2->currentItem() - 1].p.px;
			m_Pt[1][1] = (int)m_Labels[m_CbbLb2->currentItem() - 1].p.py;
			m_Pt[1][2] = (int)m_Labels[m_CbbLb2->currentItem() - 1].slicenr;
		}
		if (m_CbbLb3->currentItem() == 0)
		{
			m_Pt[2][0] = (int)m_Handler3D->Width() / 2;
			m_Pt[2][1] = (int)m_Handler3D->Height() / 2;
			m_Pt[2][2] = (int)m_Handler3D->NumSlices() / 2;
		}
		else
		{
			m_Pt[2][0] = (int)m_Labels[m_CbbLb3->currentItem() - 1].p.px;
			m_Pt[2][1] = (int)m_Labels[m_CbbLb3->currentItem() - 1].p.py;
			m_Pt[2][2] = (int)m_Labels[m_CbbLb3->currentItem() - 1].slicenr;
		}
		if (m_CbbLb4->currentItem() == 0)
		{
			m_Pt[3][0] = (int)m_Handler3D->Width() / 2;
			m_Pt[3][1] = (int)m_Handler3D->Height() / 2;
			m_Pt[3][2] = (int)m_Handler3D->NumSlices() / 2;
		}
		else
		{
			m_Pt[3][0] = (int)m_Labels[m_CbbLb4->currentItem() - 1].p.px;
			m_Pt[3][1] = (int)m_Labels[m_CbbLb4->currentItem() - 1].p.py;
			m_Pt[3][2] = (int)m_Labels[m_CbbLb4->currentItem() - 1].slicenr;
		}
	}

	if (m_RbDist->isChecked())
	{
		value = sqrt((m_Pt[0][0] - m_Pt[1][0]) * (m_Pt[0][0] - m_Pt[1][0]) * p1.high * p1.high +
								 (m_Pt[0][1] - m_Pt[1][1]) * (m_Pt[0][1] - m_Pt[1][1]) * p1.low * p1.low +
								 (m_Pt[0][2] - m_Pt[1][2]) * (m_Pt[0][2] - m_Pt[1][2]) * thick * thick);
	}
	else if (m_RbThick->isChecked())
	{
		int x_dist = abs(m_Pt[0][0] - m_Pt[1][0]);
		if (x_dist != 0)
			x_dist += 1;
		int y_dist = abs(m_Pt[0][1] - m_Pt[1][1]);
		if (y_dist != 0)
			y_dist += 1;
		int z_dist = abs(m_Pt[0][2] - m_Pt[1][2]);
		if (z_dist != 0)
			z_dist += 1;
		if (!(x_dist || y_dist || z_dist))
			x_dist = 1;
		value = sqrt((x_dist) * (x_dist)*p1.high * p1.high +
								 (y_dist) * (y_dist)*p1.low * p1.low +
								 (z_dist) * (z_dist)*thick * thick);
	}
	else if (m_RbAngle->isChecked())
	{
		float l1square =
				(m_Pt[0][0] - m_Pt[1][0]) * (m_Pt[0][0] - m_Pt[1][0]) * p1.high * p1.high +
				(m_Pt[0][1] - m_Pt[1][1]) * (m_Pt[0][1] - m_Pt[1][1]) * p1.low * p1.low +
				(m_Pt[0][2] - m_Pt[1][2]) * (m_Pt[0][2] - m_Pt[1][2]) * thick * thick;
		float l2square =
				(m_Pt[2][0] - m_Pt[1][0]) * (m_Pt[2][0] - m_Pt[1][0]) * p1.high * p1.high +
				(m_Pt[2][1] - m_Pt[1][1]) * (m_Pt[2][1] - m_Pt[1][1]) * p1.low * p1.low +
				(m_Pt[2][2] - m_Pt[1][2]) * (m_Pt[2][2] - m_Pt[1][2]) * thick * thick;
		if (l1square * l2square > 0)
			value = acos(((m_Pt[0][0] - m_Pt[1][0]) * (m_Pt[2][0] - m_Pt[1][0]) *
													 p1.high * p1.high +
											 (m_Pt[0][1] - m_Pt[1][1]) * (m_Pt[2][1] - m_Pt[1][1]) *
													 p1.low * p1.low +
											 (m_Pt[0][2] - m_Pt[1][2]) * (m_Pt[2][2] - m_Pt[1][2]) *
													 thick * thick) /
									 sqrt(l1square * l2square)) *
							180 / 3.141592f;
	}
	else if (m_Rb4ptangle->isChecked())
	{
		float d1[3];
		float d2[3];
		d1[0] = (m_Pt[0][1] - m_Pt[1][1]) * (m_Pt[1][2] - m_Pt[2][2]) * thick * p1.low -
						(m_Pt[0][2] - m_Pt[1][2]) * (m_Pt[1][1] - m_Pt[2][1]) * thick * p1.low;
		d1[1] =
				(m_Pt[0][2] - m_Pt[1][2]) * (m_Pt[1][0] - m_Pt[2][0]) * thick * p1.high -
				(m_Pt[0][0] - m_Pt[1][0]) * (m_Pt[1][2] - m_Pt[2][2]) * thick * p1.high;
		d1[2] =
				(m_Pt[0][0] - m_Pt[1][0]) * (m_Pt[1][1] - m_Pt[2][1]) * p1.high * p1.low -
				(m_Pt[0][1] - m_Pt[1][1]) * (m_Pt[1][0] - m_Pt[2][0]) * p1.high * p1.low;
		d2[0] = (m_Pt[1][1] - m_Pt[2][1]) * (m_Pt[2][2] - m_Pt[3][2]) * thick * p1.low -
						(m_Pt[1][2] - m_Pt[2][2]) * (m_Pt[2][1] - m_Pt[3][1]) * thick * p1.low;
		d2[1] =
				(m_Pt[1][2] - m_Pt[2][2]) * (m_Pt[2][0] - m_Pt[3][0]) * thick * p1.high -
				(m_Pt[1][0] - m_Pt[2][0]) * (m_Pt[2][2] - m_Pt[3][2]) * thick * p1.high;
		d2[2] =
				(m_Pt[1][0] - m_Pt[2][0]) * (m_Pt[2][1] - m_Pt[3][1]) * p1.high * p1.low -
				(m_Pt[1][1] - m_Pt[2][1]) * (m_Pt[2][0] - m_Pt[3][0]) * p1.high * p1.low;
		float l1square = (d1[0] * d1[0] + d1[1] * d1[1] + d1[2] * d1[2]);
		float l2square = (d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2]);
		if (l1square * l2square > 0)
			value = acos((d1[0] * d2[0] + d1[1] * d2[1] + d1[2] * d2[2]) /
									 sqrt(l1square * l2square)) *
							180 / 3.141592f;
	}

	return value;
}

float MeasurementWidget::Calculatevec(unsigned short orient)
{
	float thick = m_Handler3D->GetSlicethickness();
	Pair p1 = m_Handler3D->GetPixelsize();

	float value = 0;
	if (m_RbLbls->isChecked())
	{
		if (m_CbbLb1->currentItem() == 0)
		{
			m_Pt[0][0] = (int)m_Handler3D->Width() / 2;
			m_Pt[0][1] = (int)m_Handler3D->Height() / 2;
			m_Pt[0][2] = (int)m_Handler3D->NumSlices() / 2;
		}
		else
		{
			m_Pt[0][0] = (int)m_Labels[m_CbbLb1->currentItem() - 1].p.px;
			m_Pt[0][1] = (int)m_Labels[m_CbbLb1->currentItem() - 1].p.py;
			m_Pt[0][2] = (int)m_Labels[m_CbbLb1->currentItem() - 1].slicenr;
		}
		if (m_CbbLb2->currentItem() == 0)
		{
			m_Pt[1][0] = (int)m_Handler3D->Width() / 2;
			m_Pt[1][1] = (int)m_Handler3D->Height() / 2;
			m_Pt[1][2] = (int)m_Handler3D->NumSlices() / 2;
		}
		else
		{
			m_Pt[1][0] = (int)m_Labels[m_CbbLb2->currentItem() - 1].p.px;
			m_Pt[1][1] = (int)m_Labels[m_CbbLb2->currentItem() - 1].p.py;
			m_Pt[1][2] = (int)m_Labels[m_CbbLb2->currentItem() - 1].slicenr;
		}
	}

	if (orient == 0)
		value = (m_Pt[1][0] - m_Pt[0][0]) * p1.high;
	else if (orient == 1)
		value = (m_Pt[1][1] - m_Pt[0][1]) * p1.low;
	else if (orient == 2)
		value = (m_Pt[1][2] - m_Pt[0][2]) * thick;

	return value;
}

void MeasurementWidget::Cleanup()
{
	m_Dynamic.clear();
	m_Established.clear();
	m_Drawing = false;
	emit Vp1dynChanged(&m_Established, &m_Dynamic);
}

} // namespace iseg
