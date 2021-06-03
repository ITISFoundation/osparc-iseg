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

#include "AtlasViewer.h"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QPen>
#include <QWidget>

#include <algorithm>

namespace iseg {

AtlasViewer::AtlasViewer(float* bmpbits1, tissues_size_t* tissue1, unsigned char orient1, unsigned short dimx1, unsigned short dimy1, unsigned short dimz1, float dx1, float dy1, float dz1, std::vector<float>* r, std::vector<float>* g, std::vector<float>* b, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags)
{
	setCursor(Qt::CrossCursor);
	m_Scalefactor = 1.0f;
	m_Scaleoffset = 0.0f;
	m_Zoom = 1.0;
	m_Tissueopac = 0.5;
	m_Pixelsize.high = m_Pixelsize.low = 1.0f;

	m_Dimx = dimx1;
	m_Dimy = dimy1;
	m_Dimz = dimz1;

	m_Dx = dx1;
	m_Dy = dy1;
	m_Dz = dz1;

	m_Bmpbits = bmpbits1;
	m_Tissue = tissue1;
	m_CurrentBmpbits = nullptr;
	m_CurrentTissue = nullptr;

	m_Orient = orient1;

	m_Slicenr = 0;

	m_ColorR = r;
	m_ColorG = g;
	m_ColorB = b;

	Init();
}

AtlasViewer::~AtlasViewer()
{
	delete[] m_CurrentBmpbits;
	delete[] m_CurrentTissue;
}

void AtlasViewer::SetZoom(double z)
{
	if (z != m_Zoom)
	{
		m_Zoom = z;
		setFixedSize((int)m_Width * (m_Zoom * m_Pixelsize.high), (int)m_Height * (m_Zoom * m_Pixelsize.low));
		repaint();
	}
}

void AtlasViewer::paintEvent(QPaintEvent* e)
{
	if (m_Image.size() != QSize(0, 0))
	{ // is an image loaded?
		{
			QPainter painter(this);
			painter.setClipRect(e->rect());

			painter.scale(m_Zoom * m_Pixelsize.high, m_Zoom * m_Pixelsize.low);
			painter.drawImage(0, 0, m_Image);
		}
	}
}

void AtlasViewer::update()
{
	QRect rect;
	rect.setLeft(0);
	rect.setTop(0);
	rect.setRight(m_Width - 1);
	rect.setBottom(m_Height - 1);
	update(rect);
}

void AtlasViewer::update(QRect rect)
{
	unsigned short newwidth, newheight;
	float newdx, newdy;
	if (m_Orient == 0)
	{
		newwidth = m_Dimy;
		newheight = m_Dimz;
		newdx = m_Dy;
		newdy = m_Dz;
	}
	else if (m_Orient == 1)
	{
		newwidth = m_Dimx;
		newheight = m_Dimz;
		newdx = m_Dx;
		newdy = m_Dz;
	}
	else if (m_Orient == 2)
	{
		newwidth = m_Dimx;
		newheight = m_Dimy;
		newdx = m_Dx;
		newdy = m_Dy;
	}

	if (newwidth != m_Width || newheight != m_Height || newdx != m_Pixelsize.high ||
			newdy != m_Pixelsize.low)
	{
		m_Width = newwidth;
		m_Height = newheight;
		m_Pixelsize.high = newdx;
		m_Pixelsize.low = newdy;
		delete[] m_CurrentBmpbits;
		delete[] m_CurrentTissue;
		m_CurrentBmpbits = new float[m_Height * (unsigned)(m_Width)];
		m_CurrentTissue = new tissues_size_t[m_Height * (unsigned)(m_Width)];
		m_Image.create(int(m_Width), int(m_Height), 32);
		setFixedSize((int)m_Width * m_Zoom * m_Pixelsize.high, (int)m_Height * m_Zoom * m_Pixelsize.low);
	}

	GetSlice();

	ReloadBits();
	repaint((int)(rect.left() * m_Zoom * m_Pixelsize.high), (int)((m_Height - 1 - rect.bottom()) * m_Zoom * m_Pixelsize.low), (int)ceil(rect.width() * m_Zoom * m_Pixelsize.high), (int)ceil(rect.height() * m_Zoom * m_Pixelsize.low));
}

void AtlasViewer::Init()
{
	if (m_Orient == 0)
	{
		m_Width = m_Dimy;
		m_Height = m_Dimz;
		m_Pixelsize.high = m_Dy;
		m_Pixelsize.low = m_Dz;
	}
	else if (m_Orient == 1)
	{
		m_Width = m_Dimx;
		m_Height = m_Dimz;
		m_Pixelsize.high = m_Dx;
		m_Pixelsize.low = m_Dz;
	}
	else if (m_Orient == 2)
	{
		m_Width = m_Dimx;
		m_Height = m_Dimy;
		m_Pixelsize.high = m_Dx;
		m_Pixelsize.low = m_Dy;
	}

	m_Image.create(int(m_Width), int(m_Height), 32);
	delete[] m_CurrentBmpbits;
	delete[] m_CurrentTissue;
	m_CurrentBmpbits = new float[m_Height * (unsigned)(m_Width)];
	m_CurrentTissue = new tissues_size_t[m_Height * (unsigned)(m_Width)];

	setFixedSize((int)m_Width * m_Zoom * m_Pixelsize.high, (int)m_Height * m_Zoom * m_Pixelsize.low);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	GetSlice();

	ReloadBits();
	repaint();
	show();
}

void AtlasViewer::GetSlice()
{
	if (m_Orient == 0)
	{
		if (m_Slicenr >= m_Dimx)
			m_Slicenr = m_Dimx - 1;
		unsigned pos = 0;
		unsigned pos1 = unsigned(m_Slicenr);
		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < m_Width; j++, pos++, pos1 += m_Dimx)
			{
				m_CurrentTissue[pos] = m_Tissue[pos1];
				m_CurrentBmpbits[pos] = m_Bmpbits[pos1];
			}
		}
	}
	else if (m_Orient == 1)
	{
		if (m_Slicenr >= m_Dimy)
			m_Slicenr = m_Dimy - 1;
		unsigned pos = 0;
		unsigned pos1 = unsigned(m_Slicenr) * m_Width;
		for (unsigned short i = 0; i < m_Height;
				 i++, pos1 += unsigned(m_Dimy - 1) * m_Dimx)
		{
			for (unsigned short j = 0; j < m_Width; j++, pos++, pos1++)
			{
				m_CurrentTissue[pos] = m_Tissue[pos1];
				m_CurrentBmpbits[pos] = m_Bmpbits[pos1];
			}
		}
	}
	else if (m_Orient == 2)
	{
		if (m_Slicenr >= m_Dimz)
			m_Slicenr = m_Dimz - 1;
		unsigned pos = 0;
		unsigned pos1 = unsigned(m_Slicenr) * unsigned(m_Dimy) * m_Dimx;
		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < m_Width; j++, pos++, pos1++)
			{
				m_CurrentTissue[pos] = m_Tissue[pos1];
				m_CurrentBmpbits[pos] = m_Bmpbits[pos1];
			}
		}
	}
}

void AtlasViewer::ReloadBits()
{
	unsigned pos = 0;
	int f;
	for (int y = m_Height - 1; y >= 0; y--)
	{
		for (int x = 0; x < m_Width; x++)
		{
			f = (int)std::max(0.0f, std::min(255.0f, m_Scaleoffset + m_Scalefactor * (m_CurrentBmpbits)[pos]));
			if (m_CurrentTissue[pos] == 0)
				m_Image.setPixel(x, y, qRgb(int(f), int(f), int(f)));
			else
				m_Image.setPixel(x, y, qRgb(int(f + m_Tissueopac * (255.0f * ((*m_ColorR)[m_CurrentTissue[pos] - 1]) - f)), int(f + m_Tissueopac * (255.0f * ((*m_ColorG)[m_CurrentTissue[pos] - 1]) - f)), int(f + m_Tissueopac * (255.0f * ((*m_ColorB)[m_CurrentTissue[pos] - 1]) - f))));
			pos++;
		}
	}
}

void AtlasViewer::PixelsizeChanged(Pair pixelsize1)
{
	if (pixelsize1.high != m_Pixelsize.high || pixelsize1.low != m_Pixelsize.low)
	{
		m_Pixelsize = pixelsize1;
		setFixedSize((int)m_Width * m_Zoom * m_Pixelsize.high, (int)m_Height * m_Zoom * m_Pixelsize.low);
		repaint();
	}
}

void AtlasViewer::ZoomIn() { SetZoom(2 * m_Zoom); }

void AtlasViewer::ZoomOut() { SetZoom(0.5 * m_Zoom); }

void AtlasViewer::Unzoom() { SetZoom(1.0); }

double AtlasViewer::ReturnZoom() const { return m_Zoom; }

void AtlasViewer::SetScale(float offset1, float factor1)
{
	m_Scalefactor = factor1;
	m_Scaleoffset = offset1;

	ReloadBits();
	repaint();
}

void AtlasViewer::SetScaleoffset(float offset1)
{
	m_Scaleoffset = offset1;

	ReloadBits();
	repaint();
}

void AtlasViewer::SetScalefactor(float factor1)
{
	m_Scalefactor = factor1;

	ReloadBits();
	repaint();
}

void AtlasViewer::mouseMoveEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)std::max(std::min(m_Width - 1.0, (e->x() / (m_Zoom * m_Pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(m_Height - 1.0, m_Height - ((e->y() + 1) / (m_Zoom * m_Pixelsize.low))), 0.0);

	unsigned pos = p.px + p.py * m_Width;
	emit MousemovedSign(m_CurrentTissue[pos]);
}

void AtlasViewer::SlicenrChanged(unsigned short slicenr1)
{
	m_Slicenr = slicenr1;
	if (m_Orient == 0 && m_Slicenr >= m_Dimx)
		m_Slicenr = m_Dimx - 1;
	else if (m_Orient == 1 && m_Slicenr >= m_Dimy)
		m_Slicenr = m_Dimy - 1;
	else if (m_Orient == 2 && m_Slicenr >= m_Dimz)
		m_Slicenr = m_Dimz - 1;
	update();
}

void AtlasViewer::OrientChanged(unsigned char orient1)
{
	m_Orient = orient1;
	if (m_Orient == 0 && m_Slicenr >= m_Dimx)
		m_Slicenr = m_Dimx - 1;
	else if (m_Orient == 1 && m_Slicenr >= m_Dimy)
		m_Slicenr = m_Dimy - 1;
	else if (m_Orient == 2 && m_Slicenr >= m_Dimz)
		m_Slicenr = m_Dimz - 1;
	update();
}

void AtlasViewer::SetTissueopac(float tissueopac1)
{
	m_Tissueopac = tissueopac1;
	update();
}

} // namespace iseg
