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

#include "Core/Pair.h"

#include <QWidget>

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

class QVTKWidget;
class QVTKInteractor;
class Q3VBox;
class Q3HBox;
class QSlider;
class QLabel;
class QCheckBox;
class QPushButton;

class vtkLookupTable;
class vtkImplicitPlaneWidget;
class vtkActor;
class vtkPiecewiseFunction;
class vtkColorTransferFunction;
class vtkScalarBarActor;
class vtkImageData;
class vtkVolume;
class vtkVolumeProperty;
class vtkSmartVolumeMapper;
class vtkInteractorStyleTrackballCamera;
class vtkImageShiftScale;
class vtkPlane;
class vtkCutter;
class vtkOutlineFilter;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindow;
class vtkXMLImageDataReader;

namespace iseg {

class SlicesHandler;

class VolumeViewerWidget : public QWidget
{
	Q_OBJECT
public:
	VolumeViewerWidget(SlicesHandler* hand3D1, bool bmportissue1 = true,
			bool raytraceortexturemap1 = true, bool shade1 = true,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~VolumeViewerWidget();
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
		virtual void Execute(vtkObject* caller, unsigned long, void*);
		vtkSmartPointer<vtkPlane> slicePlaneY1;
		vtkSmartPointer<vtkCutter> sliceCutterY1;
	};

	class vtkMySliceCallbackZ : public vtkCommand
	{
	public:
		static vtkMySliceCallbackZ* New() { return new vtkMySliceCallbackZ; }
		void Delete() { delete this; }
		virtual void Execute(vtkObject* caller, unsigned long, void*);
		vtkSmartPointer<vtkPlane> slicePlaneZ1;
		vtkSmartPointer<vtkCutter> sliceCutterZ1;
	};

	vtkSmartPointer<VolumeViewerWidget::vtkMySliceCallbackY> my_sliceDataY;
	vtkSmartPointer<VolumeViewerWidget::vtkMySliceCallbackZ> my_sliceDataZ;
};

} // namespace iseg
