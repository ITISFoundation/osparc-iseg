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

#include "Data/Mark.h"
#include "Data/Types.h"

#include "Core/Pair.h"

#include <QWidget>

#include <vector>

class Q3Action;

namespace iseg {

class bmphandler;
class SlicesHandler;

class ImageViewerWidget : public QWidget
{
	Q_OBJECT
public:
	ImageViewerWidget(QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~ImageViewerWidget();
	void init(SlicesHandler* hand3D, bool bmporwork);
	void update();
	void update(QRect rect);
	void update_range();
	void update_range(unsigned short slicenr);
	bool toggle_tissuevisible();
	bool toggle_picturevisible();
	bool toggle_markvisible();
	bool toggle_overlayvisible();
	bool toggle_workbordervisible();
	void set_crosshairxvisible(bool on);
	void set_crosshairyvisible(bool on);
	void get_scaleoffsetfactor(float& offset1, float& factor1);

	bool return_workbordervisible();

	void setIsBmp(bool isBmpOrNot) { isBmp = isBmpOrNot; }
	void setMousePosZoom(QPoint point) { mousePosZoom = point; }

protected:
	void paintEvent(QPaintEvent* e);
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);
	void contextMenuEvent(QContextMenuEvent* e);
	void update_scaleoffsetfactor();

signals:
	void addmark_sign(Point p);
	void addlabel_sign(Point p, std::string str);
	void clearmarks_sign();
	void removemark_sign(Point p);
	void addtissue_sign(Point p);
	void addtissueconnected_sign(Point p);
	void addtissuelarger_sign(Point p);
	void subtissue_sign(Point p);
	void addtissue3D_sign(Point p);
	void selecttissue_sign(Point p, bool clear_selection);
	void viewtissue_sign(Point p);
	void mousepressed_sign(Point p);
	void mousereleased_sign(Point p);
	void mousepressedmid_sign(Point p);
	void mousedoubleclick_sign(Point p);
	void mousedoubleclickmid_sign(Point p);
	void mousemoved_sign(Point p);
	void wheelrotated_sign(int delta);
	void wheelrotatedctrl_sign(int delta);
	void scaleoffsetfactor_changed(float scaleoffset1, float scalefactor1, bool bmporwork1);
	void setcenter_sign(int x, int y);
	void mousePosZoom_sign(QPoint mousePosZoom);

private:
	void reload_bits();
	void vp_to_image_decorator();
	void vp_changed();
	void vp_changed(QRect rect);
	void vpdyn_changed();
	void vp1dyn_changed();
	void mode_changed(unsigned char newmode, bool updatescale = true);

	QPainter* painter;
	unsigned char mode;
	float brightness;
	float contrast;
	float scaleoffset;
	float scalefactor;
	double zoom;
	bool crosshairxvisible;
	bool crosshairyvisible;
	int crosshairxpos;
	int crosshairypos;
	Pair pixelsize;
	QColor actual_color;

	QImage image;
	QImage image_decorated;

	unsigned short width, height;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	float** bmpbits;
	float* overlaybits;
	tissues_size_t** tissue;
	Pair range_mode1;
	bool tissuevisible;
	bool picturevisible;
	bool overlayvisible;
	float overlayalpha;
	bool markvisible;
	bool bmporwork;
	bool workborder;
	bool workborderlimit;
	bool isBmp;
	QPoint mousePosZoom;
	std::vector<Mark>* marks;
	int eventx, eventy;
	Q3Action* addmark;
	Q3Action* addlabel;
	Q3Action* clearmarks;
	Q3Action* removemark;
	Q3Action* addtissue;
	Q3Action* addtissueconnected;
	Q3Action* subtissue;
	Q3Action* addtissue3D;
	Q3Action* addtissuelarger;
	Q3Action* selecttissue;
	Q3Action* addtoselection;
	Q3Action* viewtissue;
	Q3Action* nexttargetslice;
	std::vector<Point> vp;
	std::vector<Point> vp_old;
	std::vector<Point> vp1;
	std::vector<Point> vp1_old;
	std::vector<Point> vpdyn;
	std::vector<Point> vpdyn_old;
	std::vector<Point> limit_points;
	std::vector<Mark> vm;
	std::vector<Mark> vm_old;

public slots:
	void set_brightnesscontrast(float bright, float contr, bool paint = true);
	void set_tissuevisible(bool on);
	void set_picturevisible(bool on);
	void set_markvisible(bool on);
	void set_overlayvisible(bool on);
	void set_overlayalpha(float alpha);
	void set_workbordervisible(bool on);
	void slicenr_changed();
	void tissue_changed();
	void tissue_changed(QRect rect);
	void zoom_in();
	void zoom_out();
	void unzoom();
	double return_zoom();
	void set_zoom(double z);
	void pixelsize_changed(Pair pixelsize1);

private slots:
	void bmphand_changed(bmphandler* bmph);
	void add_mark();
	void add_label();
	void clear_marks();
	void remove_mark();
	void add_tissue();
	void add_tissue_connected();
	void sub_tissue();
	void add_tissue_3D();
	void add_tissuelarger();
	void select_tissue();
	void view_tissue_surface();
	void next_target_slice();
	void add_to_selected_tissues();
	void mark_changed();
	void bmp_changed();
	void overlay_changed();
	void overlay_changed(QRect rect);
	void workborder_changed();
	void workborder_changed(QRect rect);
	void recompute_workborder();
	void set_vp1(std::vector<Point>* vp1_arg);
	void set_vm(std::vector<Mark>* vm_arg);
	void set_vpdyn(std::vector<Point>* vpdyn_arg);
	void set_vp1_dyn(std::vector<Point>* vp1_arg, std::vector<Point>* vpdyn_arg,
			const bool also_points = false);

public slots:
	void color_changed(int tissue);
	void crosshairx_changed(int i);
	void crosshairy_changed(int i);
};

} // namespace iseg
