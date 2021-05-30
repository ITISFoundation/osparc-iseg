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

#include "SaveOutlinesWidget.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"

#include "Interface/QtConnect.h"

#include <q3hbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qfiledialog.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

SaveOutlinesWidget::SaveOutlinesWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_Handler3D(hand3D)
{
	m_Vbox1 = new Q3VBox(this);
	m_LboTissues = new QListWidget(m_Vbox1);
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); ++i)
	{
		m_LboTissues->addItem(ToQ(TissueInfos::GetTissueName(i)));
	}
	m_LboTissues->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_Hbox4 = new Q3HBox(m_Vbox1);
	m_Hbox7 = new Q3HBox(m_Vbox1);
	m_Hbox6 = new Q3HBox(m_Vbox1);
	m_Hbox8 = new Q3HBox(m_Vbox1);
	m_Hbox9 = new Q3HBox(m_Vbox1);
	m_Hboxslicesbetween = new Q3HBox(m_Vbox1);
	m_Hbox5 = new Q3HBox(m_Vbox1);

	m_LbFile = new QLabel(QString("File Name: "), m_Hbox1);
	m_LeFile = new QLineEdit(m_Hbox1);
	m_PbFile = new QPushButton("Select...", m_Hbox1);

	m_Filetype = new QButtonGroup(this);
	//	filetype->hide();
	m_RbLine = new QRadioButton(QString("Outlines"), m_Hbox2);
	m_RbTriang = new QRadioButton(QString("Triangulate"), m_Hbox2);
	m_Filetype->insert(m_RbLine);
	m_Filetype->insert(m_RbTriang);
	m_RbLine->setChecked(TRUE);
	m_RbTriang->show();
	m_RbLine->show();

	m_LbSimplif = new QLabel("Simplification: ", m_Hbox3);
	m_Simplif = new QButtonGroup(this);
	//	simplif->hide();
	m_RbNone = new QRadioButton(QString("none"), m_Hbox3);
	m_RbDougpeuck = new QRadioButton(QString("Doug-Peucker"), m_Hbox3);
	m_RbDist = new QRadioButton(QString("Distance"), m_Hbox3);
	m_Simplif->insert(m_RbNone);
	m_Simplif->insert(m_RbDougpeuck);
	m_Simplif->insert(m_RbDist);
	m_RbDougpeuck->show();
	//	rb_dist->hide(); //!!!
	m_RbDist->setEnabled(false);
	m_RbNone->setChecked(TRUE);
	m_RbNone->show();

	m_LbDist = new QLabel("Segm. Size: ", m_Hbox4);
	m_SbDist = new QSpinBox(5, 50, 5, m_Hbox4);
	m_SbDist->setValue(5);

	m_LbF1 = new QLabel("Max. Dist.: 0 ", m_Hbox7);
	m_SlF = new QSlider(0, 100, 5, 15, Qt::Horizontal, m_Hbox7);
	m_LbF2 = new QLabel(" 5", m_Hbox7);

	m_LbMinsize = new QLabel("Min. Size: ", m_Hbox6);
	m_SbMinsize = new QSpinBox(1, 999, 1, m_Hbox6);
	m_SbMinsize->setValue(1);

	m_CbExtrusion = new QCheckBox("Extrusions", m_Hbox8);
	m_CbExtrusion->setChecked(false);
	m_LbTopextrusion = new QLabel(" Slices on top: ", m_Hbox8);
	m_SbTopextrusion = new QSpinBox(0, 100, 1, m_Hbox8);
	m_SbTopextrusion->setValue(10);
	m_LbBottomextrusion = new QLabel(" Slices on bottom: ", m_Hbox8);
	m_SbBottomextrusion = new QSpinBox(0, 100, 1, m_Hbox8);
	m_SbBottomextrusion->setValue(10);

	m_CbSmooth = new QCheckBox("Smooth", m_Hbox9);
	m_CbSmooth->setChecked(false);

	m_CbMarchingcubes = new QCheckBox("Use Marching Cubes (faster)", m_Hbox9);
	m_CbMarchingcubes->setChecked(true);

	m_LbBetween = new QLabel("Interpol. Slices: ", m_Hboxslicesbetween);
	m_SbBetween = new QSpinBox(0, 10, 1, m_Hboxslicesbetween);
	m_SbBetween->setValue(0);

	m_PbExec = new QPushButton("Save", m_Hbox5);
	m_PbClose = new QPushButton("Quit", m_Hbox5);

	m_Vbox1->show();

	m_LboTissues->setFixedSize(m_LboTissues->sizeHint());

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	QObject_connect(m_PbFile, SIGNAL(clicked()), this, SLOT(FilePushed()));
	QObject_connect(m_PbExec, SIGNAL(clicked()), this, SLOT(SavePushed()));
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Filetype, SIGNAL(buttonClicked(int)), this, SLOT(ModeChanged()));
	QObject_connect(m_CbExtrusion, SIGNAL(clicked()), this, SLOT(ModeChanged()));
	QObject_connect(m_Simplif, SIGNAL(buttonClicked(int)), this, SLOT(SimplifChanged()));

	ModeChanged();
}

SaveOutlinesWidget::~SaveOutlinesWidget()
{
	delete m_Vbox1;
	delete m_Filetype;
	delete m_Simplif;
}

void SaveOutlinesWidget::ModeChanged()
{
	if (m_RbLine->isChecked())
	{
		m_RbDougpeuck->setText("Doug-Peucker");
		m_LbF1->setText("Max. Dist.: 0 ");
		m_LbF2->setText(" 5");
		//		hbox3->show();
		m_Hbox6->show();
		m_Hbox8->show();
		m_Hbox9->hide();
#ifdef ISEG_RELEASEVERSION
		m_Hbox3->show();
		m_Hbox7->show();
#endif
		if (m_CbExtrusion->isChecked())
		{
			m_LbTopextrusion->show();
			m_LbBottomextrusion->show();
			m_SbTopextrusion->show();
			m_SbBottomextrusion->show();
		}
		else
		{
			m_LbTopextrusion->hide();
			m_LbBottomextrusion->hide();
			m_SbTopextrusion->hide();
			m_SbBottomextrusion->hide();
		}
		//		hbox4->show();
		m_Hboxslicesbetween->show();
		SimplifChanged();
	}
	else
	{
		m_RbDougpeuck->setText("collapse");
		m_LbF1->setText("Ratio: 1 ");
		m_LbF2->setText(" 100%");
		//		hbox3->show();
		//		hbox4->hide();
		//		hbox7->hide();
		SimplifChanged();
		m_Hbox6->hide();
		m_Hbox8->hide();
		m_Hbox9->show();
#ifdef ISEG_RELEASEVERSION
		m_Hbox3->show();
		m_Hbox7->hide();
#endif
		m_Hboxslicesbetween->hide();
	}
}

void SaveOutlinesWidget::SimplifChanged()
{
	if (m_RbNone->isChecked())
	{
		m_Hbox4->hide();
		m_Hbox7->hide();
	}
	else if (m_RbDougpeuck->isChecked())
	{
		m_Hbox4->hide();
		m_Hbox7->show();
	}
	else if (m_RbDist->isChecked())
	{
		m_Hbox4->show();
		m_Hbox7->hide();
	}
}

void SaveOutlinesWidget::FilePushed()
{
	QString loadfilename;
	if (m_RbTriang->isChecked())
		loadfilename = QFileDialog::getSaveFileName(QString::null, "Surface grids (*.vtp *.dat *.stl)", this);
	else
		loadfilename =
				QFileDialog::getSaveFileName(QString::null, "Files (*.txt)", this);

	m_LeFile->setText(loadfilename);
}

void SaveOutlinesWidget::SavePushed()
{
	std::vector<tissues_size_t> vtissues;
	for (tissues_size_t i = 0; i < TissueInfos::GetTissueCount(); i++)
	{
		if (m_LboTissues->item((int)i)->isSelected())
		{
			vtissues.push_back((tissues_size_t)i + 1);
		}
	}

	if (vtissues.empty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("No tissues have been selected."), QMessageBox::Ok, 0);
		return;
	}

	if (!(m_LeFile->text()).isEmpty())
	{
		QString loadfilename = m_LeFile->text();
		if (m_RbTriang->isChecked())
		{
			ISEG_INFO_MSG("triangulating...");
			// If no extension given, add a default one
			if (loadfilename.length() > 4 &&
					!loadfilename.endsWith(QString(".vtp"), Qt::CaseInsensitive) &&
					!loadfilename.endsWith(QString(".stl"), Qt::CaseInsensitive) &&
					!loadfilename.endsWith(QString(".dat"), Qt::CaseInsensitive))
				loadfilename.append(".dat");

			float ratio = m_SlF->value() * 0.0099f + 0.01f;
			if (m_RbNone->isChecked())
				ratio = 1.0;

			unsigned int smoothingiter = 0;
			if (m_CbSmooth->isChecked())
				smoothingiter = 9;

			bool usemc = false;
			if (m_CbMarchingcubes->isChecked())
				usemc = true;

			// Call method to extract surface, smooth, simplify and finally save it to file
			int number_of_errors = m_Handler3D->ExtractTissueSurfaces(loadfilename.toStdString(), vtissues, usemc, ratio, smoothingiter);

			if (number_of_errors < 0)
			{
				QMessageBox::warning(this, "iSeg", "Surface extraction failed.\n\nMaybe something is "
																					 "wrong the pixel type or label field.",
						QMessageBox::Ok | QMessageBox::Default);
			}
			else if (number_of_errors > 0)
			{
				QMessageBox::warning(this, "iSeg", "Surface extraction might have failed.\n\nPlease "
																					 "verify the tissue names and colors carefully.",
						QMessageBox::Ok | QMessageBox::Default);
			}
		}
		else
		{
			if (loadfilename.length() > 4 &&
					!loadfilename.endsWith(QString(".txt")))
				loadfilename.append(".txt");

			if (m_SbBetween->value() == 0)
			{
				Pair pair1 = m_Handler3D->GetPixelsize();
				m_Handler3D->SetPixelsize(pair1.high / 2, pair1.low / 2);
				//				handler3D->extract_contours2(sb_minsize->value(), vtissues);
				if (m_RbDougpeuck->isChecked())
				{
					m_Handler3D->ExtractContours2Xmirrored(m_SbMinsize->value(), vtissues, m_SlF->value() * 0.05f);
					//					handler3D->dougpeuck_line(sl_f->value()*0.05f*2);
				}
				else if (m_RbDist->isChecked())
				{
					m_Handler3D->ExtractContours2Xmirrored(m_SbMinsize->value(), vtissues);
				}
				else if (m_RbNone->isChecked())
				{
					m_Handler3D->ExtractContours2Xmirrored(m_SbMinsize->value(), vtissues);
				}
				m_Handler3D->ShiftContours(-(int)m_Handler3D->Width(), -(int)m_Handler3D->Height());
				if (m_CbExtrusion->isChecked())
				{
					m_Handler3D->SetextrusionContours(m_SbTopextrusion->value() - 1, m_SbBottomextrusion->value() - 1);
				}
				m_Handler3D->SaveContours(loadfilename.ascii());
#define ROTTERDAM
#ifdef ROTTERDAM
				FILE* fp = fopen(loadfilename.ascii(), "a");
				float disp[3];
				m_Handler3D->GetDisplacement(disp);
				fprintf(fp, "V1\nX%i %i %i %i O%f %f %f\n", -(int)m_Handler3D->Width(), (int)m_Handler3D->Width(), -(int)m_Handler3D->Height(), (int)m_Handler3D->Height(), disp[0], disp[1], disp[2]); // TODO: rotation
				std::vector<AugmentedMark> labels;
				m_Handler3D->GetLabels(&labels);
				if (!labels.empty())
					fprintf(fp, "V1\n");
				for (size_t i = 0; i < labels.size(); i++)
				{
					fprintf(fp, "S%i 1\nL%i %i %s\n", (int)labels[i].slicenr, (int)m_Handler3D->Width() - 1 - (int)labels[i].p.px * 2, (int)labels[i].p.py * 2 - (int)m_Handler3D->Height() + 1, labels[i].name.c_str());
				}
				fclose(fp);
#endif
				m_Handler3D->SetPixelsize(pair1.high, pair1.low);
				if (m_CbExtrusion->isChecked())
				{
					m_Handler3D->ResetextrusionContours();
				}
			}
			else
			{
				m_Handler3D->ExtractinterpolatesaveContours2Xmirrored(m_SbMinsize->value(), vtissues, m_SbBetween->value(), m_RbDougpeuck->isChecked(), m_SlF->value() * 0.05f, loadfilename.ascii());
			}
		}
		close();
	}
	else
	{
		QMessageBox::warning(this, tr("Warning"), tr("No filename has been specified."), QMessageBox::Ok, 0);
		return;
	}
}

} // namespace iseg
