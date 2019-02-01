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
#include "ThresholdWidget.h"
#include "bmp_read_1.h"

#include "Core/Pair.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QtGui>
#include <q3listbox.h>
#include <q3vbox.h>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>
#include <fstream>

using namespace iseg;

ThresholdWidget::ThresholdWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Segment tissues based on thresholding techniques."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	lower = 0;
	upper = 255;
	hboxoverall = new Q3HBox(this);
	hboxoverall->setMargin(8);
	vboxmethods = new Q3VBox(hboxoverall);
	vbox1 = new Q3VBox(hboxoverall);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox2a = new Q3HBox(vbox1);
	hboxfilenames = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	hbox5 = new Q3HBox(vbox1);
	//	hbox6=new Q3HBox(vbox1);
	allslices = new QCheckBox(QString("Apply to all slices"), vbox1);
	hbox1b = new Q3HBox(vbox1);
	pushexec = new QPushButton("Execute", vbox1);

	txt_nrtissues = new QLabel("#Tissues ", hbox1);
	sb_nrtissues = new QSpinBox(2, 30, 1, hbox1);
	sb_nrtissues->setValue(2);
	pb_saveborders = new QPushButton("Save...", hbox1);
	pb_loadborders = new QPushButton("Open...", hbox1);
	hbox7 = new Q3HBox(hbox1);
	txt_dim = new QLabel(" #Dims ", hbox7);
	sb_dim = new QSpinBox(1, 10, 1, hbox7);
	sb_dim->setValue(1);

	txt_tissuenr = new QLabel("Limit-Nr: ", hbox2);
	sb_tissuenr = new QSpinBox(1, sb_nrtissues->value(), 1, hbox2);
	sb_tissuenr->setValue(1);
	txt_slider = new QLabel("Weight: ", hbox2a);
	txt_lower = new QLabel("xxxxxxx", hbox2a);
	slider = new QSlider(Qt::Horizontal, hbox2a);
	slider->setRange(0, 200);
	slider->setValue(200);
	txt_upper = new QLabel("xxxxxxx", hbox2a);
	le_borderval = new QLineEdit(hbox2a);
	//	pushrange=new QPushButton("Get Range",hbox2);

	txt_filename = new QLabel("Filename: ", hboxfilenames);
	le_filename = new QLineEdit(hboxfilenames);
	pushfilename = new QPushButton("Select", hboxfilenames);
	buttonR = new QCheckBox("R", hboxfilenames);
	buttonG = new QCheckBox("G", hboxfilenames);
	buttonB = new QCheckBox("B", hboxfilenames);
	buttonA = new QCheckBox("A", hboxfilenames);
	buttonR->setVisible(true);
	buttonG->setVisible(true);
	buttonB->setVisible(true);
	buttonA->setVisible(true);

	filenames.clear();

	subsect = new QCheckBox("Subsection ", hbox3);
	hbox8 = new Q3HBox(hbox3);
	vbox2 = new Q3VBox(hbox8);
	vbox3 = new Q3VBox(hbox8);
	vbox4 = new Q3VBox(hbox8);
	vbox5 = new Q3VBox(hbox8);
	txt_px = new QLabel("px: ", vbox2);
	txt_py = new QLabel(" py: ", vbox4);
	txt_lx = new QLabel("lx: ", vbox2);
	txt_ly = new QLabel(" ly: ", vbox4);
	sb_px = new QSpinBox(0, (int)bmphand->return_width(), 1, vbox3);
	sb_px->setValue(0);
	sb_py = new QSpinBox(0, (int)bmphand->return_height(), 1, vbox5);
	sb_py->setValue(0);
	sb_lx = new QSpinBox(0, (int)bmphand->return_width(), 1, vbox3);
	sb_lx->setValue(bmphand->return_width());
	sb_ly = new QSpinBox(0, (int)bmphand->return_height(), 1, vbox5);
	sb_ly->setValue(bmphand->return_height());

	txt_minpix = new QLabel("Min. Pixels: ", hbox4);
	sb_minpix = new QSpinBox(0, 100000, 10, hbox4);
	sb_minpix->setValue(50);
	txt_ratio = new QLabel("Ratio: ", hbox4);
	ratio = new QSlider(Qt::Horizontal, hbox4);
	ratio->setRange(0, 200);
	ratio->setValue(160);

	txt_iternr = new QLabel("#Iterations: ", hbox5);
	sb_iternr = new QSpinBox(0, 1000, 5, hbox5);
	sb_iternr->setValue(100);
	txt_converge = new QLabel("Converge: ", hbox5);
	sb_converge = new QSpinBox(0, 100000, 100, hbox5);
	sb_converge->setValue(1000);

	cb_useCenterFile = new QCheckBox(QString("Use InitCenters:"), hbox1b);
	cb_useCenterFile->setChecked(false);
	le_centerFilename = new QLineEdit(hbox1b);
	pushcenterFilename = new QPushButton("Select", hbox1b);
	le_centerFilename->setVisible(false);
	pushcenterFilename->setVisible(false);

	rb_manual = new QRadioButton(QString("Manual"), vboxmethods);
	rb_histo = new QRadioButton(QString("Histo"), vboxmethods);
	rb_kmeans = new QRadioButton(QString("k-means"), vboxmethods);
	rb_EM = new QRadioButton(QString("EM"), vboxmethods);
	modegroup = new QButtonGroup(this);
	//	modegroup->hide();
	modegroup->insert(rb_manual);
	modegroup->insert(rb_histo);
	modegroup->insert(rb_kmeans);
	modegroup->insert(rb_EM);
	rb_manual->setChecked(TRUE);

	slider->setFixedWidth(300);
	ratio->setFixedWidth(200);

	threshs[0] = float(sb_nrtissues->value() - 1);
	for (unsigned i = 0; i < 20; i++)
	{
		bits1[i] = 0;
		threshs[i + 1] = upper;
		weights[i] = 1.0f;
	}

	getrange();
	subsect_toggled();

	method_changed(3);

	vboxmethods->setMargin(5);
	vbox1->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	hbox7->setFixedSize(hbox7->sizeHint());
	hbox1->setFixedSize(hbox1->sizeHint());
	hbox2->setFixedSize(hbox2->sizeHint() + QSize(20, 0));
	hbox2a->setFixedSize(hbox2a->sizeHint());
	hboxfilenames->setFixedSize(hboxfilenames->sizeHint());
	vbox2->setFixedSize(vbox2->sizeHint());
	vbox3->setFixedSize(vbox3->sizeHint());
	vbox4->setFixedSize(vbox4->sizeHint());
	vbox5->setFixedSize(vbox5->sizeHint());
	hbox8->setFixedSize(hbox8->sizeHint());
	hbox3->setFixedSize(hbox3->sizeHint());
	hbox4->setFixedSize(hbox4->sizeHint());
	hbox5->setFixedSize(hbox5->sizeHint());
	//	hbox6->setFixedSize(hbox6->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint() + QSize(0, 50));
	//	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	//	hboxoverall->setFixedSize(hboxoverall->sizeHint());
	//	setFixedSize(vbox1->size());

	buttonR->setVisible(false);
	buttonG->setVisible(false);
	buttonB->setVisible(false);
	buttonA->setVisible(false);

	method_changed(0);

	QObject::connect(subsect, SIGNAL(clicked()), this, SLOT(subsect_toggled()));
	QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
			SLOT(method_changed(int)));
	//	QObject::connect(pushrange,SIGNAL(clicked()),this,SLOT(getrange()));
	QObject::connect(pushexec, SIGNAL(clicked()), this, SLOT(execute()));
	QObject::connect(pb_saveborders, SIGNAL(clicked()), this,
			SLOT(saveborders_execute()));
	QObject::connect(pb_loadborders, SIGNAL(clicked()), this,
			SLOT(loadborders_execute()));
	QObject::connect(sb_nrtissues, SIGNAL(valueChanged(int)), this,
			SLOT(nrtissues_changed(int)));
	QObject::connect(sb_dim, SIGNAL(valueChanged(int)), this,
			SLOT(dim_changed(int)));
	QObject::connect(sb_tissuenr, SIGNAL(valueChanged(int)), this,
			SLOT(tissuenr_changed(int)));
	QObject::connect(slider, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::connect(slider, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(slider, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(le_borderval, SIGNAL(editingFinished()), this,
			SLOT(le_borderval_returnpressed()));
	QObject::connect(pushfilename, SIGNAL(clicked()), this,
			SLOT(select_pushed()));
	QObject::connect(buttonR, SIGNAL(stateChanged(int)), this,
			SLOT(RGBA_changed(int)));
	QObject::connect(buttonG, SIGNAL(stateChanged(int)), this,
			SLOT(RGBA_changed(int)));
	QObject::connect(buttonB, SIGNAL(stateChanged(int)), this,
			SLOT(RGBA_changed(int)));
	QObject::connect(buttonA, SIGNAL(stateChanged(int)), this,
			SLOT(RGBA_changed(int)));
	QObject::connect(cb_useCenterFile, SIGNAL(stateChanged(int)), this,
			SLOT(useCenterFile_changed(int)));
	QObject::connect(pushcenterFilename, SIGNAL(clicked()), this,
			SLOT(selectCenterFile_pushed()));
}

ThresholdWidget::~ThresholdWidget()
{
	delete vbox1;
	delete modegroup;
}

void ThresholdWidget::execute()
{
	unsigned char modedummy;

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (rb_manual->isOn())
	{
		if (allslices->isChecked())
			handler3D->threshold(threshs);
		else
			bmphand->threshold(threshs);
	}
	else if (rb_histo->isOn())
	{
		bmphand->swap_bmpwork();

		if (subsect->isOn())
		{
			Point p;
			p.px = (unsigned short)sb_px->value();
			p.py = (unsigned short)sb_py->value();
			bmphand->make_histogram(p, sb_lx->value(), sb_ly->value(), true);
		}
		else
			bmphand->make_histogram(true);

		bmphand->gaussian_hist(1.0f);
		bmphand->swap_bmpwork();

		float* thresh1 = bmphand->find_modal((unsigned)sb_minpix->value(),
				0.005f * ratio->value());
		if (allslices->isChecked())
			handler3D->threshold(thresh1);
		else
			bmphand->threshold(thresh1);
		free(thresh1);
	}
	else if (rb_kmeans->isOn())
	{
		FILE* fp = fopen("C:\\gamma.txt", "r");
		if (fp != nullptr)
		{
			float** centers = new float*[sb_nrtissues->value()];
			for (int i = 0; i < sb_nrtissues->value(); i++)
				centers[i] = new float[sb_dim->value()];
			float* tol_f = new float[sb_dim->value()];
			float* tol_d = new float[sb_dim->value()];
			for (int i = 0; i < sb_nrtissues->value(); i++)
			{
				for (int j = 0; j < sb_dim->value(); j++)
					fscanf(fp, "%f", &(centers[i][j]));
			}
			for (int j = 0; j < sb_dim->value(); j++)
				fscanf(fp, "%f", &(tol_f[j]));
			for (int j = 1; j < sb_dim->value(); j++)
				fscanf(fp, "%f", &(tol_d[j]));
			tol_d[0] = 0.0f;
			fclose(fp);
			std::vector<std::string> mhdfiles;
			for (int i = 0; i + 1 < sb_dim->value(); i++)
				mhdfiles.push_back(std::string(filenames[i].ascii()));
			if (allslices->isChecked())
				handler3D->gamma_mhd(handler3D->active_slice(),
						(short)sb_nrtissues->value(),
						(short)sb_dim->value(), mhdfiles, weights,
						centers, tol_f, tol_d);
			else
				bmphand->gamma_mhd(
						(short)sb_nrtissues->value(), (short)sb_dim->value(),
						mhdfiles, handler3D->active_slice(), weights, centers,
						tol_f, tol_d, handler3D->get_pixelsize());
			delete[] tol_d;
			delete[] tol_f;
			for (int i = 0; i < sb_nrtissues->value(); i++)
				delete[] centers[i];
			delete[] centers;
		}
		else
		{
			std::vector<std::string> kmeansfiles;
			for (int i = 0; i + 1 < sb_dim->value(); i++)
				kmeansfiles.push_back(std::string(filenames[i].ascii()));
			if (kmeansfiles.size() > 0)
			{
				if (kmeansfiles[0].substr(kmeansfiles[0].find_last_of(".") +
																	1) == "png")
				{
					std::vector<int> extractChannels;
					if (buttonR->isChecked())
						extractChannels.push_back(0);
					if (buttonG->isChecked())
						extractChannels.push_back(1);
					if (buttonB->isChecked())
						extractChannels.push_back(2);
					if (buttonA->isChecked())
						extractChannels.push_back(3);
					if (sb_dim->value() != extractChannels.size() + 1)
						return;
					if (allslices->isChecked())
						handler3D->kmeans_png(
								handler3D->active_slice(),
								(short)sb_nrtissues->value(),
								(short)sb_dim->value(), kmeansfiles,
								extractChannels, weights,
								(unsigned int)sb_iternr->value(),
								(unsigned int)sb_converge->value(),
								centerFilename.toStdString());
					else
						bmphand->kmeans_png(
								(short)sb_nrtissues->value(),
								(short)sb_dim->value(), kmeansfiles,
								extractChannels, handler3D->active_slice(),
								weights, (unsigned int)sb_iternr->value(),
								(unsigned int)sb_converge->value(),
								centerFilename.toStdString());
				}
				else
				{
					if (allslices->isChecked())
						handler3D->kmeans_mhd(
								handler3D->active_slice(),
								(short)sb_nrtissues->value(),
								(short)sb_dim->value(), kmeansfiles, weights,
								(unsigned int)sb_iternr->value(),
								(unsigned int)sb_converge->value());
					else
						bmphand->kmeans_mhd((short)sb_nrtissues->value(),
								(short)sb_dim->value(), kmeansfiles,
								handler3D->active_slice(),
								weights,
								(unsigned int)sb_iternr->value(),
								(unsigned int)sb_converge->value());
				}
			}
			else
			{
				if (allslices->isChecked())
					handler3D->kmeans_mhd(
							handler3D->active_slice(),
							(short)sb_nrtissues->value(), (short)sb_dim->value(),
							kmeansfiles, weights, (unsigned int)sb_iternr->value(),
							(unsigned int)sb_converge->value());
				else
					bmphand->kmeans_mhd((short)sb_nrtissues->value(),
							(short)sb_dim->value(), kmeansfiles,
							handler3D->active_slice(), weights,
							(unsigned int)sb_iternr->value(),
							(unsigned int)sb_converge->value());
			}
		}
	}
	else
	{
		//		EM em;
		for (int i = 0; i < sb_dim->value(); i++)
		{
			if (bits1[i] == 0)
				bits[i] = bmphand->return_bmp();
			else
				bits[i] = bmphand->getstack(bits1[i], modedummy);
		}

		if (allslices->isChecked())
			handler3D->em(handler3D->active_slice(),
					(short)sb_nrtissues->value(),
					(unsigned int)sb_iternr->value(),
					(unsigned int)sb_converge->value());
		else
			bmphand->em((short)sb_nrtissues->value(), (short)sb_dim->value(),
					bits, weights, (unsigned int)sb_iternr->value(),
					(unsigned int)sb_converge->value());
	}

	emit end_datachange(this);
}

void ThresholdWidget::method_changed(int)
{
	if (hideparams)
	{
		if (!rb_histo->isOn())
			rb_histo->hide();
		if (!rb_EM->isOn())
			rb_EM->hide();
	}
	else
	{
		rb_histo->show();
		rb_EM->show();
	}
	if (rb_manual->isOn())
	{
		pb_saveborders->show();
		pb_loadborders->show();
		hbox1->show();
		if (hideparams)
		{
			pb_saveborders->hide();
			pb_loadborders->hide();
		}
		nrtissues_changed(sb_nrtissues->value());
		hbox7->hide();
		hbox3->hide();
		hbox4->hide();
		hbox5->hide();
		txt_slider->setText("Thresh: ");
		txt_tissuenr->setText("Limit-Nr: ");
		le_borderval->setText(
				QString::number(threshs[sb_tissuenr->value()], 'g', 3));
		le_borderval->show();
		txt_lower->show();
		txt_upper->show();
		hbox2a->show();
		//		pushrange->show();
		//		execute();
	}
	else if (rb_histo->isOn())
	{
		pb_saveborders->hide();
		pb_loadborders->hide();
		hbox1->hide();
		hbox2->hide();
		hbox2a->hide();
		if (hideparams)
		{
			hbox3->hide();
			hbox4->hide();
		}
		else
		{
			hbox3->show();
			hbox4->show();
		}
		hbox5->hide();
		le_borderval->hide();
	}
	else if (rb_kmeans->isOn())
	{
		pb_saveborders->hide();
		pb_loadborders->hide();
		hbox1->show();
		if (hideparams)
		{
			pb_saveborders->hide();
			pb_loadborders->hide();
			hbox7->hide();
			hbox5->hide();
		}
		else
		{
			hbox7->show();
			hbox5->show();
		}
		dim_changed(sb_dim->value());
		hbox3->hide();
		hbox4->hide();
		txt_slider->setText("Weight: ");
		txt_tissuenr->setText("Image-Nr: ");
		txt_lower->hide();
		txt_upper->hide();
		le_borderval->hide();
		//		pushrange->hide();
	}
	else
	{
		pb_saveborders->hide();
		pb_loadborders->hide();
		hbox1->show();
		if (hideparams)
		{
			pb_saveborders->hide();
			pb_loadborders->hide();
			hbox7->hide();
			hbox5->hide();
		}
		else
		{
			hbox7->show();
			hbox5->show();
		}
		dim_changed(sb_dim->value());
		hbox3->hide();
		hbox4->hide();
		txt_slider->setText("Weight: ");
		txt_tissuenr->setText("Image-Nr: ");
		txt_lower->hide();
		txt_upper->hide();
		le_borderval->hide();
		//		pushrange->hide();
	}
}

void ThresholdWidget::getrange()
{
	Pair p;
	bmphand->get_bmprange(&p);
	lower = p.low;
	upper = p.high;

	txt_lower->setText(QString::number(lower, 'g', 3));
	txt_upper->setText(QString::number(upper, 'g', 3) + QString(" "));

	for (unsigned i = 0; i < 20; i++)
	{
		if (threshs[i + 1] > upper)
			threshs[i + 1] = upper;
		if (threshs[i + 1] < lower)
			threshs[i + 1] = lower;
	}

	if (rb_manual->isOn())
	{
		int i = int((threshs[sb_tissuenr->value()] - lower) * 200 /
								(upper - lower));
		slider->setValue(i);
		le_borderval->setText(
				QString::number(threshs[sb_tissuenr->value()], 'g', 3));
	}

	return;
}

void ThresholdWidget::on_tissuenr_changed(int newval)
{
	if (rb_manual->isOn())
	{
		slider->setValue(
				int(200 * (threshs[newval] - lower) / (upper - lower)));
		le_borderval->setText(QString::number(threshs[newval], 'g', 3));
	}
	else if (rb_kmeans->isOn() || rb_EM->isOn())
	{
		slider->setValue(int(200 * weights[newval - 1]));
	}

	if (newval > 1 && (rb_kmeans->isOn() || rb_EM->isOn()))
	{
		hboxfilenames->show();
		le_filename->setText(filenames[newval - 2]);
	}
	else
		hboxfilenames->hide();

	return;
}

void ThresholdWidget::nrtissues_changed(int newval)
{
	if (rb_manual->isOn())
	{
		threshs[0] = float(newval - 1);
		sb_tissuenr->setMaxValue(newval - 1);
		if (sb_tissuenr->value() > 0)
			sb_tissuenr->setValue(sb_tissuenr->value());
		else
			sb_tissuenr->setValue(1);
		slider->setValue(int(200 * (threshs[1] - lower) / (upper - lower)));
		le_borderval->setText(QString::number(threshs[1], 'g', 3));
		if (newval == 2)
		{
			hbox2->hide();
		}
		else
		{
			hbox2->show();
		}
	}

	return;
}

void ThresholdWidget::dim_changed(int newval)
{
	if (rb_kmeans->isOn() || rb_EM->isOn())
	{
		size_t cursize = filenames.size();
		if (newval > cursize + 1)
		{
			filenames.resize(newval - 1);
			for (size_t i = cursize; i + 1 < newval; i++)
				filenames[i] = QString("");
		}

		sb_tissuenr->setMaxValue(newval);
		sb_tissuenr->setValue(1);
		slider->setValue(int(200 * weights[0]));
		if (newval == 1)
		{
			//			sb_tissuenr->hide();
			//			txt_tissuenr->hide();
			hbox2a->hide();
			hbox2->hide();
		}
		else
		{
			sb_tissuenr->show();
			txt_tissuenr->show();
			if (hideparams)
			{
				hbox2a->hide();
				hbox2->hide();
			}
			else
			{
				hbox2->show();
				hbox2a->show();
			}
		}
	}

	return;
}

void ThresholdWidget::subsect_toggled()
{
	if (subsect->isOn())
	{
		hbox8->show();
	}
	else
	{
		hbox8->hide();
	}

	return;
}

void ThresholdWidget::slider_changed(int newval)
{
	if (rb_manual->isOn())
	{
		threshs[sb_tissuenr->value()] =
				newval * 0.005f * (upper - lower) + lower;
		le_borderval->setText(
				QString::number(threshs[sb_tissuenr->value()], 'g', 3));

		if (allslices->isChecked())
			handler3D->threshold(threshs);
		else
			bmphand->threshold(threshs);

		emit end_datachange(this, iseg::NoUndo);
	}
	else if (rb_kmeans->isOn() || rb_EM->isOn())
	{
		weights[sb_tissuenr->value() - 1] = newval * 0.005f;
	}

	return;
}

void ThresholdWidget::slider_pressed()
{
	if (rb_manual->isOn())
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = allslices->isChecked();
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);
	}
}

void ThresholdWidget::slider_released()
{
	if (rb_manual->isOn())
	{
		emit end_datachange(this);
	}
}

void ThresholdWidget::bmp_changed()
{
	bmphand = handler3D->get_activebmphandler();
	getrange();

	return;
}

QSize ThresholdWidget::sizeHint() const { return vbox1->sizeHint(); }

void ThresholdWidget::on_slicenr_changed()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
	//	}
}

void ThresholdWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	getrange();

	return;
}

void ThresholdWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	else
		getrange();
	hideparams_changed();
}

void ThresholdWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void ThresholdWidget::le_borderval_returnpressed()
{
	bool b1;
	float val = le_borderval->text().toFloat(&b1);
	if (b1)
	{
		if (rb_manual->isOn())
		{
			if (val > upper)
				threshs[sb_tissuenr->value()] = upper;
			else if (val < lower)
				threshs[sb_tissuenr->value()] = lower;
			else
				threshs[sb_tissuenr->value()] = val;

			int i = int((threshs[sb_tissuenr->value()] - lower) * 200 /
									(upper - lower));
			slider->setValue(i);
			le_borderval->setText(
					QString::number(threshs[sb_tissuenr->value()], 'g', 3));

			iseg::DataSelection dataSelection;
			dataSelection.allSlices = allslices->isChecked();
			dataSelection.sliceNr = handler3D->active_slice();
			dataSelection.work = true;
			emit begin_datachange(dataSelection, this);

			if (allslices->isChecked())
				handler3D->threshold(threshs);
			else
				bmphand->threshold(threshs);

			emit end_datachange(this);
		}
	}
	else
	{
		QApplication::beep();
		le_borderval->setText(
				QString::number(threshs[sb_tissuenr->value()], 'g', 3));
	}
}

void ThresholdWidget::saveborders_execute()
{
	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Boarders (*.txt)\n", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		FILE* fp = fopen(savefilename.ascii(), "w");
		for (int i = 1; i <= sb_tissuenr->maxValue(); i++)
			fprintf(fp, "%f\n", threshs[i]);
		fclose(fp);
	}
}

void ThresholdWidget::loadborders_execute()
{
	if (rb_manual->isOn())
	{
		QString loadfilename =
				QFileDialog::getOpenFileName(QString::null,
						"Boarders (*.txt)\n"
						"All (*.*)",
						this);

		if (!loadfilename.isEmpty())
		{
			std::vector<float> fvec;
			FILE* fp = fopen(loadfilename.ascii(), "r");
			float f;
			if (fp != nullptr)
			{
				while (fscanf(fp, "%f", &f) == 1)
				{
					fvec.push_back(f);
				}
			}
			fclose(fp);

			if (fvec.size() > 0 && fvec.size() < 20)
			{
				sb_nrtissues->setValue(fvec.size() + 1);
				threshs[0] = float(fvec.size());
				sb_tissuenr->setMaxValue(fvec.size());
				for (unsigned i = 0; i < (unsigned)fvec.size(); i++)
				{
					if (fvec[i] > upper)
						f = upper;
					else if (fvec[i] < lower)
						f = lower;
					else
						f = fvec[i];
					threshs[i + 1] = f;
				}
				sb_tissuenr->setValue(1);
				slider->setValue(
						int(200 * (threshs[1] - lower) / (upper - lower)));
				le_borderval->setText(QString::number(threshs[1], 'g', 3));
			}
		}
	}
}

FILE* ThresholdWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = slider->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ratio->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_nrtissues->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_dim->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_tissuenr->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_px->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_py->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_lx->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_ly->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_iternr->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_converge->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_minpix->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(subsect->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_manual->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_histo->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_kmeans->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_EM->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allslices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&upper, 1, sizeof(float), fp);
		fwrite(&lower, 1, sizeof(float), fp);
		fwrite(threshs, 21, sizeof(float), fp);
		fwrite(weights, 20, sizeof(float), fp);
	}

	return fp;
}

FILE* ThresholdWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(sb_nrtissues, SIGNAL(valueChanged(int)), this,
				SLOT(nrtissues_changed(int)));
		QObject::disconnect(sb_dim, SIGNAL(valueChanged(int)), this,
				SLOT(dim_changed(int)));
		QObject::disconnect(sb_tissuenr, SIGNAL(valueChanged(int)), this,
				SLOT(tissuenr_changed(int)));
		QObject::disconnect(slider, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed(int)));
		QObject::disconnect(le_borderval, SIGNAL(returnPressed()), this,
				SLOT(le_borderval_returnpressed()));
		QObject::disconnect(subsect, SIGNAL(clicked()), this,
				SLOT(subsect_toggled()));
		QObject::disconnect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		slider->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ratio->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_nrtissues->setValue(dummy);
		sb_tissuenr->setMaxValue(dummy - 1);
		fread(&dummy, sizeof(int), 1, fp);
		sb_dim->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_tissuenr->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_px->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_py->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_lx->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_ly->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_iternr->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_converge->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_minpix->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		subsect->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_manual->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_histo->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_kmeans->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_EM->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allslices->setChecked(dummy > 0);

		fread(&upper, sizeof(float), 1, fp);
		fread(&lower, sizeof(float), 1, fp);
		fread(threshs, sizeof(float), 21, fp);
		fread(weights, sizeof(float), 20, fp);

		dummy = sb_tissuenr->value();
		method_changed(0);
		subsect_toggled();
		nrtissues_changed(sb_nrtissues->value());
		dim_changed(sb_dim->value());
		sb_tissuenr->setValue(dummy);
		on_tissuenr_changed(dummy);

		QObject::connect(subsect, SIGNAL(clicked()), this,
				SLOT(subsect_toggled()));
		QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed(int)));
		QObject::connect(sb_nrtissues, SIGNAL(valueChanged(int)), this,
				SLOT(nrtissues_changed(int)));
		QObject::connect(sb_dim, SIGNAL(valueChanged(int)), this,
				SLOT(dim_changed(int)));
		QObject::connect(sb_tissuenr, SIGNAL(valueChanged(int)), this,
				SLOT(tissuenr_changed(int)));
		QObject::connect(slider, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed(int)));
		QObject::connect(le_borderval, SIGNAL(returnPressed()), this,
				SLOT(le_borderval_returnpressed()));
	}
	return fp;
}

void ThresholdWidget::hideparams_changed() { method_changed(0); }

void ThresholdWidget::select_pushed()
{
	QString loadfilename = QFileDialog::getOpenFileName(
			QString::null,
			"Images (*.png)\n"
			"Images (*.mhd)\n"
			"All (*.*)", //"Images (*.bmp)\n" "All (*.*)", QString::null,
			this);			//, filename);
	le_filename->setText(loadfilename);
	filenames[sb_tissuenr->value() - 2] = loadfilename;

	buttonR->setVisible(false);
	buttonG->setVisible(false);
	buttonB->setVisible(false);
	buttonA->setVisible(false);

	QFileInfo fi(loadfilename);
	QString ext = fi.extension();
	if (ext == "png")
	{
		QImage image(loadfilename);

		if (image.depth() > 8)
		{
			buttonR->setVisible(true);
			buttonG->setVisible(true);
			buttonB->setVisible(true);
			if (image.hasAlphaChannel())
				buttonA->setVisible(true);
		}
	}

	return;
}

void ThresholdWidget::selectCenterFile_pushed()
{
	centerFilename = QFileDialog::getOpenFileName(QString::null,
			"Text File (*.txt*)", this);
	le_centerFilename->setText(centerFilename);
}

void ThresholdWidget::RGBA_changed(int change)
{
	int buttonsChecked = 0;
	buttonsChecked = buttonR->isChecked() + buttonG->isChecked() +
									 buttonB->isChecked() + buttonA->isChecked();
	sb_dim->setValue(buttonsChecked + 1);

	if (buttonsChecked > 0)
		sb_tissuenr->setValue(2);
}

void ThresholdWidget::useCenterFile_changed(int change)
{
	if (change > 0)
	{
		sb_nrtissues->setEnabled(false);
		sb_dim->setEnabled(false);

		le_centerFilename->setVisible(true);
		pushcenterFilename->setVisible(true);
	}
	else
	{
		sb_nrtissues->setEnabled(true);
		sb_dim->setEnabled(true);

		centerFilename = "";

		le_centerFilename->setVisible(false);
		pushcenterFilename->setVisible(false);
	}
}
