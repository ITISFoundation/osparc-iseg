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

#include "SliceViewerWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QCloseEvent>
#include <QPaintEvent>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcolor.h>
#include <qevent.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpen.h>
#include <qradiobutton.h>
#include <qwidget.h>

#include <algorithm>

using namespace std;
using namespace iseg;

bmptissuesliceshower::bmptissuesliceshower(
		SlicesHandler* hand3D, unsigned short slicenr1, float thickness1,
		float zoom1, bool orientation, bool bmpon, bool tissuevisible1,
		bool zposvisible1, bool xyposvisible1, int xypos1, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), tissuevisible(tissuevisible1),
			handler3D(hand3D), slicenr(slicenr1), directionx(orientation),
			bmporwork(bmpon), thickness(thickness1), zposvisible(zposvisible1),
			xyposvisible(xyposvisible1), xypos(xypos1), zoom(zoom1)
{
	if (directionx)
	{
		if (bmporwork)
			bmpbits = handler3D->slicebmp_x(slicenr);
		else
			bmpbits = handler3D->slicework_x(slicenr);
		tissue = handler3D->slicetissue_x(slicenr);
		width = handler3D->height();
		height = handler3D->num_slices();
		d = handler3D->get_pixelsize().low;
	}
	else
	{
		if (bmporwork)
			bmpbits = handler3D->slicebmp_y(slicenr);
		else
			bmpbits = handler3D->slicework_y(slicenr);
		tissue = handler3D->slicetissue_y(slicenr);
		width = handler3D->width();
		height = handler3D->num_slices();
		d = handler3D->get_pixelsize().high;
	}

	scalefactorbmp = 1.0f;
	scaleoffsetbmp = 0.0f;
	scalefactorwork = 1.0f;
	scaleoffsetwork = 0.0f;

	image.create(int(width), int(height), 32);

	setFixedSize((int)(width * d * zoom), (int)(height * thickness * zoom));
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	show();

	reload_bits();
}

void bmptissuesliceshower::paintEvent(QPaintEvent* e)
{
	if (image.size() != QSize(0, 0))
	{ // is an image loaded?
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.scale(d * zoom, thickness * zoom);
		painter.drawImage(0, 0, image);
	}
}

void bmptissuesliceshower::set_bmporwork(bool bmpon)
{
	if (bmpon != bmporwork)
	{
		bmporwork = bmpon;
		update();
	}
}

void bmptissuesliceshower::bmp_changed()
{
	if (bmporwork)
	{
		update();
	}
}

void bmptissuesliceshower::work_changed()
{
	if (!bmporwork)
	{
		update();
	}
}

void bmptissuesliceshower::update()
{
	unsigned short w, h;

	if (directionx)
	{
		w = handler3D->height();
		h = handler3D->num_slices();
	}
	else
	{
		w = handler3D->width();
		h = handler3D->num_slices();
	}

	if (w != width || h != height)
	{
		width = w;
		height = h;
		image.create(int(w), int(h), 32);
		setFixedSize((int)(w * d * zoom), (int)(h * thickness * zoom));
		free(bmpbits);
		free(tissue);
		if (directionx)
		{
			if (bmporwork)
				bmpbits = handler3D->slicebmp_x(slicenr);
			else
				bmpbits = handler3D->slicework_x(slicenr);
			tissue = handler3D->slicetissue_x(slicenr);
		}
		else
		{
			if (bmporwork)
				bmpbits = handler3D->slicebmp_y(slicenr);
			else
				bmpbits = handler3D->slicework_y(slicenr);
			tissue = handler3D->slicetissue_y(slicenr);
		}
	}
	else
	{
		if (directionx)
		{
			if (bmporwork)
				handler3D->slicebmp_x(bmpbits, slicenr);
			else
				handler3D->slicework_x(bmpbits, slicenr);
			handler3D->slicetissue_x(tissue, slicenr);
		}
		else
		{
			if (bmporwork)
				handler3D->slicebmp_y(bmpbits, slicenr);
			else
				handler3D->slicework_y(bmpbits, slicenr);
			handler3D->slicetissue_y(tissue, slicenr);
		}
	}

	reload_bits();
	repaint();
}

void bmptissuesliceshower::reload_bits()
{
	unsigned pos = 0;
	int f;
	if (tissuevisible)
	{
		float scaleoffset, scalefactor;
		if (bmporwork)
			scaleoffset = scaleoffsetbmp;
		else
			scaleoffset = scaleoffsetwork;
		if (bmporwork)
			scalefactor = scalefactorbmp;
		else
			scalefactor = scalefactorwork;
		unsigned char r, g, b;
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				f = (int)max(0.0f, min(255.0f, scaleoffset + scalefactor * (bmpbits)[pos]));
				if (tissue[pos] == 0)
				{
					image.setPixel(x, y, qRgb(int(f), int(f), int(f)));
				}
				else
				{
					TissueInfos::GetTissueColorBlendedRGB(tissue[pos], r, g, b, f);
					image.setPixel(x, y, qRgb(r, g, b));
				}
				pos++;
			}
		}
	}
	else
	{
		float scaleoffset, scalefactor;
		if (bmporwork)
			scaleoffset = scaleoffsetbmp;
		else
			scaleoffset = scaleoffsetwork;
		if (bmporwork)
			scalefactor = scalefactorbmp;
		else
			scalefactor = scalefactorwork;
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				f = (int)max(0.0f, min(255.0f, scaleoffset + scalefactor * (bmpbits)[pos]));
				image.setPixel(x, y, qRgb(f, f, f));
				pos++;
			}
		}
	}

	if (zposvisible)
	{
		for (int x = 0; x < width; x++)
		{
			image.setPixel(x, handler3D->active_slice(), qRgb(0, 255, 0));
		}
	}

	if (xyposvisible)
	{
		for (int y = 0; y < height; y++)
		{
			image.setPixel(xypos, y, qRgb(0, 255, 0));
		}
	}
}

void bmptissuesliceshower::set_scale(float offset1, float factor1, bool bmporwork1)
{
	if (bmporwork1)
	{
		scalefactorbmp = factor1;
		scaleoffsetbmp = offset1;
	}
	else
	{
		scalefactorwork = factor1;
		scaleoffsetwork = offset1;
	}

	if (bmporwork == bmporwork1)
	{
		reload_bits();
		repaint();
	}
}

void bmptissuesliceshower::set_scaleoffset(float offset1, bool bmporwork1)
{
	if (bmporwork1)
		scaleoffsetbmp = offset1;
	else
		scaleoffsetwork = offset1;

	if (bmporwork == bmporwork1)
	{
		reload_bits();
		repaint();
	}
}

void bmptissuesliceshower::set_scalefactor(float factor1, bool bmporwork1)
{
	if (bmporwork1)
		scalefactorbmp = factor1;
	else
		scalefactorwork = factor1;

	if (bmporwork == bmporwork1)
	{
		reload_bits();
		repaint();
	}
}

void bmptissuesliceshower::tissue_changed()
{
	if (directionx)
	{
		handler3D->slicetissue_x(tissue, slicenr);
	}
	else
	{
		handler3D->slicetissue_y(tissue, slicenr);
	}
	reload_bits();
	repaint();
}

void bmptissuesliceshower::set_tissuevisible(bool on)
{
	tissuevisible = on;
	update();
}

void bmptissuesliceshower::slicenr_changed(int i)
{
	slicenr = (unsigned short)i;
	update();
}

void bmptissuesliceshower::thickness_changed(float thickness1)
{
	if (thickness != thickness1)
	{
		thickness = thickness1;

		setFixedSize((int)(width * d * zoom), (int)(height * thickness * zoom));

		repaint();
	}
}

void bmptissuesliceshower::pixelsize_changed(Pair pixelsize1)
{
	float d1;
	if (directionx)
		d1 = pixelsize1.low;
	else
		d1 = pixelsize1.high;

	if (d1 != d)
	{
		d = d1;

		setFixedSize((int)(width * d * zoom), (int)(height * thickness * zoom));

		repaint();

		if (directionx)
		{
			setFixedSize((int)(width * d * zoom), (int)(height * thickness * zoom));
		}
		else
		{
			setFixedSize((int)(width * d * zoom), (int)(height * thickness * zoom));
		}
	}
}
void bmptissuesliceshower::set_zposvisible(bool on)
{
	zposvisible = on;
	reload_bits();
	repaint();
}

void bmptissuesliceshower::set_xyposvisible(bool on)
{
	xyposvisible = on;

	reload_bits();
	repaint();
}

void bmptissuesliceshower::zpos_changed()
{
	reload_bits();
	repaint();
}

void bmptissuesliceshower::xypos_changed(int i)
{
	if (i < width)
		xypos = i;
	reload_bits();
	repaint();
}

void bmptissuesliceshower::set_zoom(double z)
{
	if (z != zoom)
	{
		zoom = z;
		setFixedSize((int)(width * d * zoom), (int)(height * thickness * zoom));
		repaint();
	}
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

SliceViewerWidget::SliceViewerWidget(SlicesHandler* hand3D, bool orientation,
		float thickness1, float zoom1,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), handler3D(hand3D), directionx(orientation),
			xyexists(false)
{
	if (directionx)
		nrslices = handler3D->width();
	else
		nrslices = handler3D->height();

	vbox = new Q3VBoxLayout(this);

	scroller = new Q3ScrollView(this);
	shower = new bmptissuesliceshower(handler3D, 0, thickness1, zoom1, directionx, true, true, false, false, 0, this);

	vbox->addWidget(scroller);
	scroller->addChild(shower);

	hbox = new Q3HBoxLayout(this);
	vbox->addLayout(hbox);
	hbox2 = new Q3HBoxLayout(this);
	vbox->addLayout(hbox2);

	cb_tissuevisible = new QCheckBox("Show tissues", this);
	hbox->addWidget(cb_tissuevisible);
	cb_tissuevisible->setChecked(true);

	rb_bmp = new QRadioButton("Source", this);
	hbox->addWidget(rb_bmp);
	rb_work = new QRadioButton("Target", this);
	hbox->addWidget(rb_work);

	bg_bmporwork = new QButtonGroup(this);
	bg_bmporwork->insert(rb_bmp);
	bg_bmporwork->insert(rb_work);

	cb_zposvisible = new QCheckBox("Show zpos", this);
	hbox2->addWidget(cb_zposvisible);
	cb_zposvisible->setChecked(false);
	cb_xyposvisible = new QCheckBox("Show xypos", this);
	hbox2->addWidget(cb_xyposvisible);
	cb_xyposvisible->setChecked(false);
	cb_xyposvisible->setEnabled(false);

	rb_bmp->setChecked(TRUE);

	qsb_slicenr = new QScrollBar(1, nrslices, 1, 5, 1, Qt::Horizontal, this);
	vbox->addWidget(qsb_slicenr);

	connect(qsb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(slicenr_changed(int)));
	connect(cb_tissuevisible, SIGNAL(clicked()), this, SLOT(tissuevisible_changed()));
	connect(bg_bmporwork, SIGNAL(buttonClicked(int)), this, SLOT(workorbmp_changed()));

	connect(cb_xyposvisible, SIGNAL(clicked()), this, SLOT(xyposvisible_changed()));
	connect(cb_zposvisible, SIGNAL(clicked()), this, SLOT(zposvisible_changed()));

	show();
}

SliceViewerWidget::~SliceViewerWidget()
{
	delete vbox;
	delete shower;
	delete scroller;
	delete cb_tissuevisible;
	delete cb_zposvisible;
	delete cb_xyposvisible;
	delete rb_bmp;
	delete rb_work;
	delete bg_bmporwork;
	delete qsb_slicenr;
}

void SliceViewerWidget::closeEvent(QCloseEvent* qce)
{
	emit hasbeenclosed();
	QWidget::closeEvent(qce);
}

void SliceViewerWidget::slicenr_changed(int i)
{
	shower->slicenr_changed(i - 1);
	emit slice_changed(i - 1);
}

int SliceViewerWidget::get_slicenr() { return qsb_slicenr->value() - 1; }

void SliceViewerWidget::bmp_changed()
{
	if (rb_bmp->isChecked())
	{
		unsigned short nrslicesnew;
		if (directionx)
			nrslicesnew = handler3D->width();
		else
			nrslicesnew = handler3D->height();

		if (nrslicesnew != nrslices)
		{
			nrslices = nrslicesnew;
			qsb_slicenr->setMaxValue((int)nrslices);
			qsb_slicenr->setValue(1);
			shower->slicenr_changed(0);
		}
		else
			shower->bmp_changed();

		qsb_slicenr->setFixedWidth(shower->size().width());
	}
}

void SliceViewerWidget::work_changed()
{
	if (rb_work->isChecked())
	{
		unsigned short nrslicesnew;
		if (directionx)
			nrslicesnew = handler3D->width();
		else
			nrslicesnew = handler3D->height();

		if (nrslicesnew != nrslices)
		{
			nrslices = nrslicesnew;
			qsb_slicenr->setMaxValue((int)nrslices);
			qsb_slicenr->setValue(1);
			shower->slicenr_changed(0);
		}
		else
			shower->bmp_changed();

		qsb_slicenr->setFixedWidth(shower->size().width());
	}
}

void SliceViewerWidget::tissue_changed() { shower->tissue_changed(); }

void SliceViewerWidget::tissuevisible_changed()
{
	shower->set_tissuevisible(cb_tissuevisible->isChecked());
}

void SliceViewerWidget::workorbmp_changed()
{
	if (rb_bmp->isChecked())
	{
		shower->set_bmporwork(true);
	}
	else
	{
		shower->set_bmporwork(false);
	}
}

void SliceViewerWidget::thickness_changed(float thickness1)
{
	shower->thickness_changed(thickness1);
}

void SliceViewerWidget::pixelsize_changed(Pair pixelsize1)
{
	shower->pixelsize_changed(pixelsize1);
}

void SliceViewerWidget::xyexists_changed(bool on)
{
	cb_xyposvisible->setEnabled(on);
	if (on)
	{
		shower->set_xyposvisible(cb_xyposvisible->isChecked());
	}
}

void SliceViewerWidget::zpos_changed() { shower->zpos_changed(); }

void SliceViewerWidget::xypos_changed(int i) { shower->xypos_changed(i); }

void SliceViewerWidget::xyposvisible_changed()
{
	shower->set_xyposvisible(cb_xyposvisible->isChecked());
}

void SliceViewerWidget::zposvisible_changed()
{
	shower->set_zposvisible(cb_zposvisible->isChecked());
}

void SliceViewerWidget::set_zoom(double z)
{
	shower->set_zoom(z);
}

void SliceViewerWidget::set_scale(float offset1, float factor1,
		bool bmporwork1)
{
	shower->set_scale(offset1, factor1, bmporwork1);
}

void SliceViewerWidget::set_scaleoffset(float offset1, bool bmporwork1)
{
	shower->set_scaleoffset(offset1, bmporwork1);
}

void SliceViewerWidget::set_scalefactor(float factor1, bool bmporwork1)
{
	shower->set_scalefactor(factor1, bmporwork1);
}
