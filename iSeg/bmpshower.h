/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef BMPSHOWER
#define BMPSHOWER

#include "Core/Mark.h"
#include "Core/Pair.h"
#include "Core/Point.h"
#include "Core/Types.h"

#include <q3action.h>
#include <q3vbox.h>
#include <qcheckbox.h>
#include <qevent.h> // BL TODO
#include <qimage.h>
#include <qlabel.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>

#include <vtkCommand.h>
#include <vtkCutter.h>
#include <vtkImplicitPlaneWidget.h>
#include <vtkPlane.h> // for custom command
#include <vtkSmartPointer.h>

#include <vector>

class QVTKWidget;
class QVTKInteractor;

class vtkImageData;
;
class vtkDataSet;
class vtkDataArray;
class vtkLookupTable;
class vtkWindowToImageFilter;
class vtkCommand;
class vtkBoxWidget;
class vtkImplicitPlaneWidget;
class vtkActor;
class vtkLODActor;
class vtkPiecewiseFunction;
class vtkColorTransferFunction;
class vtkScalarBarActor;
class vtkVolume;
class vtkVolumeProperty;
class vtkSmartVolumeMapper;
class vtkInteractorStyleTrackballCamera;
class vtkImageAccumulate;
class vtkImageShiftScale;
class vtkMarchingCubes;
class vtkDiscreteMarchingCubes;
class vtkWindowedSincPolyDataFilter;
class vtkThreshold;
class vtkMaskFields;
class vtkGeometryFilter;
class vtkPlane;
class vtkPlanes;
class vtkCutter;
class vtkOutlineFilter;
class vtkDataSetMapper;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindow;
class vtkXMLImageDataReader;
class vtkDataSetReader;

namespace iseg {

class bmphandler;
class SlicesHandler;

class bmpshower : public QWidget
{
	Q_OBJECT
public:
	bmpshower(QWidget* parent = 0, const char* name = 0,
			  Qt::WindowFlags wFlags = 0);
	void init(float** bmpbits1, unsigned short w, unsigned short h);
	void update();
	void update(unsigned short w, unsigned short h);

protected:
	void paintEvent(QPaintEvent* e);

private:
	void reload_bits();
	QImage image;

	unsigned short width, height;
	float** bmpbits;

private slots:
	void bmp_changed();
	void size_changed(unsigned short w, unsigned short h);
};

class bmptissueshower : public QWidget
{
	Q_OBJECT
public:
	bmptissueshower(QWidget* parent = 0, const char* name = 0,
					Qt::WindowFlags wFlags = 0);
	void init(float** bmpbits1, tissues_size_t** tissue1, unsigned short w,
			  unsigned short h);
	void update();
	void update(unsigned short w, unsigned short h);
	bool toggle_tissuevisible();
	void set_tissuevisible(bool on);

protected:
	void paintEvent(QPaintEvent* e);

private:
	void reload_bits();
	QImage image;

	unsigned short width, height;
	float** bmpbits;
	tissues_size_t** tissue;
	bool tissuevisible;

private slots:
	void bmp_changed();
	void size_changed(unsigned short w, unsigned short h);
	void tissue_changed();
};

class bmptissuemarkshower : public QWidget
{
	Q_OBJECT
public:
	bmptissuemarkshower(QWidget* parent = 0, const char* name = 0,
						Qt::WindowFlags wFlags = 0);
	~bmptissuemarkshower();
	void init(bmphandler* bmph1, tissuelayers_size_t layeridx, bool bmporwork1);
	void update();
	bool toggle_tissuevisible();
	bool toggle_markvisible();
	void set_tissuevisible(bool on);
	void set_markvisible(bool on);

protected:
	void paintEvent(QPaintEvent* e);
	void contextMenuEvent(QContextMenuEvent* event);

signals:
	//	void marks_changed();
	//	void tissues_changed();
	void addmark_sign(Point p);
	void addlabel_sign(Point p, std::string str);
	void removemark_sign(Point p);
	void addtissue_sign(Point p);
	void addtissueconnected_sign(Point p);
	void addtissuelarger_sign(Point p);
	void subtissue_sign(Point p);

private:
	void reload_bits();
	QImage image;

	unsigned short width, height;
	bmphandler* bmphand;
	float** bmpbits;
	tissues_size_t** tissue;
	bool tissuevisible;
	bool markvisible;
	bool bmporwork;
	std::vector<Mark>* marks;
	int eventx, eventy;
	Q3Action* addmark;
	Q3Action* addlabel;
	Q3Action* removemark;
	Q3Action* addtissue;
	Q3Action* subtissue;
	Q3Action* addtissueconnected;
	Q3Action* addtissuelarger;

private slots:
	void add_mark();
	void add_label();
	//	void clear_marks();
	void remove_mark();
	void add_tissue();
	void sub_tissue();
	void add_tissue_connected();
	void add_tissuelarger();
	void mark_changed();
	void bmp_changed();
	void tissue_changed();
};

class bmptissuemarklineshower : public QWidget
{
	Q_OBJECT
public:
	bmptissuemarklineshower(QWidget* parent = 0, const char* name = 0,
							Qt::WindowFlags wFlags = 0);
	~bmptissuemarklineshower();
	void init(SlicesHandler* hand3D, bool bmporwork1);
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
	/*	void set_tissuevisible(bool on);
		void set_markvisible(bool on);
		void set_workbordervisible(bool on);*/
	bool return_workbordervisible();
	//	QMouseEvent*e1;
	//	float e1;
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
	//	void marks_changed();
	//	void tissues_changed();
	void addmark_sign(Point p);
	void addlabel_sign(Point p, std::string str);
	void clearmarks_sign();
	void removemark_sign(Point p);
	void addtissue_sign(Point p);
	void addtissueconnected_sign(Point p);
	void addtissuelarger_sign(Point p);
	void subtissue_sign(Point p);
	void addtissue3D_sign(Point p);
	void selecttissue_sign(Point p);
	void mousepressed_sign(Point p);
	void mousereleased_sign(Point p);
	void mousepressedmid_sign(Point p);
	void mousedoubleclick_sign(Point p);
	void mousedoubleclickmid_sign(Point p);
	void mousemoved_sign(Point p);
	void wheelrotated_sign(int delta);
	void wheelrotatedctrl_sign(int delta);
	void scaleoffsetfactor_changed(float scaleoffset1, float scalefactor1,
								   bool bmporwork1);
	void setcenter_sign(int x, int y);
	void mousePosZoom_sign(QPoint mousePosZoom);

private:
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
	void reload_bits();
	void vp_to_image_decorator();
	void vp_changed();
	void vp_changed(QRect rect);
	void vpdyn_changed();
	void vp1dyn_changed();
	void mode_changed(unsigned char newmode, bool updatescale = true);
	//	void vp_changed1();
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
	//	bool showvp;
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
	void mark_changed();
	void bmp_changed();
	void overlay_changed();
	void overlay_changed(QRect rect);
	void workborder_changed();
	void workborder_changed(QRect rect);
	void recompute_workborder();
	void set_vp1(std::vector<Point>* vp1_arg);
	void clear_vp1();
	void set_vm(std::vector<Mark>* vm_arg);
	void clear_vm();
	void set_vpdyn(std::vector<Point>* vpdyn_arg);
	void set_vp1_dyn(std::vector<Point>* vp1_arg, std::vector<Point>* vpdyn_arg,
					 const bool also_points = false);
	void clear_vpdyn();

public slots:
	void color_changed(int tissue);
	void crosshairx_changed(int i);
	void crosshairy_changed(int i);
};

class surfaceviewer3D : public QWidget
{
	Q_OBJECT
public:
	surfaceviewer3D(SlicesHandler* hand3D1, bool bmportissue1 = true,
					QWidget* parent = 0, const char* name = 0,
					Qt::WindowFlags wFlags = 0);
	~surfaceviewer3D();
	std::string fnamei;
	QVTKWidget* vtkWidget;
	bool bmportissue;
	Q3VBox* vbox1;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	QPushButton* bt_update;
	QSlider* sl_trans;
	QLabel* lb_trans;
	QSlider* sl_thresh;
	QLabel* lb_thresh;

public slots:
	void tissue_changed();
	void pixelsize_changed(Pair p);
	void thickness_changed(float thick);
	void reload();

protected:
	void closeEvent(QCloseEvent*);
	void resizeEvent(QResizeEvent*);

protected slots:
	void transp_changed();
	void thresh_changed();

signals:
	void hasbeenclosed();

private:
	SlicesHandler* hand3D;
	double range[2];
	vtkSmartPointer<vtkActor> gridActor;
	vtkSmartPointer<vtkDataSetMapper> gridMapper;

	vtkSmartPointer<QVTKInteractor> iren;

	vtkSmartPointer<vtkImageData> input;
	vtkSmartPointer<vtkRenderer> ren3D;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> style;
	vtkSmartPointer<vtkRenderWindow> renWin;
	vtkSmartPointer<vtkImageAccumulate> histogram;
	vtkSmartPointer<vtkDiscreteMarchingCubes> discreteCubes;
	vtkSmartPointer<vtkMarchingCubes> cubes;
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother;
	std::vector<vtkSmartPointer<vtkThreshold>> selector;
	vtkSmartPointer<vtkMaskFields> scalarsOff;
	std::vector<vtkSmartPointer<vtkGeometryFilter>> geometry;
	std::vector<vtkSmartPointer<vtkPolyDataMapper>> PolyDataMapper;
	std::vector<vtkSmartPointer<vtkActor>> Actor;

	int plotOutline;
	double lineWidth;
	unsigned int startLabel;
	unsigned int endLabel;
	std::vector<unsigned int> indices;
};

class volumeviewer3D : public QWidget
{
	Q_OBJECT
public:
	volumeviewer3D(SlicesHandler* hand3D1, bool bmportissue1 = true,
				   bool raytraceortexturemap1 = true, bool shade1 = true,
				   QWidget* parent = 0, const char* name = 0,
				   Qt::WindowFlags wFlags = 0);
	~volumeviewer3D();
	std::string fnamei;
	QVTKWidget* vtkWidget;
	bool bmportissue;
	Q3VBox* vbox1;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	QCheckBox* cb_shade;
	QCheckBox* cb_raytraceortexturemap;
	QCheckBox* cb_showslices;
	QCheckBox* cb_showslice1;
	QCheckBox* cb_showslice2;
	QCheckBox* cb_showvolume;
	QSlider* sl_contr;
	QSlider* sl_bright;
	QSlider* sl_trans;
	QLabel* lb_contr;
	QLabel* lb_bright;
	QLabel* lb_trans;
	QPushButton* bt_update;

public slots:
	void tissue_changed();
	void pixelsize_changed(Pair p);
	void thickness_changed(float thick);
	void reload();

protected:
	void closeEvent(QCloseEvent*);
	void resizeEvent(QResizeEvent*);

protected slots:
	void shade_changed();
	void raytraceortexturemap_changed();
	void showslices_changed();
	void showvolume_changed();
	void contrbright_changed();
	void transp_changed();

signals:
	void hasbeenclosed();

private:
	SlicesHandler* hand3D;
	double range[2];
	vtkSmartPointer<vtkCutter> sliceCutterY, sliceCutterZ;
	vtkSmartPointer<vtkPlane> slicePlaneY, slicePlaneZ;

	vtkSmartPointer<QVTKInteractor> iren;

	vtkSmartPointer<vtkImageData> input;
	vtkSmartPointer<vtkRenderer> ren3D;
	vtkSmartPointer<vtkXMLImageDataReader> reader;
	vtkSmartPointer<vtkOutlineFilter> outlineGrid;
	vtkSmartPointer<vtkPolyDataMapper> outlineMapper;
	vtkSmartPointer<vtkActor> outlineActor;
	vtkSmartPointer<vtkImplicitPlaneWidget> planeWidgetY;
	vtkSmartPointer<vtkImplicitPlaneWidget> planeWidgetZ;
	vtkSmartPointer<vtkLookupTable> lut;
	vtkSmartPointer<vtkPolyDataMapper> sliceY;
	vtkSmartPointer<vtkPolyDataMapper> sliceZ;
	vtkSmartPointer<vtkActor> sliceActorY;
	vtkSmartPointer<vtkActor> sliceActorZ;
	vtkSmartPointer<vtkScalarBarActor> colorBar;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> style;
	vtkSmartPointer<vtkRenderWindow> renWin;
	vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction;
	vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction;
	vtkSmartPointer<vtkVolumeProperty> volumeProperty;
	vtkSmartPointer<vtkSmartVolumeMapper> volumeMapper;
	vtkSmartPointer<vtkVolume> volume;
	vtkSmartPointer<vtkImageShiftScale> cast;

	//	vtkSmartPointer<vtkRenderWindow> renWin;

	class vtkMySliceCallbackY : public vtkCommand
	{
	public:
		static vtkMySliceCallbackY* New() { return new vtkMySliceCallbackY; }
		void Delete() { delete this; }
		virtual void Execute(vtkObject* caller, unsigned long, void*)
		{
			cerr << "ExecuteY\n";
			//
			// The plane has moved, update the sampled data values.
			//
			vtkImplicitPlaneWidget* planeWidget =
				reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
			//planeWidget->GetPolyData( plane );
			planeWidget->GetPlane(slicePlaneY1);
			sliceCutterY1->SetCutFunction(slicePlaneY1);
		}
		vtkSmartPointer<vtkPlane> slicePlaneY1;
		vtkSmartPointer<vtkCutter> sliceCutterY1;
	};

	class vtkMySliceCallbackZ : public vtkCommand
	{
	public:
		static vtkMySliceCallbackZ* New() { return new vtkMySliceCallbackZ; }
		void Delete() { delete this; }
		virtual void Execute(vtkObject* caller, unsigned long, void*)
		{
			cerr << "ExecuteZ\n";
			//
			// The plane has moved, update the sampled data values.
			//
			vtkImplicitPlaneWidget* planeWidget =
				reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
			//planeWidget->GetPolyData( plane );
			planeWidget->GetPlane(slicePlaneZ1);
			sliceCutterZ1->SetCutFunction(slicePlaneZ1);
		}
		vtkSmartPointer<vtkPlane> slicePlaneZ1;
		vtkSmartPointer<vtkCutter> sliceCutterZ1;
	};

	vtkSmartPointer<volumeviewer3D::vtkMySliceCallbackY> my_sliceDataY;
	vtkSmartPointer<volumeviewer3D::vtkMySliceCallbackZ> my_sliceDataZ;
};

class iSAR_ShepInterpolate
{
public:
	void interpolate(int nx, int ny, double dx, double dy, double offset,
					 float* valin, float* valout);
};

} // namespace iseg

#endif
