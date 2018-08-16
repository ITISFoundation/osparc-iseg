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

#include "SaveOutlinesWidget.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"

#include <qfiledialog.h>
#include <q3hbox.h>
#include <q3listbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qwidget.h>

using namespace std;
using namespace iseg;

SaveOutlinesWidget::SaveOutlinesWidget(SlicesHandler* hand3D, QWidget* parent,
									   const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	vbox1 = new Q3VBox(this);
	lbo_tissues = new Q3ListBox(vbox1);
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); ++i)
	{
		lbo_tissues->insertItem(ToQ(TissueInfos::GetTissueName(i)));
	}
	lbo_tissues->setMultiSelection(true);
	hbox2 = new Q3HBox(vbox1);
	hbox1 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	hbox7 = new Q3HBox(vbox1);
	hbox6 = new Q3HBox(vbox1);
	hbox8 = new Q3HBox(vbox1);
	hbox9 = new Q3HBox(vbox1);
	hboxslicesbetween = new Q3HBox(vbox1);
	hbox5 = new Q3HBox(vbox1);

	lb_file = new QLabel(QString("File Name: "), hbox1);
	le_file = new QLineEdit(hbox1);
	pb_file = new QPushButton("Select...", hbox1);

	filetype = new QButtonGroup(this);
	//	filetype->hide();
	rb_line = new QRadioButton(QString("Outlines"), hbox2);
	rb_triang = new QRadioButton(QString("Triangulate"), hbox2);
	filetype->insert(rb_line);
	filetype->insert(rb_triang);
	rb_line->setChecked(TRUE);
	rb_triang->show();
	rb_line->show();

	lb_simplif = new QLabel("Simplification: ", hbox3);
	simplif = new QButtonGroup(this);
	//	simplif->hide();
	rb_none = new QRadioButton(QString("none"), hbox3);
	rb_dougpeuck = new QRadioButton(QString("Doug-Peucker"), hbox3);
	rb_dist = new QRadioButton(QString("Distance"), hbox3);
	simplif->insert(rb_none);
	simplif->insert(rb_dougpeuck);
	simplif->insert(rb_dist);
	rb_dougpeuck->show();
	//	rb_dist->hide(); //!!!
	rb_dist->setEnabled(false);
	rb_none->setChecked(TRUE);
	rb_none->show();

	lb_dist = new QLabel("Segm. Size: ", hbox4);
	sb_dist = new QSpinBox(5, 50, 5, hbox4);
	sb_dist->setValue(5);

	lb_f1 = new QLabel("Max. Dist.: 0 ", hbox7);
	sl_f = new QSlider(0, 100, 5, 15, Qt::Horizontal, hbox7);
	lb_f2 = new QLabel(" 5", hbox7);

	lb_minsize = new QLabel("Min. Size: ", hbox6);
	sb_minsize = new QSpinBox(1, 999, 1, hbox6);
	sb_minsize->setValue(1);

	cb_extrusion = new QCheckBox("Extrusions", hbox8);
	cb_extrusion->setChecked(false);
	lb_topextrusion = new QLabel(" Slices on top: ", hbox8);
	sb_topextrusion = new QSpinBox(0, 100, 1, hbox8);
	sb_topextrusion->setValue(10);
	lb_bottomextrusion = new QLabel(" Slices on bottom: ", hbox8);
	sb_bottomextrusion = new QSpinBox(0, 100, 1, hbox8);
	sb_bottomextrusion->setValue(10);

	cb_smooth = new QCheckBox("Smooth", hbox9);
	cb_smooth->setChecked(false);

	cb_marchingcubes = new QCheckBox("Use Marching Cubes (faster)", hbox9);
	cb_marchingcubes->setChecked(true);

	lb_between = new QLabel("Interpol. Slices: ", hboxslicesbetween);
	sb_between = new QSpinBox(0, 10, 1, hboxslicesbetween);
	sb_between->setValue(0);

	pb_exec = new QPushButton("Save", hbox5);
	pb_close = new QPushButton("Quit", hbox5);

	vbox1->show();

	lbo_tissues->setFixedSize(lbo_tissues->sizeHint());

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(pb_file, SIGNAL(clicked()), this, SLOT(file_pushed()));
	QObject::connect(pb_exec, SIGNAL(clicked()), this, SLOT(save_pushed()));
	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(close()));
	QObject::connect(filetype, SIGNAL(buttonClicked(int)), this,
					 SLOT(mode_changed()));
	QObject::connect(cb_extrusion, SIGNAL(clicked()), this,
					 SLOT(mode_changed()));
	QObject::connect(simplif, SIGNAL(buttonClicked(int)), this,
					 SLOT(simplif_changed()));

	mode_changed();

	return;
}

SaveOutlinesWidget::~SaveOutlinesWidget()
{
	delete vbox1;
	delete filetype;
	delete simplif;
}

void SaveOutlinesWidget::mode_changed()
{
	if (rb_line->isOn())
	{
		rb_dougpeuck->setText("Doug-Peucker");
		lb_f1->setText("Max. Dist.: 0 ");
		lb_f2->setText(" 5");
		//		hbox3->show();
		hbox6->show();
		hbox8->show();
		hbox9->hide();
#ifdef ISEG_RELEASEVERSION
		hbox3->show();
		hbox7->show();
#endif
		if (cb_extrusion->isChecked())
		{
			lb_topextrusion->show();
			lb_bottomextrusion->show();
			sb_topextrusion->show();
			sb_bottomextrusion->show();
		}
		else
		{
			lb_topextrusion->hide();
			lb_bottomextrusion->hide();
			sb_topextrusion->hide();
			sb_bottomextrusion->hide();
		}
		//		hbox4->show();
		hboxslicesbetween->show();
		simplif_changed();
	}
	else
	{
		rb_dougpeuck->setText("collapse");
		lb_f1->setText("Ratio: 1 ");
		lb_f2->setText(" 100%");
		//		hbox3->show();
		//		hbox4->hide();
		//		hbox7->hide();
		simplif_changed();
		hbox6->hide();
		hbox8->hide();
		hbox9->show();
#ifdef ISEG_RELEASEVERSION
		hbox3->show();
		hbox7->hide();
#endif
		hboxslicesbetween->hide();
	}
}

void SaveOutlinesWidget::simplif_changed()
{
	if (rb_none->isOn())
	{
		hbox4->hide();
		hbox7->hide();
	}
	else if (rb_dougpeuck->isOn())
	{
		hbox4->hide();
		hbox7->show();
	}
	else if (rb_dist->isOn())
	{
		hbox4->show();
		hbox7->hide();
	}
}

void SaveOutlinesWidget::file_pushed()
{
	QString loadfilename;
	if (rb_triang->isOn())
		loadfilename = QFileDialog::getSaveFileName(
			QString::null, "Surface grids (*.vtp *.dat *.stl)", this);
	else
		loadfilename =
			QFileDialog::getSaveFileName(QString::null, "Files (*.txt)", this);

	le_file->setText(loadfilename);
	return;
}

void SaveOutlinesWidget::save_pushed()
{
	vector<tissues_size_t> vtissues;
	for (tissues_size_t i = 0; i < TissueInfos::GetTissueCount(); i++)
	{
		if (lbo_tissues->isSelected((int)i))
		{
			vtissues.push_back((tissues_size_t)i + 1);
		}
	}

	if (vtissues.empty())
	{
		QMessageBox::warning(this, tr("Warning"),
							 tr("No tissues have been selected."),
							 QMessageBox::Ok, 0);
		return;
	}

	if (!(le_file->text()).isEmpty())
	{
		QString loadfilename = le_file->text();
		if (rb_triang->isOn())
		{
			ISEG_INFO_MSG("triangulating...");
			// If no extension given, add a default one
			if (loadfilename.length() > 4 &&
				!loadfilename.toLower().endsWith(QString(".vtp")) &&
				!loadfilename.toLower().endsWith(QString(".stl")) &&
				!loadfilename.toLower().endsWith(QString(".dat")))
				loadfilename.append(".dat");

			float ratio = sl_f->value() * 0.0099f + 0.01f;
			if (rb_none->isChecked())
				ratio = 1.0;

			unsigned int smoothingiter = 0;
			if (cb_smooth->isChecked())
				smoothingiter = 9;

			bool usemc = false;
			if (cb_marchingcubes->isChecked())
				usemc = true;

			// Call method to extract surface, smooth, simplify and finally save it to file
			int numberOfErrors = handler3D->extract_tissue_surfaces(
				loadfilename.ascii(), vtissues, usemc, ratio, smoothingiter);

			if (numberOfErrors < 0)
			{
				QMessageBox::warning(
					this, "iSeg",
					"Surface extraction failed.\n\nMaybe something is "
					"wrong the pixel type or label field.",
					QMessageBox::Ok | QMessageBox::Default);
			}
			else if (numberOfErrors > 0)
			{
				QMessageBox::warning(
					this, "iSeg",
					"Surface extraction might have failed.\n\nPlease "
					"verify the tissue names and colors carefully.",
					QMessageBox::Ok | QMessageBox::Default);
			}
		}
		else
		{
			if (loadfilename.length() > 4 &&
				!loadfilename.endsWith(QString(".txt")))
				loadfilename.append(".txt");

			if (sb_between->value() == 0)
			{
				Pair pair1 = handler3D->get_pixelsize();
				handler3D->set_pixelsize(pair1.high / 2, pair1.low / 2);
				//				handler3D->extract_contours2(sb_minsize->value(), vtissues);
				if (rb_dougpeuck->isOn())
				{
					handler3D->extract_contours2_xmirrored(
						sb_minsize->value(), vtissues, sl_f->value() * 0.05f);
					//					handler3D->dougpeuck_line(sl_f->value()*0.05f*2);
				}
				else if (rb_dist->isOn())
				{
					handler3D->extract_contours2_xmirrored(sb_minsize->value(),
														   vtissues);
				}
				else if (rb_none->isOn())
				{
					handler3D->extract_contours2_xmirrored(sb_minsize->value(),
														   vtissues);
				}
				handler3D->shift_contours(-(int)handler3D->width(),
										  -(int)handler3D->height());
				if (cb_extrusion->isChecked())
				{
					handler3D->setextrusion_contours(
						sb_topextrusion->value() - 1,
						sb_bottomextrusion->value() - 1);
				}
				handler3D->save_contours(loadfilename.ascii());
#define ROTTERDAM
#ifdef ROTTERDAM
				FILE* fp = fopen(loadfilename.ascii(), "a");
				float disp[3];
				handler3D->get_displacement(disp);
				fprintf(fp, "V1\nX%i %i %i %i O%f %f %f\n",
						-(int)handler3D->width(),
						(int)handler3D->width(),
						-(int)handler3D->height(),
						(int)handler3D->height(), disp[0], disp[1],
						disp[2]); // TODO: rotation
				vector<augmentedmark> labels;
				handler3D->get_labels(&labels);
				if (!labels.empty())
					fprintf(fp, "V1\n");
				for (size_t i = 0; i < labels.size(); i++)
				{
					fprintf(fp, "S%i 1\nL%i %i %s\n", (int)labels[i].slicenr,
							(int)handler3D->width() - 1 -
								(int)labels[i].p.px * 2,
							(int)labels[i].p.py * 2 -
								(int)handler3D->height() + 1,
							labels[i].name.c_str());
				}
				fclose(fp);
#endif
				handler3D->set_pixelsize(pair1.high, pair1.low);
				if (cb_extrusion->isChecked())
				{
					handler3D->resetextrusion_contours();
				}
			}
			else
			{
				handler3D->extractinterpolatesave_contours2_xmirrored(
					sb_minsize->value(), vtissues, sb_between->value(),
					rb_dougpeuck->isOn(), sl_f->value() * 0.05f,
					loadfilename.ascii());
			}
		}
		close();
	}
	else
	{
		QMessageBox::warning(this, tr("Warning"),
							 tr("No filename has been specified."),
							 QMessageBox::Ok, 0);
		return;
	}
}
