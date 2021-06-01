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

#include "SliceViewerWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"

#include "Interface/QtConnect.h"

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QCloseEvent>
#include <QPaintEvent>
#include <QApplication>
#include <QButtonGroup>
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QRadioButton>
#include <QWidget>

#include <algorithm>

namespace iseg {

Bmptissuesliceshower::Bmptissuesliceshower(SlicesHandler* hand3D, unsigned short slicenr1, float thickness1, float zoom1, bool orientation, bool bmpon, bool tissuevisible1, bool zposvisible1, bool xyposvisible1, int xypos1, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), m_Tissuevisible(tissuevisible1), m_Handler3D(hand3D), m_Slicenr(slicenr1), m_Directionx(orientation), m_Bmporwork(bmpon), m_Thickness(thickness1), m_Zposvisible(zposvisible1), m_Xyposvisible(xyposvisible1), m_Xypos(xypos1), m_Zoom(zoom1)
{
	if (m_Directionx)
	{
		if (m_Bmporwork)
			m_Bmpbits = m_Handler3D->SlicebmpX(m_Slicenr);
		else
			m_Bmpbits = m_Handler3D->SliceworkX(m_Slicenr);
		m_Tissue = m_Handler3D->SlicetissueX(m_Slicenr);
		m_Width = m_Handler3D->Height();
		m_Height = m_Handler3D->NumSlices();
		m_D = m_Handler3D->GetPixelsize().low;
	}
	else
	{
		if (m_Bmporwork)
			m_Bmpbits = m_Handler3D->SlicebmpY(m_Slicenr);
		else
			m_Bmpbits = m_Handler3D->SliceworkY(m_Slicenr);
		m_Tissue = m_Handler3D->SlicetissueY(m_Slicenr);
		m_Width = m_Handler3D->Width();
		m_Height = m_Handler3D->NumSlices();
		m_D = m_Handler3D->GetPixelsize().high;
	}

	m_Scalefactorbmp = 1.0f;
	m_Scaleoffsetbmp = 0.0f;
	m_Scalefactorwork = 1.0f;
	m_Scaleoffsetwork = 0.0f;

	m_Image.create(int(m_Width), int(m_Height), 32);

	setFixedSize((int)(m_Width * m_D * m_Zoom), (int)(m_Height * m_Thickness * m_Zoom));
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	show();

	ReloadBits();
}

void Bmptissuesliceshower::paintEvent(QPaintEvent* e)
{
	if (m_Image.size() != QSize(0, 0))
	{ // is an image loaded?
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.scale(m_D * m_Zoom, m_Thickness * m_Zoom);
		painter.drawImage(0, 0, m_Image);
	}
}

void Bmptissuesliceshower::SetBmporwork(bool bmpon)
{
	if (bmpon != m_Bmporwork)
	{
		m_Bmporwork = bmpon;
		update();
	}
}

void Bmptissuesliceshower::BmpChanged()
{
	if (m_Bmporwork)
	{
		update();
	}
}

void Bmptissuesliceshower::WorkChanged()
{
	if (!m_Bmporwork)
	{
		update();
	}
}

void Bmptissuesliceshower::update()
{
	ISEG_DEBUG("SliceViewerWidget::update (slice:" << m_Handler3D->ActiveSlice() << ")");
	unsigned short w, h;

	if (m_Directionx)
	{
		w = m_Handler3D->Height();
		h = m_Handler3D->NumSlices();
	}
	else
	{
		w = m_Handler3D->Width();
		h = m_Handler3D->NumSlices();
	}

	if (w != m_Width || h != m_Height)
	{
		m_Width = w;
		m_Height = h;
		m_Image.create(int(w), int(h), 32);
		setFixedSize((int)(w * m_D * m_Zoom), (int)(h * m_Thickness * m_Zoom));
		free(m_Bmpbits);
		free(m_Tissue);
		if (m_Directionx)
		{
			if (m_Bmporwork)
				m_Bmpbits = m_Handler3D->SlicebmpX(m_Slicenr);
			else
				m_Bmpbits = m_Handler3D->SliceworkX(m_Slicenr);
			m_Tissue = m_Handler3D->SlicetissueX(m_Slicenr);
		}
		else
		{
			if (m_Bmporwork)
				m_Bmpbits = m_Handler3D->SlicebmpY(m_Slicenr);
			else
				m_Bmpbits = m_Handler3D->SliceworkY(m_Slicenr);
			m_Tissue = m_Handler3D->SlicetissueY(m_Slicenr);
		}
	}
	else
	{
		if (m_Directionx)
		{
			if (m_Bmporwork)
				m_Handler3D->SlicebmpX(m_Bmpbits, m_Slicenr);
			else
				m_Handler3D->SliceworkX(m_Bmpbits, m_Slicenr);
			m_Handler3D->SlicetissueX(m_Tissue, m_Slicenr);
		}
		else
		{
			if (m_Bmporwork)
				m_Handler3D->SlicebmpY(m_Bmpbits, m_Slicenr);
			else
				m_Handler3D->SliceworkY(m_Bmpbits, m_Slicenr);
			m_Handler3D->SlicetissueY(m_Tissue, m_Slicenr);
		}
	}

	ReloadBits();
	repaint();
}

void Bmptissuesliceshower::ReloadBits()
{
	unsigned pos = 0;
	int f;
	if (m_Tissuevisible)
	{
		float scaleoffset, scalefactor;
		if (m_Bmporwork)
			scaleoffset = m_Scaleoffsetbmp;
		else
			scaleoffset = m_Scaleoffsetwork;
		if (m_Bmporwork)
			scalefactor = m_Scalefactorbmp;
		else
			scalefactor = m_Scalefactorwork;
		unsigned char r, g, b;
		for (int y = 0; y < m_Height; y++)
		{
			for (int x = 0; x < m_Width; x++)
			{
				f = (int)std::max(0.0f, std::min(255.0f, scaleoffset + scalefactor * (m_Bmpbits)[pos]));
				if (m_Tissue[pos] == 0)
				{
					m_Image.setPixel(x, y, qRgb(int(f), int(f), int(f)));
				}
				else
				{
					TissueInfos::GetTissueColorBlendedRGB(m_Tissue[pos], r, g, b, f);
					m_Image.setPixel(x, y, qRgb(r, g, b));
				}
				pos++;
			}
		}
	}
	else
	{
		float scaleoffset, scalefactor;
		if (m_Bmporwork)
			scaleoffset = m_Scaleoffsetbmp;
		else
			scaleoffset = m_Scaleoffsetwork;
		if (m_Bmporwork)
			scalefactor = m_Scalefactorbmp;
		else
			scalefactor = m_Scalefactorwork;
		for (int y = 0; y < m_Height; y++)
		{
			for (int x = 0; x < m_Width; x++)
			{
				f = (int)std::max(0.0f, std::min(255.0f, scaleoffset + scalefactor * (m_Bmpbits)[pos]));
				m_Image.setPixel(x, y, qRgb(f, f, f));
				pos++;
			}
		}
	}

	if (m_Zposvisible)
	{
		for (int x = 0; x < m_Width; x++)
		{
			m_Image.setPixel(x, m_Handler3D->ActiveSlice(), qRgb(0, 255, 0));
		}
	}

	if (m_Xyposvisible)
	{
		for (int y = 0; y < m_Height; y++)
		{
			m_Image.setPixel(m_Xypos, y, qRgb(0, 255, 0));
		}
	}
}

void Bmptissuesliceshower::SetScale(float offset1, float factor1, bool bmporwork1)
{
	if (bmporwork1)
	{
		m_Scalefactorbmp = factor1;
		m_Scaleoffsetbmp = offset1;
	}
	else
	{
		m_Scalefactorwork = factor1;
		m_Scaleoffsetwork = offset1;
	}

	if (m_Bmporwork == bmporwork1)
	{
		ReloadBits();
		repaint();
	}
}

void Bmptissuesliceshower::SetScaleoffset(float offset1, bool bmporwork1)
{
	if (bmporwork1)
		m_Scaleoffsetbmp = offset1;
	else
		m_Scaleoffsetwork = offset1;

	if (m_Bmporwork == bmporwork1)
	{
		ReloadBits();
		repaint();
	}
}

void Bmptissuesliceshower::SetScalefactor(float factor1, bool bmporwork1)
{
	if (bmporwork1)
		m_Scalefactorbmp = factor1;
	else
		m_Scalefactorwork = factor1;

	if (m_Bmporwork == bmporwork1)
	{
		ReloadBits();
		repaint();
	}
}

void Bmptissuesliceshower::TissueChanged()
{
	if (m_Directionx)
	{
		m_Handler3D->SlicetissueX(m_Tissue, m_Slicenr);
	}
	else
	{
		m_Handler3D->SlicetissueY(m_Tissue, m_Slicenr);
	}
	ReloadBits();
	repaint();
}

void Bmptissuesliceshower::SetTissuevisible(bool on)
{
	m_Tissuevisible = on;
	update();
}

void Bmptissuesliceshower::SlicenrChanged(int i)
{
	m_Slicenr = (unsigned short)i;
	update();
}

void Bmptissuesliceshower::ThicknessChanged(float thickness1)
{
	if (m_Thickness != thickness1)
	{
		m_Thickness = thickness1;

		setFixedSize((int)(m_Width * m_D * m_Zoom), (int)(m_Height * m_Thickness * m_Zoom));

		repaint();
	}
}

void Bmptissuesliceshower::PixelsizeChanged(Pair pixelsize1)
{
	float d1;
	if (m_Directionx)
		d1 = pixelsize1.low;
	else
		d1 = pixelsize1.high;

	if (d1 != m_D)
	{
		m_D = d1;

		setFixedSize((int)(m_Width * m_D * m_Zoom), (int)(m_Height * m_Thickness * m_Zoom));

		repaint();

		if (m_Directionx)
		{
			setFixedSize((int)(m_Width * m_D * m_Zoom), (int)(m_Height * m_Thickness * m_Zoom));
		}
		else
		{
			setFixedSize((int)(m_Width * m_D * m_Zoom), (int)(m_Height * m_Thickness * m_Zoom));
		}
	}
}
void Bmptissuesliceshower::SetZposvisible(bool on)
{
	m_Zposvisible = on;
	ReloadBits();
	repaint();
}

void Bmptissuesliceshower::SetXyposvisible(bool on)
{
	m_Xyposvisible = on;

	ReloadBits();
	repaint();
}

void Bmptissuesliceshower::ZposChanged()
{
	ReloadBits();
	repaint();
}

void Bmptissuesliceshower::XyposChanged(int i)
{
	if (i < m_Width)
		m_Xypos = i;
	ReloadBits();
	repaint();
}

void Bmptissuesliceshower::SetZoom(double z)
{
	if (z != m_Zoom)
	{
		m_Zoom = z;
		setFixedSize((int)(m_Width * m_D * m_Zoom), (int)(m_Height * m_Thickness * m_Zoom));
		repaint();
	}
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

SliceViewerWidget::SliceViewerWidget(SlicesHandler* hand3D, bool orientation, float thickness1, float zoom1, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), m_Handler3D(hand3D), m_Directionx(orientation), m_Xyexists(false)
{
	if (m_Directionx)
		m_Nrslices = m_Handler3D->Width();
	else
		m_Nrslices = m_Handler3D->Height();

	m_Vbox = new Q3VBoxLayout(this);

	m_Scroller = new Q3ScrollView(this);
	m_Shower = new Bmptissuesliceshower(m_Handler3D, 0, thickness1, zoom1, m_Directionx, true, true, false, false, 0, this);

	m_Vbox->addWidget(m_Scroller);
	m_Scroller->addChild(m_Shower);

	m_Hbox = new Q3HBoxLayout(this);
	m_Vbox->addLayout(m_Hbox);
	m_Hbox2 = new Q3HBoxLayout(this);
	m_Vbox->addLayout(m_Hbox2);

	m_CbTissuevisible = new QCheckBox("Show tissues", this);
	m_Hbox->addWidget(m_CbTissuevisible);
	m_CbTissuevisible->setChecked(true);

	m_RbBmp = new QRadioButton("Source", this);
	m_Hbox->addWidget(m_RbBmp);
	m_RbWork = new QRadioButton("Target", this);
	m_Hbox->addWidget(m_RbWork);

	m_BgBmporwork = new QButtonGroup(this);
	m_BgBmporwork->insert(m_RbBmp);
	m_BgBmporwork->insert(m_RbWork);

	m_CbZposvisible = new QCheckBox("Show zpos", this);
	m_Hbox2->addWidget(m_CbZposvisible);
	m_CbZposvisible->setChecked(false);
	m_CbXyposvisible = new QCheckBox("Show xypos", this);
	m_Hbox2->addWidget(m_CbXyposvisible);
	m_CbXyposvisible->setChecked(false);
	m_CbXyposvisible->setEnabled(false);

	m_RbBmp->setChecked(TRUE);

	m_QsbSlicenr = new QScrollBar(1, m_Nrslices, 1, 5, 1, Qt::Horizontal, this);
	m_Vbox->addWidget(m_QsbSlicenr);

	QObject_connect(m_QsbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SlicenrChanged(int)));
	QObject_connect(m_CbTissuevisible, SIGNAL(clicked()), this, SLOT(TissuevisibleChanged()));
	QObject_connect(m_BgBmporwork, SIGNAL(buttonClicked(int)), this, SLOT(WorkorbmpChanged()));

	QObject_connect(m_CbXyposvisible, SIGNAL(clicked()), this, SLOT(XyposvisibleChanged()));
	QObject_connect(m_CbZposvisible, SIGNAL(clicked()), this, SLOT(ZposvisibleChanged()));

	show();
}

SliceViewerWidget::~SliceViewerWidget()
{
	delete m_Vbox;
	delete m_Shower;
	delete m_Scroller;
	delete m_CbTissuevisible;
	delete m_CbZposvisible;
	delete m_CbXyposvisible;
	delete m_RbBmp;
	delete m_RbWork;
	delete m_BgBmporwork;
	delete m_QsbSlicenr;
}

void SliceViewerWidget::closeEvent(QCloseEvent* qce)
{
	emit Hasbeenclosed();
	QWidget::closeEvent(qce);
}

void SliceViewerWidget::SlicenrChanged(int i)
{
	m_Shower->SlicenrChanged(i - 1);
	emit SliceChanged(i - 1);
}

int SliceViewerWidget::GetSlicenr() { return m_QsbSlicenr->value() - 1; }

void SliceViewerWidget::BmpChanged()
{
	if (m_RbBmp->isChecked())
	{
		unsigned short nrslicesnew;
		if (m_Directionx)
			nrslicesnew = m_Handler3D->Width();
		else
			nrslicesnew = m_Handler3D->Height();

		if (nrslicesnew != m_Nrslices)
		{
			m_Nrslices = nrslicesnew;
			m_QsbSlicenr->setMaxValue((int)m_Nrslices);
			m_QsbSlicenr->setValue(1);
			m_Shower->SlicenrChanged(0);
		}
		else
			m_Shower->BmpChanged();

		m_QsbSlicenr->setFixedWidth(m_Shower->size().width());
	}
}

void SliceViewerWidget::WorkChanged()
{
	if (m_RbWork->isChecked())
	{
		unsigned short nrslicesnew;
		if (m_Directionx)
			nrslicesnew = m_Handler3D->Width();
		else
			nrslicesnew = m_Handler3D->Height();

		if (nrslicesnew != m_Nrslices)
		{
			m_Nrslices = nrslicesnew;
			m_QsbSlicenr->setMaxValue((int)m_Nrslices);
			m_QsbSlicenr->setValue(1);
			m_Shower->SlicenrChanged(0);
		}
		else
			m_Shower->BmpChanged();

		m_QsbSlicenr->setFixedWidth(m_Shower->size().width());
	}
}

void SliceViewerWidget::TissueChanged() { m_Shower->TissueChanged(); }

void SliceViewerWidget::TissuevisibleChanged()
{
	m_Shower->SetTissuevisible(m_CbTissuevisible->isChecked());
}

void SliceViewerWidget::WorkorbmpChanged()
{
	if (m_RbBmp->isChecked())
	{
		m_Shower->SetBmporwork(true);
	}
	else
	{
		m_Shower->SetBmporwork(false);
	}
}

void SliceViewerWidget::ThicknessChanged(float thickness1)
{
	m_Shower->ThicknessChanged(thickness1);
}

void SliceViewerWidget::PixelsizeChanged(Pair pixelsize1)
{
	m_Shower->PixelsizeChanged(pixelsize1);
}

void SliceViewerWidget::XyexistsChanged(bool on)
{
	m_CbXyposvisible->setEnabled(on);
	if (on)
	{
		m_Shower->SetXyposvisible(m_CbXyposvisible->isChecked());
	}
}

void SliceViewerWidget::ZposChanged() { m_Shower->ZposChanged(); }

void SliceViewerWidget::XyposChanged(int i) { m_Shower->XyposChanged(i); }

void SliceViewerWidget::XyposvisibleChanged()
{
	m_Shower->SetXyposvisible(m_CbXyposvisible->isChecked());
}

void SliceViewerWidget::ZposvisibleChanged()
{
	m_Shower->SetZposvisible(m_CbZposvisible->isChecked());
}

void SliceViewerWidget::SetZoom(double z)
{
	m_Shower->SetZoom(z);
}

void SliceViewerWidget::SetScale(float offset1, float factor1, bool bmporwork1)
{
	m_Shower->SetScale(offset1, factor1, bmporwork1);
}

void SliceViewerWidget::SetScaleoffset(float offset1, bool bmporwork1)
{
	m_Shower->SetScaleoffset(offset1, bmporwork1);
}

void SliceViewerWidget::SetScalefactor(float factor1, bool bmporwork1)
{
	m_Shower->SetScalefactor(factor1, bmporwork1);
}

} // namespace iseg
