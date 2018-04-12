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
#include "Core/Types.h"

#include <QWidget>

#include <vtkSmartPointer.h>

class QVTKWidget;
class QVTKInteractor;
class Q3VBox;
class Q3HBox;
class QSlider;
class QLabel;
class QPushButton;

class vtkActor;
class vtkInteractorStyleTrackballCamera;
class vtkImageData;
class vtkImageAccumulate;
class vtkMarchingCubes;
class vtkDiscreteMarchingCubes;
class vtkWindowedSincPolyDataFilter;
class vtkThreshold;
class vtkMaskFields;
class vtkGeometryFilter;
class vtkDataSetMapper;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindow;

namespace iseg {

class SlicesHandler;

class SurfaceViewerWidget : public QWidget
{
	Q_OBJECT
public:
	SurfaceViewerWidget(SlicesHandler* hand3D1, bool bmportissue1 = true,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~SurfaceViewerWidget();
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

} // namespace iseg
