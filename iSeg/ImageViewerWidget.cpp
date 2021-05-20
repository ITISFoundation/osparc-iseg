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

#include "ImageViewerWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Interface/QtConnect.h"

#include "Core/ColorLookupTable.h"

#include "Data/ExtractBoundary.h"
#include "Data/Point.h"

#include <QAction>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <qapplication.h>
#include <qcolor.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpen.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <sstream>
#include <string>

namespace iseg {

ImageViewerWidget::ImageViewerWidget(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags) //,showvp(false)
{
	m_Brightness = m_Scaleoffset = 0.0f;
	m_Contrast = m_Scalefactor = 1.0f;
	m_Mode = 1;
	m_Zoom = 1.0;
	m_Pixelsize.high = m_Pixelsize.low = 1.0f;
	m_Workborderlimit = true;
	m_ActualColor.setRgb(255, 255, 255);
	m_OutlineColor.setRgb(255, 255, 255);
	m_Crosshairxpos = 0;
	m_Crosshairypos = 0;
	m_Marks = nullptr;
	m_Overlayalpha = 0.0f;

	m_Selecttissue = new QAction("Select Tissue", this);
	m_Addtoselection = new QAction("Select Tissue", this);
	m_Viewtissue = new QAction("View Tissue Surface", this);
	m_Viewtarget = new QAction("View Target Surface", this);
	m_Nexttargetslice = new QAction("Next Target Slice", this);
	m_Addmark = new QAction("&Add Mark", this);
	m_Addlabel = new QAction("Add &Label", this);
	m_Removemark = new QAction("&Remove Mark", this);
	m_Clearmarks = new QAction("&Clear Marks", this);
	m_Addtissue = new QAction("Add &Tissue", this);
	m_Addtissueconnected = new QAction("Add Tissue &Conn", this);
	m_Addtissue3D = new QAction("Add Tissue 3&D", this);
	m_Subtissue = new QAction("&Subtract Tissue", this);
	m_Addtissuelarger = new QAction("Add Tissue &Larger", this);
	QObject_connect(m_Addmark, SIGNAL(activated()), this, SLOT(AddMark()));
	QObject_connect(m_Addlabel, SIGNAL(activated()), this, SLOT(AddLabel()));
	QObject_connect(m_Clearmarks, SIGNAL(activated()), this, SLOT(ClearMarks()));
	QObject_connect(m_Removemark, SIGNAL(activated()), this, SLOT(RemoveMark()));
	QObject_connect(m_Addtissue, SIGNAL(activated()), this, SLOT(AddTissue()));
	QObject_connect(m_Addtissueconnected, SIGNAL(activated()), this, SLOT(AddTissueConnected()));
	QObject_connect(m_Subtissue, SIGNAL(activated()), this, SLOT(SubTissue()));
	QObject_connect(m_Addtissue3D, SIGNAL(activated()), this, SLOT(AddTissue3D()));
	QObject_connect(m_Addtissuelarger, SIGNAL(activated()), this, SLOT(AddTissuelarger()));
	QObject_connect(m_Selecttissue, SIGNAL(activated()), this, SLOT(SelectTissue()));
	QObject_connect(m_Addtoselection, SIGNAL(activated()), this, SLOT(AddToSelectedTissues()));
	QObject_connect(m_Viewtissue, SIGNAL(activated()), this, SLOT(ViewTissueSurface()));
	QObject_connect(m_Viewtarget, SIGNAL(activated()), this, SLOT(ViewTargetSurface()));
	QObject_connect(m_Nexttargetslice, SIGNAL(activated()), this, SLOT(NextTargetSlice()));
}

ImageViewerWidget::~ImageViewerWidget()
{
	delete m_Addmark;
	delete m_Addlabel;
	delete m_Removemark;
	delete m_Clearmarks;
	delete m_Addtissue;
	delete m_Addtissueconnected;
	delete m_Addtissue3D;
	delete m_Subtissue;
	delete m_Addtissuelarger;
	delete m_Selecttissue;
	delete m_Addtoselection;
}

void ImageViewerWidget::ModeChanged(unsigned char newmode, bool updatescale)
{
	if (newmode != 0 && m_Mode != newmode)
	{
		m_Mode = newmode;
		if (updatescale)
		{
			UpdateScaleoffsetfactor();
		}
	}
}

void ImageViewerWidget::GetScaleoffsetfactor(float& offset1, float& factor1) const
{
	offset1 = m_Scaleoffset;
	factor1 = m_Scalefactor;
}

void ImageViewerWidget::SetZoom(double z)
{
	if (z != m_Zoom)
	{
		//QPoint oldCenter = visibleRegion().boundingRect().center();
		QPoint old_center = rect().center();
		QPoint new_center;
		if (m_MousePosZoom.x() == 0 && m_MousePosZoom.y() == 0)
		{
			new_center = z * old_center / m_Zoom;
		}
		else
		{
			new_center = m_Zoom * (old_center + z * m_MousePosZoom / m_Zoom - m_MousePosZoom) / z;
		}

		m_Zoom = z;
		int const w = static_cast<int>(m_Width) * m_Zoom * m_Pixelsize.high;
		int const h = static_cast<int>(m_Height) * m_Zoom * m_Pixelsize.low;
		setFixedSize(w, h);
		if (m_MousePosZoom.x() != 0 && m_MousePosZoom.y() != 0)
		{
			emit SetcenterSign(new_center.x(), new_center.y());
		}
	}
}

void ImageViewerWidget::PixelsizeChanged(Pair pixelsize1)
{
	if (pixelsize1.high != m_Pixelsize.high || pixelsize1.low != m_Pixelsize.low)
	{
		m_Pixelsize = pixelsize1;
		setFixedSize((int)m_Width * m_Zoom * m_Pixelsize.high, (int)m_Height * m_Zoom * m_Pixelsize.low);
		repaint();
	}
}

void ImageViewerWidget::paintEvent(QPaintEvent* e)
{
	m_Marks = m_Handler3D->GetActivebmphandler()->ReturnMarks();
	if (m_Image.size() != QSize(0, 0)) // is an image loaded?
	{
		{
			QPainter painter(this);
			painter.setClipRect(e->rect());
			painter.scale(m_Zoom * m_Pixelsize.high, m_Zoom * m_Pixelsize.low);
			painter.drawImage(0, 0, m_ImageDecorated);
			painter.setPen(QPen(m_ActualColor));

			if (m_Marks != nullptr)
			{
				unsigned char r, g, b;
				for (auto& m : *m_Marks)
				{
					std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
					QColor qc1(r, g, b);
					painter.setPen(QPen(qc1));

					painter.drawLine(int(m.p.px) - 2, int(m_Height - m.p.py) - 3, int(m.p.px) + 2, int(m_Height - m.p.py) + 1);
					painter.drawLine(int(m.p.px) - 2, int(m_Height - m.p.py) + 1, int(m.p.px) + 2, int(m_Height - m.p.py) - 3);
					if (!m.name.empty())
					{
						painter.drawText(int(m.p.px) + 3, int(m_Height - m.p.py) + 1, QString(m.name.c_str()));
					}
				}
			}
		}
		{
			QPainter painter1(this);

			float dx = m_Zoom * m_Pixelsize.high;
			float dy = m_Zoom * m_Pixelsize.low;

			for (auto& p : m_Vpdyn)
			{
				painter1.fillRect(int(dx * p.px), int(dy * (m_Height - p.py - 1)), int(dx + 0.999f), int(dy + 0.999f), m_ActualColor);
			}
		}
	}
}

void ImageViewerWidget::BmpChanged() { update(); }

void ImageViewerWidget::SlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void ImageViewerWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
	if (m_Bmporwork)
		m_Bmpbits = bmph->ReturnBmpfield();
	else
		m_Bmpbits = bmph->ReturnWorkfield();
	m_Tissue = bmph->ReturnTissuefield(m_Handler3D->ActiveTissuelayer());
	m_Marks = bmph->ReturnMarks();

	ModeChanged(bmph->ReturnMode(m_Bmporwork), false);
	UpdateScaleoffsetfactor();

	ReloadBits();
	if (m_Workborder)
	{
		if (m_Bmporwork)
		{
			WorkborderChanged();
		}
	}
	else
	{
		repaint();
	}
}

void ImageViewerWidget::OverlayChanged()
{
	ReloadBits();
	repaint();
}

void ImageViewerWidget::OverlayChanged(QRect rect)
{
	ReloadBits();
	repaint((int)(rect.left() * m_Zoom * m_Pixelsize.high), (int)((m_Height - 1 - rect.bottom()) * m_Zoom * m_Pixelsize.low), (int)ceil(rect.width() * m_Zoom * m_Pixelsize.high), (int)ceil(rect.height() * m_Zoom * m_Pixelsize.low));
}

void ImageViewerWidget::update()
{
	QRect rect;
	rect.setLeft(0);
	rect.setTop(0);
	rect.setRight(m_Width - 1);
	rect.setBottom(m_Height - 1);
	update(rect);
}

void ImageViewerWidget::update(QRect rect)
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_Overlaybits = m_Handler3D->ReturnOverlay();
	ModeChanged(m_Bmphand->ReturnMode(m_Bmporwork), false);
	UpdateScaleoffsetfactor();
	if (m_Bmporwork)
		m_Bmpbits = m_Bmphand->ReturnBmpfield();
	else
		m_Bmpbits = m_Bmphand->ReturnWorkfield();
	m_Tissue = m_Bmphand->ReturnTissuefield(m_Handler3D->ActiveTissuelayer());
	m_Marks = m_Bmphand->ReturnMarks();

	if (m_Bmphand->ReturnWidth() != m_Width || m_Bmphand->ReturnHeight() != m_Height)
	{
		m_Vp.clear();
		m_VpOld.clear();
		m_Vp1.clear();
		m_Vp1Old.clear();
		m_Vpdyn.clear();
		m_VpdynOld.clear();
		m_Vm.clear();
		m_VmOld.clear();
		m_Width = m_Bmphand->ReturnWidth();
		m_Height = m_Bmphand->ReturnHeight();
		m_Image.create(int(m_Width), int(m_Height), 32);
		m_ImageDecorated.create(int(m_Width), int(m_Height), 32);
		setFixedSize((int)m_Width * m_Zoom * m_Pixelsize.high, (int)m_Height * m_Zoom * m_Pixelsize.low);

		if (m_Bmporwork && m_Workborder)
		{
			ReloadBits();
			WorkborderChanged();
			return;
		}
	}

	ReloadBits();
	repaint((int)(rect.left() * m_Zoom * m_Pixelsize.high), (int)((m_Height - 1 - rect.bottom()) * m_Zoom * m_Pixelsize.low), (int)ceil(rect.width() * m_Zoom * m_Pixelsize.high), (int)ceil(rect.height() * m_Zoom * m_Pixelsize.low));
}

void ImageViewerWidget::Init(SlicesHandler* hand3D, bool bmporwork1)
{
	m_Handler3D = hand3D;
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_Overlaybits = m_Handler3D->ReturnOverlay();
	m_Bmporwork = bmporwork1;
	if (m_Bmporwork)
		m_Bmpbits = m_Bmphand->ReturnBmpfield();
	else
		m_Bmpbits = m_Bmphand->ReturnWorkfield();
	m_Tissue = m_Bmphand->ReturnTissuefield(hand3D->ActiveTissuelayer());
	m_Width = m_Bmphand->ReturnWidth();
	m_Height = m_Bmphand->ReturnHeight();
	m_Marks = m_Bmphand->ReturnMarks();
	m_Image.create(int(m_Width), int(m_Height), 32);
	m_ImageDecorated.create(int(m_Width), int(m_Height), 32);

	setFixedSize((int)m_Width * m_Zoom * m_Pixelsize.high, (int)m_Height * m_Zoom * m_Pixelsize.low);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	ModeChanged(m_Bmphand->ReturnMode(m_Bmporwork), false);
	UpdateRange();
	UpdateScaleoffsetfactor();

	ReloadBits();
	if (m_Workborder)
	{
		if (m_Bmporwork)
			WorkborderChanged();
		else
			repaint();
	}
	show();
}

void ImageViewerWidget::UpdateRange()
{
	// Recompute ranges for all slices
	if (m_Bmporwork)
	{
		m_Handler3D->ComputeBmprangeMode1(&m_RangeMode1);
	}
	else
	{
		m_Handler3D->ComputeRangeMode1(&m_RangeMode1);
	}
}

void ImageViewerWidget::UpdateRange(unsigned short slicenr)
{
	// Recompute range only for single slice
	if (m_Bmporwork)
	{
		m_Handler3D->ComputeBmprangeMode1(slicenr, &m_RangeMode1);
	}
	else
	{
		m_Handler3D->ComputeRangeMode1(slicenr, &m_RangeMode1);
	}
}

void ImageViewerWidget::ReloadBits()
{
	auto color_lut = m_Handler3D->GetColorLookupTable();

	float* bmpbits1 = *m_Bmpbits;
	tissues_size_t* tissue1 = *m_Tissue;
	unsigned pos = 0;
	int f;
	unsigned char r, g, b;

	for (int y = m_Height - 1; y >= 0; y--)
	{
		for (int x = 0; x < m_Width; x++)
		{
			if (m_Picturevisible)
			{
				if (color_lut && m_Bmporwork)
				{
					// \todo not sure if we should allow to 'scale & offset & clamp' when a color lut is available
					//f = max(0.0f, min(255.0f, scaleoffset + scalefactor * (bmpbits1)[pos]));
					color_lut->GetColor((bmpbits1)[pos], r, g, b);
				}
				else
				{
					r = g = b = (int)std::max(0.0f, std::min(255.0f, m_Scaleoffset + m_Scalefactor * (bmpbits1)[pos]));
				}

				// overlay only visible if picture is visible
				if (m_Overlayvisible)
				{
					f = std::max(0.0f, std::min(255.0f, m_Scaleoffset + m_Scalefactor * (m_Overlaybits)[pos]));

					r = (1.0f - m_Overlayalpha) * r + m_Overlayalpha * f;
					g = (1.0f - m_Overlayalpha) * g + m_Overlayalpha * f;
					b = (1.0f - m_Overlayalpha) * b + m_Overlayalpha * f;
				}
			}
			else
			{
				r = g = b = 0;
			}

			if (m_Tissuevisible && tissue1[pos] != 0)
			{
				// blend with tissue color
				auto rgbo = TissueInfos::GetTissueColor(tissue1[pos]);
				float alpha = 0.5f;
				r = static_cast<unsigned char>(r + alpha * (255.0f * rgbo[0] - r));
				g = static_cast<unsigned char>(g + alpha * (255.0f * rgbo[1] - g));
				b = static_cast<unsigned char>(b + alpha * (255.0f * rgbo[2] - b));
				m_Image.setPixel(x, y, qRgb(r, g, b));
			}
			else // no tissue
			{
				m_Image.setPixel(x, y, qRgb(r, g, b));
			}
			pos++;
		}
	}

	// copy to decorated image
	m_ImageDecorated = m_Image;

	// now decorate
	QRgb color_used = m_ActualColor.rgb();
	QRgb color_dim = (m_ActualColor.light(30)).rgb();

	if (m_Workborder && m_Bmporwork &&
			((!m_Workborderlimit) || (unsigned)m_Vp.size() < unsigned(m_Width) * m_Height / 5))
	{
		for (auto& p : m_Vp)
		{
			m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_dim);
		}
	}

	for (auto& p : m_Vp1)
	{
		m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_used);
	}

	for (auto& m : m_Vm)
	{
		std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
		m_ImageDecorated.setPixel(int(m.p.px), int(m_Height - m.p.py - 1), qRgb(r, g, b));
	}

	if (m_Crosshairxvisible)
	{
		for (int x = 0; x < m_Width; x++)
		{
			m_ImageDecorated.setPixel(x, m_Height - 1 - m_Crosshairxpos, qRgb(0, 255, 0));
			m_Image.setPixel(x, m_Height - 1 - m_Crosshairxpos, qRgb(0, 255, 0));
		}
	}

	if (m_Crosshairyvisible)
	{
		for (int y = 0; y < m_Height; y++)
		{
			m_ImageDecorated.setPixel(m_Crosshairypos, y, qRgb(0, 255, 0));
			m_Image.setPixel(m_Crosshairypos, y, qRgb(0, 255, 0));
		}
	}
}

void ImageViewerWidget::TissueChanged()
{
	ReloadBits();
	repaint();
}

void ImageViewerWidget::TissueChanged(QRect rect)
{
	ReloadBits();
	repaint((int)(rect.left() * m_Zoom * m_Pixelsize.high), (int)((m_Height - 1 - rect.bottom()) * m_Zoom * m_Pixelsize.low), (int)ceil(rect.width() * m_Zoom * m_Pixelsize.high), (int)ceil(rect.height() * m_Zoom * m_Pixelsize.low));
}

void ImageViewerWidget::MarkChanged()
{
	m_Marks = m_Bmphand->ReturnMarks();
	repaint();
}

bool ImageViewerWidget::ToggleTissuevisible()
{
	m_Tissuevisible = !m_Tissuevisible;
	update();
	return m_Tissuevisible;
}

bool ImageViewerWidget::TogglePicturevisible()
{
	m_Picturevisible = !m_Picturevisible;
	update();
	return m_Picturevisible;
}

bool ImageViewerWidget::ToggleMarkvisible()
{
	m_Markvisible = !m_Markvisible;
	repaint();
	return m_Markvisible;
}

bool ImageViewerWidget::ToggleOverlayvisible()
{
	m_Overlayvisible = !m_Overlayvisible;
	update();
	return m_Overlayvisible;
}

void ImageViewerWidget::SetTissuevisible(bool on)
{
	m_Tissuevisible = on;
	update();
}

void ImageViewerWidget::SetPicturevisible(bool on)
{
	m_Picturevisible = on;
	update();
}

void ImageViewerWidget::SetMarkvisible(bool on)
{
	m_Markvisible = on;
	repaint();
}

void ImageViewerWidget::SetOverlayvisible(bool on)
{
	m_Overlayvisible = on;
	update();
}

void ImageViewerWidget::SetOverlayalpha(float alpha)
{
	m_Overlayalpha = alpha;
	update();
}

void ImageViewerWidget::AddMark()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;

	emit AddmarkSign(p);
}

void ImageViewerWidget::AddLabel()
{
	bool ok;
	QString new_text = QInputDialog::getText("Label", "Enter a name for the label:", QLineEdit::Normal, "", &ok, this);
	if (ok)
	{
		Point p;
		p.px = (unsigned short)m_Eventx;
		p.py = (unsigned short)m_Eventy;
		emit AddlabelSign(p, new_text.ascii());
	}
}

void ImageViewerWidget::ClearMarks() { emit ClearmarksSign(); }

void ImageViewerWidget::RemoveMark()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit RemovemarkSign(p);
}

void ImageViewerWidget::AddTissue()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit AddtissueSign(p);
}

void ImageViewerWidget::AddTissueConnected()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit AddtissueconnectedSign(p);
}

void ImageViewerWidget::AddTissue3D()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit Addtissue3DSign(p);
}

void ImageViewerWidget::SubTissue()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit SubtissueSign(p);
}

void ImageViewerWidget::AddTissuelarger()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit AddtissuelargerSign(p);
}

void ImageViewerWidget::SelectTissue()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit SelecttissueSign(p, true);
}

void ImageViewerWidget::ViewTissueSurface()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit ViewtissueSign(p);
}
void ImageViewerWidget::ViewTargetSurface()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit ViewtargetSign(p);
}

void ImageViewerWidget::NextTargetSlice()
{
	auto target_slices = m_Handler3D->TargetSlices();
	size_t slice_size = m_Handler3D->Width() * m_Handler3D->Height();

	// find next slice
	int slice = -1;
	auto non_zero = [](float v) { return v != 0.f; };

	for (int s = m_Handler3D->ActiveSlice() + 1; s < m_Handler3D->NumSlices(); ++s)
	{
		auto data = target_slices.at(s);
		if (std::any_of(data, data + slice_size, non_zero))
		{
			slice = s;
			break;
		}
	}

	if (slice == -1)
	{
		for (int s = 0; s <= m_Handler3D->ActiveSlice(); ++s)
		{
			auto data = target_slices.at(s);
			if (std::any_of(data, data + slice_size, non_zero))
			{
				slice = s;
				break;
			}
		}
	}

	if (slice < 0)
	{
		QMessageBox::information(this, "iSeg", "The target contains no foreground pixels");
	}
	else
	{
		m_Handler3D->SetActiveSlice(slice, true);
	}
}

void ImageViewerWidget::AddToSelectedTissues()
{
	Point p;
	p.px = (unsigned short)m_Eventx;
	p.py = (unsigned short)m_Eventy;
	emit SelecttissueSign(p, false);
}

void ImageViewerWidget::ZoomIn() { SetZoom(2 * m_Zoom); }

void ImageViewerWidget::ZoomOut() { SetZoom(0.5 * m_Zoom); }

void ImageViewerWidget::Unzoom() { SetZoom(1.0); }

double ImageViewerWidget::ReturnZoom() const { return m_Zoom; }

void ImageViewerWidget::contextMenuEvent(QContextMenuEvent* event)
{
	m_Eventx = (int)std::max(std::min(m_Width - 1.0, (event->x() / (m_Zoom * m_Pixelsize.high))), 0.0);
	m_Eventy = (int)std::max(std::min(m_Height - 1.0, m_Height - 1 - (event->y() / (m_Zoom * m_Pixelsize.low))), 0.0);

	QMenu context_menu(this);
	// tissue selection
	if (event->modifiers() == Qt::ControlModifier)
	{
		context_menu.addAction(m_Addtoselection);
	}
	else
	{
		context_menu.addAction(m_Selecttissue);
	}

	// surface view
	if (m_Bmporwork)
	{
		context_menu.addAction(m_Viewtissue);
	}
	else
	{
		context_menu.addAction(m_Viewtarget);
		context_menu.addAction(m_Nexttargetslice);

		// add to tissue
		context_menu.insertSeparator();
		context_menu.addAction(m_Addtissue);
		context_menu.addAction(m_Subtissue);
		context_menu.addAction(m_Addtissue3D);
		context_menu.addAction(m_Addtissueconnected);
		context_menu.addAction(m_Addtissuelarger);
	}

	// marks
	context_menu.insertSeparator();
	context_menu.addAction(m_Addmark);
	context_menu.addAction(m_Addlabel);
	context_menu.addAction(m_Removemark);
	context_menu.addAction(m_Clearmarks);

	context_menu.exec(event->globalPos());
}

void ImageViewerWidget::SetBrightnesscontrast(float bright, float contr, bool paint)
{
	m_Brightness = bright;
	m_Contrast = contr;
	UpdateScaleoffsetfactor();
	if (paint)
	{
		ReloadBits();
		repaint();
	}
}

void ImageViewerWidget::UpdateScaleoffsetfactor()
{
	Bmphandler* bmphand = m_Handler3D->GetActivebmphandler();
	if (m_Bmporwork && m_Handler3D->GetColorLookupTable())
	{
		// Disable scaling/offset for color mapped images, since it would break the lookup table
		m_Scalefactor = 1.0f;
		m_Scaleoffset = 0.0f;
	}
	else if (bmphand->ReturnMode(m_Bmporwork) == eScaleMode::kFixedRange)
	{
		// Mode 2 assumes the range [0, 255]
		m_Scalefactor = m_Contrast;
		m_Scaleoffset = (127.5f - 255 * m_Scalefactor) * (1.0f - m_Brightness) + 127.5f * m_Brightness;
	}
	else if (bmphand->ReturnMode(m_Bmporwork) == eScaleMode::kArbitraryRange)
	{
		// Mode 1 assumes an arbitrary range --> scale to range [0, 255]
		auto r = m_RangeMode1;
		if (r.high == r.low)
		{
			r.high = r.low + 1.f;
		}
		m_Scalefactor = 255.0f * m_Contrast / (r.high - r.low);
		m_Scaleoffset = (127.5f - r.high * m_Scalefactor) * (1.0f - m_Brightness) + (127.5f - r.low * m_Scalefactor) * m_Brightness;
	}
	emit ScaleoffsetfactorChanged(m_Scaleoffset, m_Scalefactor, m_Bmporwork);
}

void ImageViewerWidget::mousePressEvent(QMouseEvent* e)
{
	Point p;
	//	p.px=(unsigned short)(e->x()/(zoom*pixelsize.high));
	//	p.py=(unsigned short)height-1-(e->y()/(zoom*pixelsize.low));
	p.px = (unsigned short)std::max(std::min(m_Width - 1.0, (e->x() / (m_Zoom * m_Pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(m_Height - 1.0, m_Height - ((e->y() + 1) / (m_Zoom * m_Pixelsize.low))), 0.0);

	if (e->button() == Qt::LeftButton)
	{
		emit MousepressedSign(p);
	}
	else if (e->button() == Qt::MidButton)
	{
		emit MousepressedmidSign(p);
	}
}

void ImageViewerWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		Point p;
		p.px = (unsigned short)std::max(std::min(m_Width - 1.0, (e->x() / (m_Zoom * m_Pixelsize.high))), 0.0);
		p.py = (unsigned short)std::max(std::min(m_Height - 1.0, m_Height - ((e->y() + 1) / (m_Zoom * m_Pixelsize.low))), 0.0);

		emit MousereleasedSign(p);
	}
}

void ImageViewerWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)std::max(std::min(m_Width - 1.0, (e->x() / (m_Zoom * m_Pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(m_Height - 1.0, m_Height - ((e->y() + 1) / (m_Zoom * m_Pixelsize.low))), 0.0);

	if (e->button() == Qt::LeftButton)
	{
		emit MousedoubleclickSign(p);
	}
	else if (e->button() == Qt::MidButton)
	{
		emit MousedoubleclickmidSign(p);
	}
}

void ImageViewerWidget::mouseMoveEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)std::max(std::min(m_Width - 1.0, (e->x() / (m_Zoom * m_Pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(m_Height - 1.0, m_Height - ((e->y() + 1) / (m_Zoom * m_Pixelsize.low))), 0.0);

	emit MousemovedSign(p);
}

void ImageViewerWidget::wheelEvent(QWheelEvent* e)
{
	int delta = e->delta();

	if (e->state() & Qt::ControlModifier)
	{
		m_MousePosZoom = e->pos();
		emit MousePosZoomSign(m_MousePosZoom);
		emit WheelrotatedctrlSign(delta);
	}
	else
	{
		e->ignore();
	}
}

void ImageViewerWidget::RecomputeWorkborder()
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_Vp = extract_boundary(m_Bmphand->ReturnWork(), m_Width, m_Height, Point());
}

void ImageViewerWidget::WorkborderChanged()
{
	if (m_Workborder)
	{
		RecomputeWorkborder();
		VpChanged();
	}
}

void ImageViewerWidget::WorkborderChanged(QRect rect)
{
	if (m_Workborder)
	{
		RecomputeWorkborder();
		VpChanged(rect);
	}
}

void ImageViewerWidget::VpToImageDecorator()
{
	if ((!m_Workborderlimit) || ((unsigned)m_VpOld.size() < unsigned(m_Width) * m_Height / 5))
	{
		for (auto& p : m_VpOld)
		{
			m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), m_Image.pixel(int(p.px), int(m_Height - p.py - 1)));
		}
	}

	QRgb color_used = m_OutlineColor.rgb();
	QRgb color_dim = (m_OutlineColor.light(30)).rgb();
	if ((!m_Workborderlimit) || ((unsigned)m_Vp.size() < unsigned(m_Width) * m_Height / 5))
	{
		for (auto& p : m_Vp)
		{
			m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_dim);
		}
	}

	for (auto& p : m_Vp1Old)
	{
		m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), m_Image.pixel(int(p.px), int(m_Height - p.py - 1)));
	}

	for (auto& p : m_Vp1)
	{
		m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_used);
	}

	for (auto& m : m_VmOld)
	{
		m_ImageDecorated.setPixel(int(m.p.px), int(m_Height - m.p.py - 1), m_Image.pixel(int(m.p.px), int(m_Height - m.p.py - 1)));
	}

	unsigned char r, g, b;
	for (auto& m : m_Vm)
	{
		std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
		m_ImageDecorated.setPixel(int(m.p.px), int(m_Height - m.p.py - 1), qRgb(r, g, b));
	}
}

void ImageViewerWidget::VpChanged()
{
	VpToImageDecorator();

	repaint();

	m_VpOld = m_Vp;
	m_Vp1Old = m_Vp1;
	m_VmOld = m_Vm;
}

void ImageViewerWidget::VpChanged(QRect rect)
{
	VpToImageDecorator();

	if (rect.left() > 0)
		rect.setLeft(rect.left() - 1);
	if (rect.top() > 0)
		rect.setTop(rect.top() - 1);
	if (rect.right() + 1 < m_Width)
		rect.setRight(rect.right() + 1);
	if (rect.bottom() + 1 < m_Height)
		rect.setBottom(rect.bottom() + 1);
	repaint((int)(rect.left() * m_Zoom * m_Pixelsize.high), (int)((m_Height - 1 - rect.bottom()) * m_Zoom * m_Pixelsize.low), (int)ceil(rect.width() * m_Zoom * m_Pixelsize.high), (int)ceil(rect.height() * m_Zoom * m_Pixelsize.low));

	m_VpOld = m_Vp;
	m_Vp1Old = m_Vp1;
	m_VmOld = m_Vm;
}

void ImageViewerWidget::Vp1dynChanged()
{
	if ((!m_Workborderlimit) || ((unsigned)m_VpOld.size() < unsigned(m_Width) * m_Height / 5))
	{
		for (auto& p : m_VpOld)
		{
			m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), m_Image.pixel(int(p.px), int(m_Height - p.py - 1)));
		}
	}

	QRgb color_used = m_ActualColor.rgb();
	QRgb color_dim = (m_ActualColor.light(30)).rgb();
	QRgb color_highlight = (m_ActualColor.lighter(60)).rgb();
	if ((!m_Workborderlimit) || ((unsigned)m_Vp.size() < unsigned(m_Width) * m_Height / 5))
	{
		for (auto& p : m_Vp)
		{
			m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_dim);
		}
	}

	for (auto& p : m_Vp1Old)
	{
		m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), m_Image.pixel(int(p.px), int(m_Height - p.py - 1)));
	}

	for (auto& p : m_Vp1)
	{
		m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_used);
	}

	for (auto& p : m_LimitPoints)
	{
		m_ImageDecorated.setPixel(int(p.px), int(m_Height - p.py - 1), color_highlight);
	}

	for (auto& m : m_VmOld)
	{
		m_ImageDecorated.setPixel(int(m.p.px), int(m_Height - m.p.py - 1), m_Image.pixel(int(m.p.px), int(m_Height - m.p.py - 1)));
	}

	unsigned char r, g, b;
	for (auto& m : m_Vm)
	{
		std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
		m_ImageDecorated.setPixel(int(m.p.px), int(m_Height - m.p.py - 1), qRgb(r, g, b));
	}

	repaint();

	m_VpdynOld = m_Vpdyn;
	m_VpOld = m_Vp;
	m_Vp1Old = m_Vp1;
	m_VmOld = m_Vm;
}

void ImageViewerWidget::VpdynChanged()
{
	repaint();
}

void ImageViewerWidget::SetWorkbordervisible(bool on)
{
	if (on)
	{
		if (m_Bmporwork && !m_Workborder)
		{
			m_Workborder = true;
			WorkborderChanged();
		}
	}
	else
	{
		if (m_Workborder && m_Bmporwork)
		{
			m_Vp.clear();
			m_Workborder = false;
			ReloadBits();
			repaint();
		}
	}
}

void ImageViewerWidget::SetOutlineColor(const QColor& c)
{
	if (m_OutlineColor != c)
	{
		m_OutlineColor = c;

		// this will trigger a repaint with the new color
		VpChanged();
	}
}

bool ImageViewerWidget::ToggleWorkbordervisible()
{
	if (m_Workborder)
	{
		if (m_Bmporwork)
		{
			m_Vp.clear();
			m_Workborder = false;
			ReloadBits();
			repaint();
		}
	}
	else
	{
		if (m_Bmporwork)
		{
			m_Workborder = true;
			WorkborderChanged();
		}
	}

	return m_Workborder;
}

bool ImageViewerWidget::ReturnWorkbordervisible() const { return m_Workborder; }

void ImageViewerWidget::SetVp1(std::vector<Point>* vp1_arg)
{
	m_Vp1.clear();
	m_Vp1.insert(m_Vp1.begin(), vp1_arg->begin(), vp1_arg->end());
	VpChanged();
}

void ImageViewerWidget::SetVm(std::vector<Mark>* vm_arg)
{
	m_Vm.clear();
	m_Vm.insert(m_Vm.begin(), vm_arg->begin(), vm_arg->end());
	VpChanged();
}

void ImageViewerWidget::SetVpdyn(std::vector<Point>* vpdyn_arg)
{
	m_Vpdyn.clear();
	m_Vpdyn.insert(m_Vpdyn.begin(), vpdyn_arg->begin(), vpdyn_arg->end());
	VpdynChanged();
}

void ImageViewerWidget::SetVp1Dyn(std::vector<Point>* vp1_arg, std::vector<Point>* vpdyn_arg, bool also_points)
{
	m_Vp1.clear();
	m_Vp1.insert(m_Vp1.begin(), vp1_arg->begin(), vp1_arg->end());
	m_Vpdyn.clear();
	m_Vpdyn.insert(m_Vpdyn.begin(), vpdyn_arg->begin(), vpdyn_arg->end());
	m_LimitPoints.clear();
	if (also_points && m_Vp1.size() > 1)
	{
		m_LimitPoints.push_back(m_Vp1.front());
		m_LimitPoints.push_back(m_Vp1.back());
	}
	Vp1dynChanged();
}

void ImageViewerWidget::ColorChanged(int tissue)
{
	unsigned char r, g, b;
	std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(tissue + 1);
	m_ActualColor.setRgb(r, g, b);
	VpChanged();
}

void ImageViewerWidget::CrosshairxChanged(int i)
{
	if (i < m_Height)
	{
		m_Crosshairxpos = i;

		if (m_Crosshairxvisible)
		{
			ReloadBits();
			repaint();
		}
	}
}

void ImageViewerWidget::CrosshairyChanged(int i)
{
	if (i < m_Width)
	{
		m_Crosshairypos = i;

		if (m_Crosshairyvisible)
		{
			ReloadBits();
			repaint();
		}
	}
}

void ImageViewerWidget::SetCrosshairxvisible(bool on)
{
	if (m_Crosshairxvisible != on)
	{
		m_Crosshairxvisible = on;
		ReloadBits();
		repaint();
	}
}

void ImageViewerWidget::SetCrosshairyvisible(bool on)
{
	if (m_Crosshairyvisible != on)
	{
		m_Crosshairyvisible = on;
		ReloadBits();
		repaint();
	}
}

} // namespace iseg
