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

#include "Core/Pair.h"

#include "Data/Property.h"

#include <QWidget>

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

class QVTKWidget;
class QVTKInteractor;

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
	VolumeViewerWidget(SlicesHandler* hand3D1, bool bmportissue1 = true, bool raytraceortexturemap1 = true, bool shade1 = true, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~VolumeViewerWidget() override {}

protected:
	void closeEvent(QCloseEvent*) override;

	void ShadeChanged();
	void RaytraceortexturemapChanged();
	void ShowslicesChanged();
	void ShowvolumeChanged();
	void ContrbrightChanged();
	void TranspChanged();

public slots:
	void TissueChanged();
	void PixelsizeChanged(Pair p);
	void ThicknessChanged(float thick);
	void Reload();

signals:
	void Hasbeenclosed();

private:
	QVTKWidget* m_VtkWidget;
	bool m_Bmportissue;

	std::shared_ptr<PropertyBool> m_CbShade;
	std::shared_ptr<PropertyBool> m_CbRaytraceortexturemap;
	std::shared_ptr<PropertyBool> m_CbShowslices;
	std::shared_ptr<PropertyBool> m_CbShowslice1;
	std::shared_ptr<PropertyBool> m_CbShowslice2;
	std::shared_ptr<PropertyBool> m_CbShowvolume;

	std::shared_ptr<PropertyReal> m_SlContr;
	std::shared_ptr<PropertyReal> m_SlBright;
	std::shared_ptr<PropertyReal> m_SlTrans;
	std::shared_ptr<PropertyButton> m_BtUpdate;

	SlicesHandler* m_Hand3D;
	double m_Range[2];

	vtkSmartPointer<vtkCutter> m_SliceCutterY, m_SliceCutterZ;
	vtkSmartPointer<vtkPlane> m_SlicePlaneY, m_SlicePlaneZ;
	vtkSmartPointer<QVTKInteractor> m_Iren;

	vtkSmartPointer<vtkImageData> m_Input;
	vtkSmartPointer<vtkRenderer> m_Ren3D;
	vtkSmartPointer<vtkXMLImageDataReader> m_Reader;
	vtkSmartPointer<vtkOutlineFilter> m_OutlineGrid;
	vtkSmartPointer<vtkPolyDataMapper> m_OutlineMapper;
	vtkSmartPointer<vtkActor> m_OutlineActor;
	vtkSmartPointer<vtkImplicitPlaneWidget> m_PlaneWidgetY;
	vtkSmartPointer<vtkImplicitPlaneWidget> m_PlaneWidgetZ;
	vtkSmartPointer<vtkLookupTable> m_Lut;
	vtkSmartPointer<vtkPolyDataMapper> m_SliceY;
	vtkSmartPointer<vtkPolyDataMapper> m_SliceZ;
	vtkSmartPointer<vtkActor> m_SliceActorY;
	vtkSmartPointer<vtkActor> m_SliceActorZ;
	vtkSmartPointer<vtkScalarBarActor> m_ColorBar;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_Style;
	vtkSmartPointer<vtkRenderWindow> m_RenWin;
	vtkSmartPointer<vtkPiecewiseFunction> m_OpacityTransferFunction;
	vtkSmartPointer<vtkColorTransferFunction> m_ColorTransferFunction;
	vtkSmartPointer<vtkVolumeProperty> m_VolumeProperty;
	vtkSmartPointer<vtkSmartVolumeMapper> m_VolumeMapper;
	vtkSmartPointer<vtkVolume> m_Volume;
	vtkSmartPointer<vtkImageShiftScale> m_Cast;

	//	vtkSmartPointer<vtkRenderWindow> renWin;

	class SliceCallbackY : public vtkCommand
	{
	public:
		static SliceCallbackY* New() { return new SliceCallbackY; }
		void Delete() override { delete this; }
		void Execute(vtkObject* caller, unsigned long, void*) override;
		vtkSmartPointer<vtkPlane> m_SlicePlaneY1;
		vtkSmartPointer<vtkCutter> m_SliceCutterY1;
	};

	class SliceCallbackZ : public vtkCommand
	{
	public:
		static SliceCallbackZ* New() { return new SliceCallbackZ; }
		void Delete() override { delete this; }
		void Execute(vtkObject* caller, unsigned long, void*) override;
		vtkSmartPointer<vtkPlane> m_SlicePlaneZ1;
		vtkSmartPointer<vtkCutter> m_SliceCutterZ1;
	};

	vtkSmartPointer<VolumeViewerWidget::SliceCallbackY> m_MySliceDataY;
	vtkSmartPointer<VolumeViewerWidget::SliceCallbackZ> m_MySliceDataZ;
};

} // namespace iseg
