/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SLICESHOWER_29April05
#define SLICESHOWER_29April05

#include "SlicesHandler.h"

#include "Data/Point.h"

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QCloseEvent>
#include <QPaintEvent>
#include <q3action.h>
#include <q3hbox.h>
#include <q3scrollview.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qevent.h>
#include <qimage.h>
#include <qlayout.h>
#include <qpoint.h>
#include <qradiobutton.h>
#include <qscrollbar.h>
#include <qwidget.h>

#include <vector>

namespace iseg {

class bmptissuesliceshower : public QWidget
{
	Q_OBJECT
public:
	bmptissuesliceshower(SlicesHandler* hand3D, unsigned short slicenr1,
						 float thickness1, float zoom1, bool orientation,
						 bool bmpon, bool tissuevisible1, bool zposvisible1,
						 bool xyposvisible1, int xypos1, QWidget* parent = 0,
						 const char* name = 0, Qt::WindowFlags wFlags = 0);
	void update();
	void set_tissuevisible(bool on);
	void set_zposvisible(bool on);
	void set_xyposvisible(bool on);
	void set_bmporwork(bool bmpon);

protected:
	void paintEvent(QPaintEvent* e);

private:
	void reload_bits();
	QImage image;
	unsigned short width, height;
	unsigned short nrslices, slicenr;
	float* bmpbits;
	tissues_size_t* tissue;
	bool tissuevisible;
	bool zposvisible;
	bool xyposvisible;
	int xypos;
	bool directionx;
	bool bmporwork;
	SlicesHandler* handler3D;
	float thickness;
	float d;
	float zoom;
	float scalefactorbmp;
	float scaleoffsetbmp;
	float scalefactorwork;
	float scaleoffsetwork;
	//unsigned char mode;

public slots:
	void bmp_changed();
	void work_changed();
	void tissue_changed();
	void slicenr_changed(int i);
	void zpos_changed();
	void xypos_changed(int i);
	void thickness_changed(float thickness1);
	void pixelsize_changed(Pair pixelsize1);
	void set_zoom(double z);
	void set_scale(float offset1, float factor1, bool bmporwork1);
	void set_scaleoffset(float offset1, bool bmporwork1);
	void set_scalefactor(float factor1, bool bmporwork1);
	//void mode_changed(unsigned char newmode);
};

class SliceViewerWidget : public QWidget
{
	Q_OBJECT
public:
	SliceViewerWidget(SlicesHandler* hand3D, bool orientation,
					  float thickness1, float zoom1, QWidget* parent = 0,
					  const char* name = 0, Qt::WindowFlags wFlags = 0);
	~SliceViewerWidget();
	int get_slicenr();

protected:
	void closeEvent(QCloseEvent*);

private:
	Q3VBoxLayout* vbox;
	Q3HBoxLayout* hbox;
	Q3HBoxLayout* hbox2;
	//	QHBox *hbox1;
	QCheckBox* cb_tissuevisible;
	QCheckBox* cb_zposvisible;
	QCheckBox* cb_xyposvisible;
	bmptissuesliceshower* shower;
	QScrollBar* qsb_slicenr;
	unsigned short nrslices;
	bool tissuevisible;
	bool directionx;
	QRadioButton* rb_bmp;
	QRadioButton* rb_work;
	QButtonGroup* bg_bmporwork;
	Q3ScrollView* scroller;
	SlicesHandler* handler3D;
	//	float thickness;
	bool xyexists;

signals:
	void hasbeenclosed();
	void slice_changed(int i);

private slots:
	void workorbmp_changed();
	void xyposvisible_changed();
	void zposvisible_changed();
	void slicenr_changed(int i);
	void tissuevisible_changed();

public slots:
	void bmp_changed();
	void work_changed();
	void tissue_changed();
	void thickness_changed(float thickness1);
	void pixelsize_changed(Pair pixelsize1);
	void xyexists_changed(bool on);
	void zpos_changed();
	void xypos_changed(int i);
	void set_zoom(double z);
	void set_scale(float offset1, float factor1, bool bmporwork1);
	void set_scaleoffset(float offset1, bool bmporwork1);
	void set_scalefactor(float factor1, bool bmporwork1);
};

} // namespace iseg

#endif
