/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
#include <q3hbox.h>
#include <q3scrollview.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QEvent>
#include <QImage>
#include <qlayout.h>
#include <qpoint.h>
#include <QRadioButton>
#include <qscrollbar.h>
#include <QWidget>

#include <vector>

namespace iseg {

class Bmptissuesliceshower : public QWidget
{
	Q_OBJECT
public:
	Bmptissuesliceshower(SlicesHandler* hand3D, unsigned short slicenr1, float thickness1, float zoom1, bool orientation, bool bmpon, bool tissuevisible1, bool zposvisible1, bool xyposvisible1, int xypos1, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	void update();
	void SetTissuevisible(bool on);
	void SetZposvisible(bool on);
	void SetXyposvisible(bool on);
	void SetBmporwork(bool bmpon);

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	void ReloadBits();
	QImage m_Image;
	unsigned short m_Width, m_Height;
	unsigned short m_Nrslices, m_Slicenr;
	float* m_Bmpbits;
	tissues_size_t* m_Tissue;
	bool m_Tissuevisible;
	bool m_Zposvisible;
	bool m_Xyposvisible;
	int m_Xypos;
	bool m_Directionx;
	bool m_Bmporwork;
	SlicesHandler* m_Handler3D;
	float m_Thickness;
	float m_D;
	float m_Zoom;
	float m_Scalefactorbmp;
	float m_Scaleoffsetbmp;
	float m_Scalefactorwork;
	float m_Scaleoffsetwork;
	//unsigned char mode;

public slots:
	void BmpChanged();
	void WorkChanged();
	void TissueChanged();
	void SlicenrChanged(int i);
	void ZposChanged();
	void XyposChanged(int i);
	void ThicknessChanged(float thickness1);
	void PixelsizeChanged(Pair pixelsize1);
	void SetZoom(double z);
	void SetScale(float offset1, float factor1, bool bmporwork1);
	void SetScaleoffset(float offset1, bool bmporwork1);
	void SetScalefactor(float factor1, bool bmporwork1);
	//void mode_changed(unsigned char newmode);
};

class SliceViewerWidget : public QWidget
{
	Q_OBJECT
public:
	SliceViewerWidget(SlicesHandler* hand3D, bool orientation, float thickness1, float zoom1, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~SliceViewerWidget() override;
	int GetSlicenr();

protected:
	void closeEvent(QCloseEvent*) override;

private:
	Q3VBoxLayout* m_Vbox;
	Q3HBoxLayout* m_Hbox;
	Q3HBoxLayout* m_Hbox2;
	//	QHBox *hbox1;
	QCheckBox* m_CbTissuevisible;
	QCheckBox* m_CbZposvisible;
	QCheckBox* m_CbXyposvisible;
	Bmptissuesliceshower* m_Shower;
	QScrollBar* m_QsbSlicenr;
	unsigned short m_Nrslices;
	bool m_Tissuevisible;
	bool m_Directionx;
	QRadioButton* m_RbBmp;
	QRadioButton* m_RbWork;
	QButtonGroup* m_BgBmporwork;
	Q3ScrollView* m_Scroller;
	SlicesHandler* m_Handler3D;
	//	float thickness;
	bool m_Xyexists;

signals:
	void Hasbeenclosed();
	void SliceChanged(int i);

private slots:
	void WorkorbmpChanged();
	void XyposvisibleChanged();
	void ZposvisibleChanged();
	void SlicenrChanged(int i);
	void TissuevisibleChanged();

public slots:
	void BmpChanged();
	void WorkChanged();
	void TissueChanged();
	void ThicknessChanged(float thickness1);
	void PixelsizeChanged(Pair pixelsize1);
	void XyexistsChanged(bool on);
	void ZposChanged();
	void XyposChanged(int i);
	void SetZoom(double z);
	void SetScale(float offset1, float factor1, bool bmporwork1);
	void SetScaleoffset(float offset1, bool bmporwork1);
	void SetScalefactor(float factor1, bool bmporwork1);
};

} // namespace iseg

#endif
