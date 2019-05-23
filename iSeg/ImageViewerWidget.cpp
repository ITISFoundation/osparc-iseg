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

#include "ImageViewerWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Data/ExtractBoundary.h"
#include "Data/Point.h"

#include "Core/ColorLookupTable.h"

#include <QAction>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMenu>
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
		: QWidget(parent, name, wFlags), tissuevisible(true), picturevisible(true),
			markvisible(true), overlayvisible(false), workborder(false),
			crosshairxvisible(false), crosshairyvisible(false) //,showvp(false)
{
	brightness = scaleoffset = 0.0f;
	contrast = scalefactor = 1.0f;
	mode = 1;
	zoom = 1.0;
	pixelsize.high = pixelsize.low = 1.0f;
	workborderlimit = true;
	actual_color.setRgb(255, 255, 255);
	outline_color.setRgb(255, 255, 255);
	crosshairxpos = 0;
	crosshairypos = 0;
	marks = nullptr;
	overlayalpha = 0.0f;
	
	selecttissue = new QAction("Select Tissue", this);
	addtoselection = new QAction("Select Tissue", this);
	viewtissue = new QAction("View Tissue Surface", this);
	viewtarget = new QAction("View Target Surface", this);
	nexttargetslice = new QAction("Next Target Slice", this);
	addmark = new QAction("&Add Mark", this);
	addlabel = new QAction("Add &Label", this);
	removemark = new QAction("&Remove Mark", this);
	clearmarks = new QAction("&Clear Marks", this);
	addtissue = new QAction("Add &Tissue", this);
	addtissueconnected = new QAction("Add Tissue &Conn", this);
	addtissue3D = new QAction("Add Tissue 3&D", this);
	subtissue = new QAction("&Subtract Tissue", this);
	addtissuelarger = new QAction("Add Tissue &Larger", this);
	connect(addmark, SIGNAL(activated()), this, SLOT(add_mark()));
	connect(addlabel, SIGNAL(activated()), this, SLOT(add_label()));
	connect(clearmarks, SIGNAL(activated()), this, SLOT(clear_marks()));
	connect(removemark, SIGNAL(activated()), this, SLOT(remove_mark()));
	connect(addtissue, SIGNAL(activated()), this, SLOT(add_tissue()));
	connect(addtissueconnected, SIGNAL(activated()), this, SLOT(add_tissue_connected()));
	connect(subtissue, SIGNAL(activated()), this, SLOT(sub_tissue()));
	connect(addtissue3D, SIGNAL(activated()), this, SLOT(add_tissue_3D()));
	connect(addtissuelarger, SIGNAL(activated()), this, SLOT(add_tissuelarger()));
	connect(selecttissue, SIGNAL(activated()), this, SLOT(select_tissue()));
	connect(addtoselection, SIGNAL(activated()), this, SLOT(add_to_selected_tissues()));
	connect(viewtissue, SIGNAL(activated()), this, SLOT(view_tissue_surface()));
	connect(viewtarget, SIGNAL(activated()), this, SLOT(view_target_surface()));
	connect(nexttargetslice, SIGNAL(activated()), this, SLOT(next_target_slice()));
}

ImageViewerWidget::~ImageViewerWidget()
{
	delete addmark;
	delete addlabel;
	delete removemark;
	delete clearmarks;
	delete addtissue;
	delete addtissueconnected;
	delete addtissue3D;
	delete subtissue;
	delete addtissuelarger;
	delete selecttissue;
	delete addtoselection;
}

void ImageViewerWidget::mode_changed(unsigned char newmode, bool updatescale)
{
	if (newmode != 0 && mode != newmode)
	{
		mode = newmode;
		if (updatescale)
		{
			update_scaleoffsetfactor();
		}
	}
}

void ImageViewerWidget::get_scaleoffsetfactor(float& offset1, float& factor1)
{
	offset1 = scaleoffset;
	factor1 = scalefactor;
}

void ImageViewerWidget::set_zoom(double z)
{
	if (z != zoom)
	{
		//QPoint oldCenter = visibleRegion().boundingRect().center();
		QPoint oldCenter = rect().center();
		QPoint newCenter;
		if (mousePosZoom.x() == 0 && mousePosZoom.y() == 0)
		{
			newCenter = z * oldCenter / zoom;
		}
		else
		{
			newCenter = zoom * (oldCenter + z * mousePosZoom / zoom - mousePosZoom) / z;
		}

		zoom = z;
		int const w = static_cast<int>(width) * zoom * pixelsize.high;
		int const h = static_cast<int>(height) * zoom * pixelsize.low;
		setFixedSize(w, h);
		if (mousePosZoom.x() != 0 && mousePosZoom.y() != 0)
		{
			emit setcenter_sign(newCenter.x(), newCenter.y());
		}
	}
}

void ImageViewerWidget::pixelsize_changed(Pair pixelsize1)
{
	if (pixelsize1.high != pixelsize.high || pixelsize1.low != pixelsize.low)
	{
		pixelsize = pixelsize1;
		setFixedSize((int)width * zoom * pixelsize.high,
				(int)height * zoom * pixelsize.low);
		repaint();
	}
}

void ImageViewerWidget::paintEvent(QPaintEvent* e)
{
	marks = handler3D->get_activebmphandler()->return_marks();
	if (image.size() != QSize(0, 0)) // is an image loaded?
	{
		{
			QPainter painter(this);
			painter.setClipRect(e->rect());
			painter.scale(zoom * pixelsize.high, zoom * pixelsize.low);
			painter.drawImage(0, 0, image_decorated);
			painter.setPen(QPen(actual_color));

			if (marks != nullptr)
			{
				unsigned char r, g, b;
				for (auto& m : *marks)
				{
					std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
					QColor qc1(r, g, b);
					painter.setPen(QPen(qc1));

					painter.drawLine(
							int(m.p.px) - 2, int(height - m.p.py) - 3,
							int(m.p.px) + 2, int(height - m.p.py) + 1);
					painter.drawLine(
							int(m.p.px) - 2, int(height - m.p.py) + 1,
							int(m.p.px) + 2, int(height - m.p.py) - 3);
					if (!m.name.empty())
					{
						painter.drawText(int(m.p.px) + 3,
								int(height - m.p.py) + 1,
								QString(m.name.c_str()));
					}
				}
			}
		}
		{
			QPainter painter1(this);

			float dx = zoom * pixelsize.high;
			float dy = zoom * pixelsize.low;

			for (auto& p : vpdyn)
			{
				painter1.fillRect(
						int(dx * p.px), int(dy * (height - p.py - 1)),
						int(dx + 0.999f), int(dy + 0.999f), actual_color);
			}
		}
	}
}

void ImageViewerWidget::bmp_changed() { update(); }

void ImageViewerWidget::slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void ImageViewerWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	if (bmporwork)
		bmpbits = bmph->return_bmpfield();
	else
		bmpbits = bmph->return_workfield();
	tissue = bmph->return_tissuefield(handler3D->active_tissuelayer());
	marks = bmph->return_marks();

	mode_changed(bmph->return_mode(bmporwork), false);
	update_scaleoffsetfactor();

	reload_bits();
	if (workborder)
	{
		if (bmporwork)
		{
			workborder_changed();
		}
	}
	else
	{
		repaint();
	}
}

void ImageViewerWidget::overlay_changed()
{
	reload_bits();
	repaint();
}

void ImageViewerWidget::overlay_changed(QRect rect)
{
	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void ImageViewerWidget::update()
{
	QRect rect;
	rect.setLeft(0);
	rect.setTop(0);
	rect.setRight(width - 1);
	rect.setBottom(height - 1);
	update(rect);
}

void ImageViewerWidget::update(QRect rect)
{
	bmphand = handler3D->get_activebmphandler();
	overlaybits = handler3D->return_overlay();
	mode_changed(bmphand->return_mode(bmporwork), false);
	update_scaleoffsetfactor();
	if (bmporwork)
		bmpbits = bmphand->return_bmpfield();
	else
		bmpbits = bmphand->return_workfield();
	tissue = bmphand->return_tissuefield(handler3D->active_tissuelayer());
	marks = bmphand->return_marks();

	if (bmphand->return_width() != width || bmphand->return_height() != height)
	{
		vp.clear();
		vp_old.clear();
		vp1.clear();
		vp1_old.clear();
		vpdyn.clear();
		vpdyn_old.clear();
		vm.clear();
		vm_old.clear();
		width = bmphand->return_width();
		height = bmphand->return_height();
		image.create(int(width), int(height), 32);
		image_decorated.create(int(width), int(height), 32);
		setFixedSize((int)width * zoom * pixelsize.high,
				(int)height * zoom * pixelsize.low);

		if (bmporwork && workborder)
		{
			reload_bits();
			workborder_changed();
			return;
		}
	}

	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void ImageViewerWidget::init(SlicesHandler* hand3D, bool bmporwork1)
{
	handler3D = hand3D;
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	overlaybits = handler3D->return_overlay();
	bmporwork = bmporwork1;
	if (bmporwork)
		bmpbits = bmphand->return_bmpfield();
	else
		bmpbits = bmphand->return_workfield();
	tissue = bmphand->return_tissuefield(hand3D->active_tissuelayer());
	width = bmphand->return_width();
	height = bmphand->return_height();
	marks = bmphand->return_marks();
	image.create(int(width), int(height), 32);
	image_decorated.create(int(width), int(height), 32);

	setFixedSize((int)width * zoom * pixelsize.high,
			(int)height * zoom * pixelsize.low);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	mode_changed(bmphand->return_mode(bmporwork), false);
	update_range();
	update_scaleoffsetfactor();

	reload_bits();
	if (workborder)
	{
		if (bmporwork)
			workborder_changed();
		else
			repaint();
	}
	show();
}

void ImageViewerWidget::update_range()
{
	// Recompute ranges for all slices
	if (bmporwork)
	{
		handler3D->compute_bmprange_mode1(&range_mode1);
	}
	else
	{
		handler3D->compute_range_mode1(&range_mode1);
	}
}

void ImageViewerWidget::update_range(unsigned short slicenr)
{
	// Recompute range only for single slice
	if (bmporwork)
	{
		handler3D->compute_bmprange_mode1(slicenr, &range_mode1);
	}
	else
	{
		handler3D->compute_range_mode1(slicenr, &range_mode1);
	}
}

void ImageViewerWidget::reload_bits()
{
	auto color_lut = handler3D->GetColorLookupTable();

	float* bmpbits1 = *bmpbits;
	tissues_size_t* tissue1 = *tissue;
	unsigned pos = 0;
	int f;
	unsigned char r, g, b;

	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < width; x++)
		{
			if (picturevisible)
			{
				if (color_lut && bmporwork)
				{
					// \todo not sure if we should allow to 'scale & offset & clamp' when a color lut is available
					//f = max(0.0f, min(255.0f, scaleoffset + scalefactor * (bmpbits1)[pos]));
					color_lut->GetColor((bmpbits1)[pos], r, g, b);
				}
				else
				{
					r = g = b = (int)std::max(0.0f, std::min(255.0f, scaleoffset + scalefactor * (bmpbits1)[pos]));
				}

				// overlay only visible if picture is visible
				if (overlayvisible)
				{
					f = std::max(0.0f, std::min(255.0f, scaleoffset + scalefactor * (overlaybits)[pos]));

					r = (1.0f - overlayalpha) * r + overlayalpha * f;
					g = (1.0f - overlayalpha) * g + overlayalpha * f;
					b = (1.0f - overlayalpha) * b + overlayalpha * f;
				}
			}
			else
			{
				r = g = b = 0;
			}

			if (tissuevisible && tissue1[pos] != 0)
			{
				// blend with tissue color
				auto rgbo = TissueInfos::GetTissueColor(tissue1[pos]);
				float alpha = 0.5f;
				r = static_cast<unsigned char>(r + alpha * (255.0f * rgbo[0] - r));
				g = static_cast<unsigned char>(g + alpha * (255.0f * rgbo[1] - g));
				b = static_cast<unsigned char>(b + alpha * (255.0f * rgbo[2] - b));
				image.setPixel(x, y, qRgb(r, g, b));
			}
			else // no tissue
			{
				image.setPixel(x, y, qRgb(r, g, b));
			}
			pos++;
		}
	}

	// copy to decorated image
	image_decorated = image;

	// now decorate
	QRgb color_used = actual_color.rgb();
	QRgb color_dim = (actual_color.light(30)).rgb();

	if (workborder && bmporwork &&
			((!workborderlimit) || (unsigned)vp.size() < unsigned(width) * height / 5))
	{
		for (auto& p : vp)
		{
			image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_dim);
		}
	}

	for (auto& p : vp1)
	{
		image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_used);
	}

	for (auto& m : vm)
	{
		std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
		image_decorated.setPixel(int(m.p.px), int(height - m.p.py - 1), qRgb(r, g, b));
	}

	if (crosshairxvisible)
	{
		for (int x = 0; x < width; x++)
		{
			image_decorated.setPixel(x, height - 1 - crosshairxpos, qRgb(0, 255, 0));
			image.setPixel(x, height - 1 - crosshairxpos, qRgb(0, 255, 0));
		}
	}

	if (crosshairyvisible)
	{
		for (int y = 0; y < height; y++)
		{
			image_decorated.setPixel(crosshairypos, y, qRgb(0, 255, 0));
			image.setPixel(crosshairypos, y, qRgb(0, 255, 0));
		}
	}
}

void ImageViewerWidget::tissue_changed()
{
	reload_bits();
	repaint();
}

void ImageViewerWidget::tissue_changed(QRect rect)
{
	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void ImageViewerWidget::mark_changed()
{
	marks = bmphand->return_marks();
	repaint();
}

bool ImageViewerWidget::toggle_tissuevisible()
{
	tissuevisible = !tissuevisible;
	update();
	return tissuevisible;
}

bool ImageViewerWidget::toggle_picturevisible()
{
	picturevisible = !picturevisible;
	update();
	return picturevisible;
}

bool ImageViewerWidget::toggle_markvisible()
{
	markvisible = !markvisible;
	repaint();
	return markvisible;
}

bool ImageViewerWidget::toggle_overlayvisible()
{
	overlayvisible = !overlayvisible;
	update();
	return overlayvisible;
}

void ImageViewerWidget::set_tissuevisible(bool on)
{
	tissuevisible = on;
	update();
}

void ImageViewerWidget::set_picturevisible(bool on)
{
	picturevisible = on;
	update();
}

void ImageViewerWidget::set_markvisible(bool on)
{
	markvisible = on;
	repaint();
}

void ImageViewerWidget::set_overlayvisible(bool on)
{
	overlayvisible = on;
	update();
}

void ImageViewerWidget::set_overlayalpha(float alpha)
{
	overlayalpha = alpha;
	update();
}

void ImageViewerWidget::add_mark()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;

	emit addmark_sign(p);
}

void ImageViewerWidget::add_label()
{
	bool ok;
	QString newText = QInputDialog::getText("Label", "Enter a name for the label:", QLineEdit::Normal, "", &ok, this);
	if (ok)
	{
		Point p;
		p.px = (unsigned short)eventx;
		p.py = (unsigned short)eventy;
		emit addlabel_sign(p, newText.ascii());
	}
}

void ImageViewerWidget::clear_marks() { emit clearmarks_sign(); }

void ImageViewerWidget::remove_mark()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit removemark_sign(p);
}

void ImageViewerWidget::add_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissue_sign(p);
}

void ImageViewerWidget::add_tissue_connected()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissueconnected_sign(p);
}

void ImageViewerWidget::add_tissue_3D()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissue3D_sign(p);
}

void ImageViewerWidget::sub_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit subtissue_sign(p);
}

void ImageViewerWidget::add_tissuelarger()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissuelarger_sign(p);
}

void ImageViewerWidget::select_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit selecttissue_sign(p, true);
}

void ImageViewerWidget::view_tissue_surface()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit viewtissue_sign(p);
}
void ImageViewerWidget::view_target_surface()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit viewtarget_sign(p);
}

void ImageViewerWidget::next_target_slice()
{
	auto target_slices = handler3D->target_slices();
	size_t slice_size = handler3D->width() * handler3D->height();

	// find next slice
	int slice = -1;
	auto non_zero = [](float v) { return v != 0.f; };

	for (int s = handler3D->active_slice() + 1; s < handler3D->num_slices(); ++s)
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
		for (int s = 0; s <= handler3D->active_slice(); ++s)
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
		handler3D->set_active_slice(slice, true);
	}
}

void ImageViewerWidget::add_to_selected_tissues()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit selecttissue_sign(p, false);
}

void ImageViewerWidget::zoom_in() { set_zoom(2 * zoom); }

void ImageViewerWidget::zoom_out() { set_zoom(0.5 * zoom); }

void ImageViewerWidget::unzoom() { set_zoom(1.0); }

double ImageViewerWidget::return_zoom() { return zoom; }

void ImageViewerWidget::contextMenuEvent(QContextMenuEvent* event)
{
	eventx = (int)std::max(std::min(width - 1.0, (event->x() / (zoom * pixelsize.high))), 0.0);
	eventy = (int)std::max(std::min(height - 1.0, height - 1 - (event->y() / (zoom * pixelsize.low))), 0.0);

	QMenu contextMenu(this);
	// tissue selection
	if (event->modifiers() == Qt::ControlModifier)
	{
		contextMenu.addAction(addtoselection);
	}
	else
	{
		contextMenu.addAction(selecttissue);
	}

	// surface view
	if (bmporwork)
	{
		contextMenu.addAction(viewtissue);
	}
	else
	{
		contextMenu.addAction(viewtarget);
		contextMenu.addAction(nexttargetslice);

		// add to tissue
		contextMenu.insertSeparator();
		contextMenu.addAction(addtissue);
		contextMenu.addAction(subtissue);
		contextMenu.addAction(addtissue3D);
		contextMenu.addAction(addtissueconnected);
		contextMenu.addAction(addtissuelarger);
	}

	// marks
	contextMenu.insertSeparator();
	contextMenu.addAction(addmark);
	contextMenu.addAction(addlabel);
	contextMenu.addAction(removemark);
	contextMenu.addAction(clearmarks);

	contextMenu.exec(event->globalPos());
}

void ImageViewerWidget::set_brightnesscontrast(float bright, float contr, bool paint)
{
	brightness = bright;
	contrast = contr;
	update_scaleoffsetfactor();
	if (paint)
	{
		reload_bits();
		repaint();
	}
}

void ImageViewerWidget::update_scaleoffsetfactor()
{
	bmphandler* bmphand = handler3D->get_activebmphandler();
	if (bmporwork && handler3D->GetColorLookupTable())
	{
		// Disable scaling/offset for color mapped images, since it would break the lookup table
		scalefactor = 1.0f;
		scaleoffset = 0.0f;
	}
	else if (bmphand->return_mode(bmporwork) == 2)
	{
		// Mode 2 assumes the range [0, 255]
		scalefactor = contrast;
		scaleoffset = (127.5f - 255 * scalefactor) * (1.0f - brightness) + 127.5f * brightness;
	}
	else if (bmphand->return_mode(bmporwork) == 1)
	{
		// Mode 1 assumes an arbitrary range --> scale to range [0, 255]
		auto r = range_mode1;
		if (r.high == r.low)
		{
			r.high = r.low + 1.f;
		}
		scalefactor = 255.0f * contrast / (r.high - r.low);
		scaleoffset = (127.5f - r.high * scalefactor) * (1.0f - brightness) + (127.5f - r.low * scalefactor) * brightness;
	}
	emit scaleoffsetfactor_changed(scaleoffset, scalefactor, bmporwork);
}

void ImageViewerWidget::mousePressEvent(QMouseEvent* e)
{
	Point p;
	//	p.px=(unsigned short)(e->x()/(zoom*pixelsize.high));
	//	p.py=(unsigned short)height-1-(e->y()/(zoom*pixelsize.low));
	p.px = (unsigned short)std::max(std::min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))), 0.0);

	if (e->button() == Qt::LeftButton)
	{
		emit mousepressed_sign(p);
	}
	else if (e->button() == Qt::MidButton)
	{
		emit mousepressedmid_sign(p);
	}
}

void ImageViewerWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		Point p;
		p.px = (unsigned short)std::max(std::min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
		p.py = (unsigned short)std::max(std::min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))), 0.0);

		emit mousereleased_sign(p);
	}
}

void ImageViewerWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)std::max(std::min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))), 0.0);

	if (e->button() == Qt::LeftButton)
	{
		emit mousedoubleclick_sign(p);
	}
	else if (e->button() == Qt::MidButton)
	{
		emit mousedoubleclickmid_sign(p);
	}
}

void ImageViewerWidget::mouseMoveEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)std::max(std::min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)std::max(std::min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))), 0.0);

	emit mousemoved_sign(p);
}

void ImageViewerWidget::wheelEvent(QWheelEvent* e)
{
	int delta = e->delta();

	if (e->state() & Qt::ControlModifier)
	{
		mousePosZoom = e->pos();
		emit mousePosZoom_sign(mousePosZoom);
		emit wheelrotatedctrl_sign(delta);
	}
	else
	{
		e->ignore();
	}
}

void ImageViewerWidget::recompute_workborder()
{
	bmphand = handler3D->get_activebmphandler();
	vp = extract_boundary(bmphand->return_work(), width, height, Point());
}

void ImageViewerWidget::workborder_changed()
{
	if (workborder)
	{
		recompute_workborder();
		vp_changed();
	}
}

void ImageViewerWidget::workborder_changed(QRect rect)
{
	if (workborder)
	{
		recompute_workborder();
		vp_changed(rect);
	}
}

void ImageViewerWidget::vp_to_image_decorator()
{
	if ((!workborderlimit) || ((unsigned)vp_old.size() < unsigned(width) * height / 5))
	{
		for (auto& p : vp_old)
		{
			image_decorated.setPixel(int(p.px), int(height - p.py - 1),
					image.pixel(int(p.px), int(height - p.py - 1)));
		}
	}

	QRgb color_used = outline_color.rgb();
	QRgb color_dim = (outline_color.light(30)).rgb();
	if ((!workborderlimit) || ((unsigned)vp.size() < unsigned(width) * height / 5))
	{
		for (auto& p : vp)
		{
			image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_dim);
		}
	}

	for (auto& p : vp1_old)
	{
		image_decorated.setPixel(int(p.px), int(height - p.py - 1),
				image.pixel(int(p.px), int(height - p.py - 1)));
	}

	for (auto& p : vp1)
	{
		image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_used);
	}

	for (auto& m : vm_old)
	{
		image_decorated.setPixel(int(m.p.px), int(height - m.p.py - 1),
				image.pixel(int(m.p.px), int(height - m.p.py - 1)));
	}

	unsigned char r, g, b;
	for (auto& m : vm)
	{
		std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
		image_decorated.setPixel(int(m.p.px), int(height - m.p.py - 1), qRgb(r, g, b));
	}
}

void ImageViewerWidget::vp_changed()
{
	vp_to_image_decorator();

	repaint();

	vp_old = vp;
	vp1_old = vp1;
	vm_old = vm;
}

void ImageViewerWidget::vp_changed(QRect rect)
{
	vp_to_image_decorator();

	if (rect.left() > 0)
		rect.setLeft(rect.left() - 1);
	if (rect.top() > 0)
		rect.setTop(rect.top() - 1);
	if (rect.right() + 1 < width)
		rect.setRight(rect.right() + 1);
	if (rect.bottom() + 1 < height)
		rect.setBottom(rect.bottom() + 1);
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));

	vp_old = vp;
	vp1_old = vp1;
	vm_old = vm;
}

void ImageViewerWidget::vp1dyn_changed()
{
	if ((!workborderlimit) || ((unsigned)vp_old.size() < unsigned(width) * height / 5))
	{
		for (auto& p : vp_old)
		{
			image_decorated.setPixel(int(p.px), int(height - p.py - 1),
					image.pixel(int(p.px), int(height - p.py - 1)));
		}
	}

	QRgb color_used = actual_color.rgb();
	QRgb color_dim = (actual_color.light(30)).rgb();
	QRgb color_highlight = (actual_color.lighter(60)).rgb();
	if ((!workborderlimit) || ((unsigned)vp.size() < unsigned(width) * height / 5))
	{
		for (auto& p : vp)
		{
			image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_dim);
		}
	}

	for (auto& p : vp1_old)
	{
		image_decorated.setPixel(int(p.px), int(height - p.py - 1),
				image.pixel(int(p.px), int(height - p.py - 1)));
	}

	for (auto& p : vp1)
	{
		image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_used);
	}

	for (auto& p : limit_points)
	{
		image_decorated.setPixel(int(p.px), int(height - p.py - 1), color_highlight);
	}

	for (auto& m : vm_old)
	{
		image_decorated.setPixel(int(m.p.px), int(height - m.p.py - 1),
				image.pixel(int(m.p.px), int(height - m.p.py - 1)));
	}

	unsigned char r, g, b;
	for (auto& m : vm)
	{
		std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(m.mark);
		image_decorated.setPixel(int(m.p.px), int(height - m.p.py - 1), qRgb(r, g, b));
	}

	repaint();

	vpdyn_old = vpdyn;
	vp_old = vp;
	vp1_old = vp1;
	vm_old = vm;
}

void ImageViewerWidget::vpdyn_changed()
{
	repaint();
}

void ImageViewerWidget::set_workbordervisible(bool on)
{
	if (on)
	{
		if (bmporwork && !workborder)
		{
			workborder = true;
			workborder_changed();
		}
	}
	else
	{
		if (workborder && bmporwork)
		{
			vp.clear();
			workborder = false;
			reload_bits();
			repaint();
		}
	}
}

void ImageViewerWidget::set_outline_color(const QColor& c)
{
	if (outline_color != c)
	{
		outline_color = c;

		// this will trigger a repaint with the new color
		vp_changed();
	}
}

bool ImageViewerWidget::toggle_workbordervisible()
{
	if (workborder)
	{
		if (bmporwork)
		{
			vp.clear();
			workborder = false;
			reload_bits();
			repaint();
		}
	}
	else
	{
		if (bmporwork)
		{
			workborder = true;
			workborder_changed();
		}
	}

	return workborder;
}

bool ImageViewerWidget::return_workbordervisible() { return workborder; }

void ImageViewerWidget::set_vp1(std::vector<Point>* vp1_arg)
{
	vp1.clear();
	vp1.insert(vp1.begin(), vp1_arg->begin(), vp1_arg->end());
	vp_changed();
}

void ImageViewerWidget::set_vm(std::vector<Mark>* vm_arg)
{
	vm.clear();
	vm.insert(vm.begin(), vm_arg->begin(), vm_arg->end());
	vp_changed();
}

void ImageViewerWidget::set_vpdyn(std::vector<Point>* vpdyn_arg)
{
	vpdyn.clear();
	vpdyn.insert(vpdyn.begin(), vpdyn_arg->begin(), vpdyn_arg->end());
	vpdyn_changed();
}

void ImageViewerWidget::set_vp1_dyn(std::vector<Point>* vp1_arg,
		std::vector<Point>* vpdyn_arg,
		const bool also_points /*= false*/)
{
	vp1.clear();
	vp1.insert(vp1.begin(), vp1_arg->begin(), vp1_arg->end());
	vpdyn.clear();
	vpdyn.insert(vpdyn.begin(), vpdyn_arg->begin(), vpdyn_arg->end());
	limit_points.clear();
	if (also_points && vp1.size() > 1)
	{
		limit_points.push_back(vp1.front());
		limit_points.push_back(vp1.back());
	}
	vp1dyn_changed();
}

void ImageViewerWidget::color_changed(int tissue)
{
	unsigned char r, g, b;
	std::tie(r, g, b) = TissueInfos::GetTissueColorMapped(tissue + 1);
	actual_color.setRgb(r, g, b);
	vp_changed();
}

void ImageViewerWidget::crosshairx_changed(int i)
{
	if (i < height)
	{
		crosshairxpos = i;

		if (crosshairxvisible)
		{
			reload_bits();
			repaint();
		}
	}
}

void ImageViewerWidget::crosshairy_changed(int i)
{
	if (i < width)
	{
		crosshairypos = i;

		if (crosshairyvisible)
		{
			reload_bits();
			repaint();
		}
	}
}

void ImageViewerWidget::set_crosshairxvisible(bool on)
{
	if (crosshairxvisible != on)
	{
		crosshairxvisible = on;
		reload_bits();
		repaint();
	}
}

void ImageViewerWidget::set_crosshairyvisible(bool on)
{
	if (crosshairyvisible != on)
	{
		crosshairyvisible = on;
		reload_bits();
		repaint();
	}
}

}// namespace iseg