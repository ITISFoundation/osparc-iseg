/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

class QAction;

namespace iseg {

class Bmphandler;
class SlicesHandler;

class ImageViewerWidget : public QWidget
{
	Q_OBJECT
public:
	ImageViewerWidget(QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ImageViewerWidget() override;
	void Init(SlicesHandler* hand3D, bool bmporwork);
	void update();
	void update(QRect rect);
	void UpdateRange();
	void UpdateRange(unsigned short slicenr);
	bool ToggleTissuevisible();
	bool TogglePicturevisible();
	bool ToggleMarkvisible();
	bool ToggleOverlayvisible();
	bool ToggleWorkbordervisible();
	void SetCrosshairxvisible(bool on);
	void SetCrosshairyvisible(bool on);
	void GetScaleoffsetfactor(float& offset1, float& factor1) const;

	bool ReturnWorkbordervisible() const;

	void SetIsBmp(bool isBmpOrNot) { m_IsBmp = isBmpOrNot; }
	void SetMousePosZoom(QPoint point) { m_MousePosZoom = point; }

protected:
	void paintEvent(QPaintEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void wheelEvent(QWheelEvent* e) override;
	void contextMenuEvent(QContextMenuEvent* e) override;
	void UpdateScaleoffsetfactor();

signals:
	void AddmarkSign(Point p);
	void AddlabelSign(Point p, std::string str);
	void ClearmarksSign();
	void RemovemarkSign(Point p);
	void AddtissueSign(Point p);
	void AddtissueconnectedSign(Point p);
	void AddtissuelargerSign(Point p);
	void SubtissueSign(Point p);
	void Addtissue3DSign(Point p);
	void SelecttissueSign(Point p, bool clear_selection);
	void ViewtissueSign(Point p);
	void ViewtargetSign(Point p);
	void MousepressedSign(Point p);
	void MousereleasedSign(Point p);
	void MousepressedmidSign(Point p);
	void MousedoubleclickSign(Point p);
	void MousedoubleclickmidSign(Point p);
	void MousemovedSign(Point p);
	void WheelrotatedSign(int delta);
	void WheelrotatedctrlSign(int delta);
	void ScaleoffsetfactorChanged(float scaleoffset1, float scalefactor1, bool bmporwork1);
	void SetcenterSign(int x, int y);
	void MousePosZoomSign(QPoint mousePosZoom);

private:
	void ReloadBits();
	void VpToImageDecorator();
	void VpChanged();
	void VpChanged(QRect rect);
	void VpdynChanged();
	void Vp1dynChanged();
	void ModeChanged(unsigned char newmode, bool updatescale = true);

	QPainter* m_Painter;
	unsigned char m_Mode;
	float m_Brightness;
	float m_Contrast;
	float m_Scaleoffset;
	float m_Scalefactor;
	double m_Zoom;
	bool m_Crosshairxvisible = false;
	bool m_Crosshairyvisible = false;
	int m_Crosshairxpos;
	int m_Crosshairypos;
	Pair m_Pixelsize;
	QColor m_ActualColor;
	QColor m_OutlineColor;

	QImage m_Image;
	QImage m_ImageDecorated;

	unsigned short m_Width, m_Height;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	float** m_Bmpbits;
	float* m_Overlaybits;
	tissues_size_t** m_Tissue;
	Pair m_RangeMode1;
	bool m_Tissuevisible = true;
	bool m_Picturevisible = true;
	bool m_Overlayvisible = false;
	float m_Overlayalpha;
	bool m_Markvisible = true;
	bool m_Bmporwork;
	bool m_Workborder = false;
	bool m_Workborderlimit;
	bool m_IsBmp;
	QPoint m_MousePosZoom;
	std::vector<Mark>* m_Marks;
	int m_Eventx, m_Eventy;
	QAction* m_Addmark;
	QAction* m_Addlabel;
	QAction* m_Clearmarks;
	QAction* m_Removemark;
	QAction* m_Addtissue;
	QAction* m_Addtissueconnected;
	QAction* m_Subtissue;
	QAction* m_Addtissue3D;
	QAction* m_Addtissuelarger;
	QAction* m_Selecttissue;
	QAction* m_Addtoselection;
	QAction* m_Viewtissue;
	QAction* m_Viewtarget;
	QAction* m_Nexttargetslice;
	std::vector<Point> m_Vp;
	std::vector<Point> m_VpOld;
	std::vector<Point> m_Vp1;
	std::vector<Point> m_Vp1Old;
	std::vector<Point> m_Vpdyn;
	std::vector<Point> m_VpdynOld;
	std::vector<Point> m_LimitPoints;
	std::vector<Mark> m_Vm;
	std::vector<Mark> m_VmOld;

public slots:
	void SetBrightnesscontrast(float bright, float contr, bool paint = true);
	void SetTissuevisible(bool on);
	void SetPicturevisible(bool on);
	void SetMarkvisible(bool on);
	void SetOverlayvisible(bool on);
	void SetOverlayalpha(float alpha);
	void SetWorkbordervisible(bool on);
	void SetOutlineColor(const QColor&);
	void SlicenrChanged();
	void TissueChanged();
	void TissueChanged(QRect rect);
	void ZoomIn();
	void ZoomOut();
	void Unzoom();
	double ReturnZoom() const;
	void SetZoom(double z);
	void PixelsizeChanged(Pair pixelsize1);

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void AddMark();
	void AddLabel();
	void ClearMarks();
	void RemoveMark();
	void AddTissue();
	void AddTissueConnected();
	void SubTissue();
	void AddTissue3D();
	void AddTissuelarger();
	void SelectTissue();
	void ViewTissueSurface();
	void ViewTargetSurface();
	void NextTargetSlice();
	void AddToSelectedTissues();
	void MarkChanged();
	void BmpChanged();
	void OverlayChanged();
	void OverlayChanged(QRect rect);
	void WorkborderChanged();
	void WorkborderChanged(QRect rect);
	void RecomputeWorkborder();
	void SetVp1(std::vector<Point>* vp1_arg);
	void SetVm(std::vector<Mark>* vm_arg);
	void SetVpdyn(std::vector<Point>* vpdyn_arg);
	void SetVp1Dyn(std::vector<Point>* vp1_arg, std::vector<Point>* vpdyn_arg, bool also_points = false);

public slots:
	void ColorChanged(int tissue);
	void CrosshairxChanged(int i);
	void CrosshairyChanged(int i);
};

} // namespace iseg
