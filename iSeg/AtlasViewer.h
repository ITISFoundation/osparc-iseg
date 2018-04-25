/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "Data/Point.h"

#include "Data/Types.h"

#include "Core/Pair.h"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QScrollArea>
#include <QWheelEvent>
#include <q3action.h>
#include <q3vbox.h>
#include <qcheckbox.h>
#include <qevent.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwidget.h>

#include <vector>

namespace iseg {

class AtlasViewer : public QWidget
{
	Q_OBJECT
public:
	AtlasViewer(float* bmpbits1, tissues_size_t* tissue1, unsigned char orient1,
			unsigned short dimx1, unsigned short dimy1,
			unsigned short dimz1, float dx1, float dy1, float dz1,
			std::vector<float>* r, std::vector<float>* g,
			std::vector<float>* b, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~AtlasViewer();
	void init();
	void update();
	void update(QRect rect);

signals:
	void mousemoved_sign(tissues_size_t t);

protected:
	void paintEvent(QPaintEvent* e);
	void mouseMoveEvent(QMouseEvent* e);

private:
	QPainter* painter;
	float scalefactor;
	float scaleoffset;
	double zoom;
	Pair pixelsize;
	void reload_bits();
	void get_slice();
	QImage image;
	unsigned char orient;

	unsigned short width, height, slicenr;
	unsigned short dimx, dimy, dimz;
	float dx, dy, dz;
	float tissueopac;
	float* bmpbits;
	tissues_size_t* tissue;
	float* current_bmpbits;
	tissues_size_t* current_tissue;
	std::vector<float>* color_r;
	std::vector<float>* color_g;
	std::vector<float>* color_b;

public slots:
	void set_scale(float offset1, float factor1);
	void set_scaleoffset(float offset1);
	void set_scalefactor(float factor1);
	void zoom_in();
	void zoom_out();
	void unzoom();
	double return_zoom();
	void set_zoom(double z);
	void slicenr_changed(unsigned short slicenr1);
	void orient_changed(unsigned char orient1);
	void set_tissueopac(float tissueopac1);
	void pixelsize_changed(Pair pixelsize1);
};

} // namespace iseg
