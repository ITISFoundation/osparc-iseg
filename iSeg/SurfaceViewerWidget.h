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

#include "Data/DataSelection.h"
#include "Data/Types.h"

#include "Core/Pair.h"

#include <QWidget>

#include <vtkSmartPointer.h>

#include <map>

class QVTKWidget;
class QVTKInteractor;
class QSlider;
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;

class vtkActor;
class vtkInteractorStyleTrackballCamera;
class vtkImageData;
class vtkFlyingEdges3D;
class vtkDiscreteFlyingEdges3D;
class vtkDecimatePro;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;
class vtkPropPicker;
class vtkLookupTable;

namespace iseg {

class SlicesHandler;

class SurfaceViewerWidget : public QWidget
{
	Q_OBJECT
public:
	enum eInputType { kSource,
		kTarget,
		kSelectedTissues };
	SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType input_type,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	virtual ~SurfaceViewerWidget();

	static bool isOpenGLSupported();

protected:
	void load();
	void build_lookuptable();
	int get_picked_tissue() const;
	void closeEvent(QCloseEvent*) override;
	//void resizeEvent(QResizeEvent*) override;

public slots:
	void tissue_changed();
	void pixelsize_changed(Pair p);
	void thickness_changed(float thick);
	void reload();
	void split_surface();

protected slots:
	void transp_changed();
	void thresh_changed();
	void reduction_changed();
	void popup(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	void select_action(QAction*);

signals:
	void hasbeenclosed();
	void begin_datachange(iseg::DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void end_datachange(QWidget* sender = nullptr, iseg::EndUndoAction undoAction = iseg::EndUndo);

private:
	eInputType input_type;
	SlicesHandler* hand3D;

	QVTKWidget* vtkWidget;
	QSlider* sl_trans;
	QSlider* sl_thresh;
	QLineEdit* reduction;
	QPushButton* bt_update;
	QPushButton* bt_connectivity;
	QLabel* lb_connectivity_count;

	vtkSmartPointer<QVTKInteractor> iren;
	vtkSmartPointer<vtkEventQtSlotConnect> connections;
	vtkSmartPointer<vtkPropPicker> picker;
	vtkSmartPointer<vtkImageData> input;
	vtkSmartPointer<vtkRenderer> ren3D;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> style;
	vtkSmartPointer<vtkDiscreteFlyingEdges3D> discreteCubes;
	vtkSmartPointer<vtkFlyingEdges3D> cubes;
	vtkSmartPointer<vtkDecimatePro> decimate;
	vtkSmartPointer<vtkPolyDataMapper> mapper;
	vtkSmartPointer<vtkActor> actor;
	vtkSmartPointer<vtkLookupTable> lut;

	double range[2];
	std::map<int, tissues_size_t> index_tissue_map;
	unsigned int startLabel;
	unsigned int endLabel;
};

} // namespace iseg
