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

#include "Data/Types.h"

#include "Core/Pair.h"

#include <QWidget>

#include <vtkSmartPointer.h>

#include <map>

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
class vtkFlyingEdges3D;
class vtkDiscreteFlyingEdges3D;
class vtkWindowedSincPolyDataFilter;
class vtkThreshold;
class vtkMaskFields;
class vtkGeometryFilter;
class vtkDataSetMapper;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindow;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;
class vtkPropPicker;

namespace iseg {

class SlicesHandler;

class SurfaceViewerWidget : public QWidget
{
	Q_OBJECT
public:
	enum eInputType { kSource, kTarget, kTissues, kSelectedTissues };
	SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType input_type,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~SurfaceViewerWidget();

protected:
	eInputType input_type;
	std::map<int, tissues_size_t> index_tissue_map;

	std::string fnamei;
	QVTKWidget* vtkWidget;
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
	void popup(vtkObject* obj, unsigned long,
			void* client_data, void*,
			vtkCommand* command);
	void select_action(QAction*);

signals:
	void hasbeenclosed();

private:
	SlicesHandler* hand3D;
	double range[2];
	vtkSmartPointer<vtkActor> gridActor;
	vtkSmartPointer<vtkDataSetMapper> gridMapper;

	vtkSmartPointer<QVTKInteractor> iren;

	vtkSmartPointer<vtkEventQtSlotConnect> connections;
	vtkSmartPointer<vtkPropPicker> picker;

	vtkSmartPointer<vtkImageData> input;
	vtkSmartPointer<vtkRenderer> ren3D;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> style;
	vtkSmartPointer<vtkRenderWindow> renWin;
	vtkSmartPointer<vtkImageAccumulate> histogram;
	vtkSmartPointer<vtkDiscreteFlyingEdges3D> discreteCubes;
	vtkSmartPointer<vtkFlyingEdges3D> cubes;
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
