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

#include "AtlasViewer.h"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
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

#include <algorithm>

using namespace std;
using namespace iseg;

AtlasViewer::AtlasViewer(float* bmpbits1, tissues_size_t* tissue1,
						 unsigned char orient1, unsigned short dimx1,
						 unsigned short dimy1, unsigned short dimz1, float dx1,
						 float dy1, float dz1, std::vector<float>* r,
						 std::vector<float>* g, std::vector<float>* b,
						 QWidget* parent, const char* name,
						 Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags)
{
	setCursor(Qt::CrossCursor);
	scalefactor = 1.0f;
	scaleoffset = 0.0f;
	zoom = 1.0;
	tissueopac = 0.5;
	pixelsize.high = pixelsize.low = 1.0f;

	dimx = dimx1;
	dimy = dimy1;
	dimz = dimz1;

	dx = dx1;
	dy = dy1;
	dz = dz1;

	bmpbits = bmpbits1;
	tissue = tissue1;
	current_bmpbits = nullptr;
	current_tissue = nullptr;

	orient = orient1;

	slicenr = 0;

	color_r = r;
	color_g = g;
	color_b = b;

	init();

	return;
}

AtlasViewer::~AtlasViewer()
{
	delete[] current_bmpbits;
	delete[] current_tissue;
}

void AtlasViewer::set_zoom(double z)
{
	if (z != zoom)
	{
		zoom = z;
		setFixedSize((int)width * (zoom * pixelsize.high),
					 (int)height * (zoom * pixelsize.low));
		repaint();
	}
}

void AtlasViewer::paintEvent(QPaintEvent* e)
{
	if (image.size() != QSize(0, 0))
	{ // is an image loaded?
		{
			QPainter painter(this);
			painter.setClipRect(e->rect());

			painter.scale(zoom * pixelsize.high, zoom * pixelsize.low);
			painter.drawImage(0, 0, image);
		}
	}
}

void AtlasViewer::update()
{
	QRect rect;
	rect.setLeft(0);
	rect.setTop(0);
	rect.setRight(width - 1);
	rect.setBottom(height - 1);
	update(rect);
}

void AtlasViewer::update(QRect rect)
{
	unsigned short newwidth, newheight;
	float newdx, newdy;
	if (orient == 0)
	{
		newwidth = dimy;
		newheight = dimz;
		newdx = dy;
		newdy = dz;
	}
	else if (orient == 1)
	{
		newwidth = dimx;
		newheight = dimz;
		newdx = dx;
		newdy = dz;
	}
	else if (orient == 2)
	{
		newwidth = dimx;
		newheight = dimy;
		newdx = dx;
		newdy = dy;
	}

	if (newwidth != width || newheight != height || newdx != pixelsize.high ||
		newdy != pixelsize.low)
	{
		width = newwidth;
		height = newheight;
		pixelsize.high = newdx;
		pixelsize.low = newdy;
		delete[] current_bmpbits;
		delete[] current_tissue;
		current_bmpbits = new float[height * (unsigned)(width)];
		current_tissue = new tissues_size_t[height * (unsigned)(width)];
		image.create(int(width), int(height), 32);
		setFixedSize((int)width * zoom * pixelsize.high,
					 (int)height * zoom * pixelsize.low);
	}

	get_slice();

	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void AtlasViewer::init()
{
	if (orient == 0)
	{
		width = dimy;
		height = dimz;
		pixelsize.high = dy;
		pixelsize.low = dz;
	}
	else if (orient == 1)
	{
		width = dimx;
		height = dimz;
		pixelsize.high = dx;
		pixelsize.low = dz;
	}
	else if (orient == 2)
	{
		width = dimx;
		height = dimy;
		pixelsize.high = dx;
		pixelsize.low = dy;
	}

	image.create(int(width), int(height), 32);
	delete[] current_bmpbits;
	delete[] current_tissue;
	current_bmpbits = new float[height * (unsigned)(width)];
	current_tissue = new tissues_size_t[height * (unsigned)(width)];

	setFixedSize((int)width * zoom * pixelsize.high,
				 (int)height * zoom * pixelsize.low);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	get_slice();

	reload_bits();
	repaint();
	show();
	return;
}

void AtlasViewer::get_slice()
{
	if (orient == 0)
	{
		if (slicenr >= dimx)
			slicenr = dimx - 1;
		unsigned pos = 0;
		unsigned pos1 = unsigned(slicenr);
		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < width; j++, pos++, pos1 += dimx)
			{
				current_tissue[pos] = tissue[pos1];
				current_bmpbits[pos] = bmpbits[pos1];
			}
		}
	}
	else if (orient == 1)
	{
		if (slicenr >= dimy)
			slicenr = dimy - 1;
		unsigned pos = 0;
		unsigned pos1 = unsigned(slicenr) * width;
		for (unsigned short i = 0; i < height;
			 i++, pos1 += unsigned(dimy - 1) * dimx)
		{
			for (unsigned short j = 0; j < width; j++, pos++, pos1++)
			{
				current_tissue[pos] = tissue[pos1];
				current_bmpbits[pos] = bmpbits[pos1];
			}
		}
	}
	else if (orient == 2)
	{
		if (slicenr >= dimz)
			slicenr = dimz - 1;
		unsigned pos = 0;
		unsigned pos1 = unsigned(slicenr) * unsigned(dimy) * dimx;
		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < width; j++, pos++, pos1++)
			{
				current_tissue[pos] = tissue[pos1];
				current_bmpbits[pos] = bmpbits[pos1];
			}
		}
	}
}

void AtlasViewer::reload_bits()
{
	unsigned pos = 0;
	int f;
	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < width; x++)
		{
			f = (int)max(0.0f,
						 min(255.0f, scaleoffset +
										 scalefactor * (current_bmpbits)[pos]));
			if (current_tissue[pos] == 0)
				image.setPixel(x, y, qRgb(int(f), int(f), int(f)));
			else
				image.setPixel(
					x, y,
					qRgb(
						int(f + tissueopac *
									(255.0f *
										 ((*color_r)[current_tissue[pos] - 1]) -
									 f)),
						int(f + tissueopac *
									(255.0f *
										 ((*color_g)[current_tissue[pos] - 1]) -
									 f)),
						int(f + tissueopac *
									(255.0f *
										 ((*color_b)[current_tissue[pos] - 1]) -
									 f))));
			pos++;
		}
	}

	return;
}

void AtlasViewer::pixelsize_changed(Pair pixelsize1)
{
	if (pixelsize1.high != pixelsize.high || pixelsize1.low != pixelsize.low)
	{
		pixelsize = pixelsize1;
		setFixedSize((int)width * zoom * pixelsize.high,
					 (int)height * zoom * pixelsize.low);
		repaint();
	}
}

void AtlasViewer::zoom_in() { set_zoom(2 * zoom); }

void AtlasViewer::zoom_out() { set_zoom(0.5 * zoom); }

void AtlasViewer::unzoom() { set_zoom(1.0); }

double AtlasViewer::return_zoom() { return zoom; }

void AtlasViewer::set_scale(float offset1, float factor1)
{
	scalefactor = factor1;
	scaleoffset = offset1;

	reload_bits();
	repaint();
}

void AtlasViewer::set_scaleoffset(float offset1)
{
	scaleoffset = offset1;

	reload_bits();
	repaint();
}

void AtlasViewer::set_scalefactor(float factor1)
{
	scalefactor = factor1;

	reload_bits();
	repaint();
}

void AtlasViewer::mouseMoveEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)max(
		min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)max(
		min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))),
		0.0);

	unsigned pos = p.px + p.py * width;
	emit mousemoved_sign(current_tissue[pos]);
}

void AtlasViewer::slicenr_changed(unsigned short slicenr1)
{
	slicenr = slicenr1;
	if (orient == 0 && slicenr >= dimx)
		slicenr = dimx - 1;
	else if (orient == 1 && slicenr >= dimy)
		slicenr = dimy - 1;
	else if (orient == 2 && slicenr >= dimz)
		slicenr = dimz - 1;
	update();
}

void AtlasViewer::orient_changed(unsigned char orient1)
{
	orient = orient1;
	if (orient == 0 && slicenr >= dimx)
		slicenr = dimx - 1;
	else if (orient == 1 && slicenr >= dimy)
		slicenr = dimy - 1;
	else if (orient == 2 && slicenr >= dimz)
		slicenr = dimz - 1;
	update();
}

void AtlasViewer::set_tissueopac(float tissueopac1)
{
	tissueopac = tissueopac1;
	update();
}
