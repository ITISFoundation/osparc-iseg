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

#include "AtlasWidget.h"

#include "Core/ProjectVersion.h"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <algorithm>
#include <q3popupmenu.h>
#include <qapplication.h>
#include <qcolor.h>
#include <qevent.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpen.h>
#include <qwidget.h>

using namespace iseg;

AtlasWidget::AtlasWidget(const char *filename, QDir picpath, QWidget *parent,
		const char *name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	isOK = false;
	QString title("Atlas - ");
	title = title + QFileInfo(filename).completeBaseName();
	setCaption(title);
	tissue = nullptr;
	image = nullptr;
	if (!loadfile(filename))
	{
		return;
	}
	isOK = true;
	m_picpath = picpath;

	QVBoxLayout *vbox1 = new QVBoxLayout;
	QHBoxLayout *hbox1 = new QHBoxLayout;
	QHBoxLayout *hbox2 = new QHBoxLayout;
	QHBoxLayout *hbox3 = new QHBoxLayout;

	sl_contrast = new QSlider(Qt::Horizontal, this);
	sl_contrast->setRange(0, 99);
	sl_contrast->setValue(0);
	sl_brightness = new QSlider(Qt::Horizontal, this);
	sl_brightness->setRange(0, 100);
	sl_brightness->setValue(50);
	lb_contrast = new QLabel("C:", this);
	lb_contrast->setPixmap(
			QIcon(m_picpath.absFilePath(QString("icon-contrast.png")).ascii())
					.pixmap());
	lb_brightness = new QLabel("B:", this);
	lb_brightness->setPixmap(
			QIcon(m_picpath.absFilePath(QString("icon-brightness.png")).ascii())
					.pixmap());
	hbox1->addWidget(lb_contrast);
	hbox1->addWidget(sl_contrast);
	hbox1->addWidget(lb_brightness);
	hbox1->addWidget(sl_brightness);

	vbox1->setSpacing(0);
	vbox1->setMargin(2);
	vbox1->addLayout(hbox1);
	sa_viewer = new QScrollArea(this);
	atlasViewer = new AtlasViewer(image, tissue, 2, dimx, dimy, dimz, dx, dy,
			dz, &color_r, &color_g, &color_b, this);
	sa_viewer->setWidget(atlasViewer);
	vbox1->addWidget(sa_viewer);
	scb_slicenr = new QScrollBar(0, dimz - 1, 1, 5, 0, Qt::Horizontal, this);
	vbox1->addWidget(scb_slicenr);

	bg_orient = new QButtonGroup(this);
	//	imgorval->hide();
	rb_x = new QRadioButton(QString("x"), this);
	rb_y = new QRadioButton(QString("y"), this);
	rb_z = new QRadioButton(QString("z"), this);
	bg_orient->insert(rb_x);
	bg_orient->insert(rb_y);
	bg_orient->insert(rb_z);
	rb_z->setChecked(TRUE);
	hbox2->addWidget(rb_x);
	hbox2->addWidget(rb_y);
	hbox2->addWidget(rb_z);

	sl_transp = new QSlider(Qt::Horizontal, this);
	sl_transp->setRange(0, 100);
	sl_transp->setValue(50);
	lb_transp = new QLabel("Transp:", this);
	hbox3->addWidget(lb_transp);
	hbox3->addWidget(sl_transp);

	vbox1->addLayout(hbox2);
	vbox1->addLayout(hbox3);
	zoomer = new ZoomWidget(1.0, m_picpath, this);
	zoomer->setFixedSize(zoomer->sizeHint());
	vbox1->addWidget(zoomer);
	lb_name = new QLabel(QString("jljlfds"), this);
	vbox1->addWidget(lb_name);
	setLayout(vbox1);

	QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this,
			SLOT(scb_slicenr_changed()));
	QObject::connect(sl_transp, SIGNAL(valueChanged(int)), this,
			SLOT(sl_transp_changed()));
	QObject::connect(bg_orient, SIGNAL(buttonClicked(int)), this,
			SLOT(xyz_changed()));
	QObject::connect(zoomer, SIGNAL(set_zoom(double)), atlasViewer,
			SLOT(set_zoom(double)));
	QObject::connect(sl_brightness, SIGNAL(valueChanged(int)), this,
			SLOT(sl_brightcontr_moved()));
	QObject::connect(sl_contrast, SIGNAL(valueChanged(int)), this,
			SLOT(sl_brightcontr_moved()));

	QObject::connect(atlasViewer, SIGNAL(mousemoved_sign(tissues_size_t)), this,
			SLOT(pt_moved(tissues_size_t)));
	atlasViewer->setMouseTracking(true);
}

AtlasWidget::~AtlasWidget()
{
	delete[] tissue;
	delete[] image;
}

bool AtlasWidget::loadfile(const char *filename)
{
	dimx = dimy = dimz = 10;
	dx = dy = dz = 1.0;

	QFile file(filename);
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);

	// Read and check the header
	quint32 magic;
	in >> magic;
	if (magic != 0xD0C0B0A0)
		return false;

	// Read the version
	qint32 combinedVersion;
	in >> combinedVersion;
	int version, tissuesVersion;
	iseg::ExtractTissuesVersion((int)combinedVersion, version, tissuesVersion);
	if (version < 1)
		return false;
	if (version > 1)
		return false;

	in.setVersion(QDataStream::Qt_4_0);

	qint32 dummy;
	in >> dummy;
	dimx = (unsigned short)dummy;
	in >> dummy;
	dimy = (unsigned short)dummy;
	in >> dummy;
	dimz = (unsigned short)dummy;
	in >> dx >> dy >> dz;
	in >> dummy;
	tissues_size_t nrtissues = (tissues_size_t)dummy;
	tissue_names.resize(nrtissues);
	color_r.resize(nrtissues);
	color_g.resize(nrtissues);
	color_b.resize(nrtissues);

	unsigned dimtot = unsigned(dimx) * unsigned(dimy) * dimz;
	image = new float[dimtot];
	if (image == nullptr)
		return false;
	tissue = new tissues_size_t[dimtot];
	if (tissue == nullptr)
	{
		delete[] image;
		return false;
	}

	for (tissues_size_t i = 0; i < nrtissues; i++)
	{
		in >> tissue_names[i] >> color_r[i] >> color_g[i] >> color_b[i];
	}

	int area = dimx * (int)dimy;
	if (tissuesVersion > 0)
	{
		for (unsigned short i = 0; i < dimz; i++)
		{
			in.readRawData((char *)&(image[area * i]), area * sizeof(float));
			in.readRawData((char *)&(tissue[area * i]),
					area * sizeof(tissues_size_t));
		}
	}
	else
	{
		char *charBuffer = new char[area];
		for (unsigned short i = 0; i < dimz; i++)
		{
			in.readRawData((char *)&(image[area * i]), area * sizeof(float));
			in.readRawData(charBuffer, area);
			for (int j = 0; j < area; j++)
			{
				tissue[area * i + j] = charBuffer[j];
			}
		}
		delete[] charBuffer;
	}

	//for(unsigned i=0;i<dimtot;i++) image[i]=10*(i%10);
	//for(unsigned i=0;i<dimtot;i++) tissue[i]=((i/10)%10);

	minval = maxval = image[0];
	for (unsigned i = 1; i < dimtot; i++)
	{
		if (minval > image[i])
			minval = image[i];
		if (maxval < image[i])
			maxval = image[i];
	}

	//tissue_names.push_back(QString("a"));
	//tissue_names.push_back(QString("b"));
	//tissue_names.push_back(QString("c"));
	//tissue_names.push_back(QString("d"));
	//tissue_names.push_back(QString("e"));
	//tissue_names.push_back(QString("f"));
	//tissue_names.push_back(QString("g"));
	//tissue_names.push_back(QString("h"));
	//tissue_names.push_back(QString("i"));
	//tissue_names.push_back(QString("j"));

	//color_r.push_back(0.1f);
	//color_r.push_back(0.2f);
	//color_r.push_back(0.3f);
	//color_r.push_back(0.4f);
	//color_r.push_back(0.5f);
	//color_r.push_back(0.6f);
	//color_r.push_back(0.7f);
	//color_r.push_back(0.8f);
	//color_r.push_back(0.9f);

	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);

	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);

	return true;
}

void AtlasWidget::scb_slicenr_changed()
{
	atlasViewer->slicenr_changed(scb_slicenr->value());
}

void AtlasWidget::sl_transp_changed()
{
	atlasViewer->set_tissueopac(1.0f - sl_transp->value() * 0.01f);
}

void AtlasWidget::xyz_changed()
{
	scb_slicenr->setValue(0);
	if (rb_x->isOn())
	{
		scb_slicenr->setMaxValue(dimx - 1);
		atlasViewer->orient_changed(0);
	}
	else if (rb_y->isOn())
	{
		scb_slicenr->setMaxValue(dimy - 1);
		atlasViewer->orient_changed(1);
	}
	else if (rb_z->isOn())
	{
		scb_slicenr->setMaxValue(dimz - 1);
		atlasViewer->orient_changed(2);
	}
}

void AtlasWidget::pt_moved(tissues_size_t val)
{
	if (val == 0)
		lb_name->setText("Background");
	else
		lb_name->setText(tissue_names[val - 1]);
}

void AtlasWidget::sl_brightcontr_moved()
{
	float factor, offset;
	factor = 255.0f * (1 + sl_contrast->value()) / (maxval - minval);
	offset =
			(127.5f - maxval * factor) * (1.0f - sl_brightness->value() * 0.01f) +
			0.01f * (127.5f - minval * factor) * sl_brightness->value();
	atlasViewer->set_scale(offset, factor);
}
