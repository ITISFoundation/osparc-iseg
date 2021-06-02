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
	SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType input_type, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~SurfaceViewerWidget() override;

	static bool IsOpenGlSupported();

protected:
	void Load();
	void BuildLookuptable();
	int GetPickedTissue() const;
	void closeEvent(QCloseEvent*) override;
	//void resizeEvent(QResizeEvent*) override;

public slots:
	void TissueChanged();
	void PixelsizeChanged(Pair p);
	void ThicknessChanged(float thick);
	void Reload();
	void SplitSurface();

protected slots:
	void TranspChanged();
	void ThreshChanged();
	void ReductionChanged();
	void Popup(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	void SelectAction(QAction*);

signals:
	void Hasbeenclosed();
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);

private:
	eInputType m_InputType;
	SlicesHandler* m_Hand3D;

	QVTKWidget* m_VtkWidget;
	QSlider* m_SlTrans;
	QSlider* m_SlThresh;
	QLineEdit* m_Reduction;
	QPushButton* m_BtUpdate;
	QPushButton* m_BtConnectivity;
	QLabel* m_LbConnectivityCount;

	vtkSmartPointer<QVTKInteractor> m_Iren;
	vtkSmartPointer<vtkEventQtSlotConnect> m_Connections;
	vtkSmartPointer<vtkPropPicker> m_Picker;
	vtkSmartPointer<vtkImageData> m_Input;
	vtkSmartPointer<vtkRenderer> m_Ren3D;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_Style;
	vtkSmartPointer<vtkDiscreteFlyingEdges3D> m_DiscreteCubes;
	vtkSmartPointer<vtkFlyingEdges3D> m_Cubes;
	vtkSmartPointer<vtkDecimatePro> m_Decimate;
	vtkSmartPointer<vtkPolyDataMapper> m_Mapper;
	vtkSmartPointer<vtkActor> m_Actor;
	vtkSmartPointer<vtkLookupTable> m_Lut;

	double m_Range[2];
	std::map<int, tissues_size_t> m_IndexTissueMap;
	unsigned int m_StartLabel;
	unsigned int m_EndLabel;
};

} // namespace iseg
