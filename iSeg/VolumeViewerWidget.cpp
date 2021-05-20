/*
* Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#include "Precompiled.h"

#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "VolumeViewerWidget.h"

#include "QVTKWidget.h"

#include "Interface/QtConnect.h"

#include <Q3VBox>
#include <QResizeEvent>

#include <vtkActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkCutter.h>
#include <vtkImageShiftScale.h>
#include <vtkImplicitPlaneWidget.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

namespace iseg {

VolumeViewerWidget::VolumeViewerWidget(SlicesHandler* hand3D1, bool bmportissue1, bool gpu_or_raycast, bool shade1, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	m_Bmportissue = bmportissue1;
	m_Hand3D = hand3D1;
	m_Vbox1 = new Q3VBox(this);
	m_VtkWidget = new QVTKWidget(m_Vbox1);
	m_VtkWidget->setFixedSize(600, 600);
	//	resize( QSize(600, 600).expandedTo(minimumSizeHint()) );

	m_CbShade = new QCheckBox("Shade", m_Vbox1);
	m_CbShade->setChecked(shade1);
	m_CbShade->show();
	m_CbRaytraceortexturemap = new QCheckBox("GPU / Software", m_Vbox1);
	m_CbRaytraceortexturemap->setChecked(gpu_or_raycast);
	m_CbRaytraceortexturemap->show();

	m_CbShowslices = new QCheckBox("Show Slices", m_Vbox1);
	m_CbShowslices->setChecked(true);
	m_CbShowslices->show();
	m_CbShowslice1 = new QCheckBox("Enable Slicer 1", m_Vbox1);
	m_CbShowslice1->setChecked(false);
	m_CbShowslice1->show();
	m_CbShowslice2 = new QCheckBox("Enable Slicer 2", m_Vbox1);
	m_CbShowslice2->setChecked(false);
	m_CbShowslice2->show();
	m_CbShowvolume = new QCheckBox("Show Volume", m_Vbox1);
	m_CbShowvolume->setChecked(true);
	m_CbShowvolume->show();

	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_LbContr = new QLabel("Window:", m_Hbox1);
	m_SlContr = new QSlider(Qt::Horizontal, m_Hbox1);
	m_SlContr->setRange(0, 100);
	m_SlContr->setValue(100);
	m_Hbox1->setFixedHeight(m_Hbox1->sizeHint().height());
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_LbBright = new QLabel("Level:", m_Hbox2);
	m_SlBright = new QSlider(Qt::Horizontal, m_Hbox2);
	m_SlBright->setRange(0, 100);
	m_SlBright->setValue(50);
	m_Hbox2->setFixedHeight(m_Hbox2->sizeHint().height());
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_LbTrans = new QLabel("Transp.:", m_Hbox3);
	m_SlTrans = new QSlider(Qt::Horizontal, m_Hbox3);
	m_SlTrans->setRange(0, 100);
	m_SlTrans->setValue(50);
	m_Hbox3->setFixedHeight(m_Hbox3->sizeHint().height());

	m_BtUpdate = new QPushButton("Update", m_Vbox1);
	m_BtUpdate->show();

	if (m_Bmportissue)
	{
		m_Hbox3->hide();
		m_Hbox1->show();
		m_Hbox2->show();
	}
	else
	{
		m_Hbox3->show();
		m_Hbox1->hide();
		m_Hbox2->hide();
	}

	m_Vbox1->show();

	//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	//	vbox1->setFixedSize(vbox1->sizeHint());
	resize(m_Vbox1->sizeHint().expandedTo(minimumSizeHint()));

	QObject_connect(m_CbShade, SIGNAL(clicked()), this, SLOT(ShadeChanged()));
	QObject_connect(m_CbRaytraceortexturemap, SIGNAL(clicked()), this, SLOT(RaytraceortexturemapChanged()));
	QObject_connect(m_BtUpdate, SIGNAL(clicked()), this, SLOT(Reload()));
	QObject_connect(m_CbShowslices, SIGNAL(clicked()), this, SLOT(ShowslicesChanged()));
	QObject_connect(m_CbShowslice1, SIGNAL(clicked()), this, SLOT(ShowslicesChanged()));
	QObject_connect(m_CbShowslice2, SIGNAL(clicked()), this, SLOT(ShowslicesChanged()));
	QObject_connect(m_CbShowvolume, SIGNAL(clicked()), this, SLOT(ShowvolumeChanged()));

	QObject_connect(m_SlBright, SIGNAL(sliderReleased()), this, SLOT(ContrbrightChanged()));
	QObject_connect(m_SlContr, SIGNAL(sliderReleased()), this, SLOT(ContrbrightChanged()));
	QObject_connect(m_SlTrans, SIGNAL(sliderReleased()), this, SLOT(TranspChanged()));

	//   vtkStructuredPointsReader* reader = vtkStructuredPointsReader::New();
	//  reader= vtkSmartPointer<vtkXMLImageDataReader>::New();
	//  fnamei="D:\\Development\\bmp_read_QT\\bmp_read_3D_8\\qvtkViewer\\test2.vti";
	//  reader->SetFileName(fnamei.c_str());

	//  vtkImageData* input = (vtkImageData*)reader->GetOutput();
	//  input->Update();

	m_Input = vtkSmartPointer<vtkImageData>::New();
	m_Input->SetExtent(0, (int)m_Hand3D->Width() - 1, 0, (int)m_Hand3D->Height() - 1, 0, (int)m_Hand3D->NumSlices() - 1);
	Pair ps = m_Hand3D->GetPixelsize();
	if (m_Bmportissue)
	{
		m_Input->SetSpacing(ps.high, ps.low, m_Hand3D->GetSlicethickness());
		m_Input->AllocateScalars(VTK_FLOAT, 1);
		float* field = (float*)m_Input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < m_Hand3D->NumSlices(); i++)
		{
			m_Hand3D->Copyfrombmp(i, &(field[i * (unsigned long long)m_Hand3D->ReturnArea()]));
		}
	}
	else
	{
		switch (sizeof(tissues_size_t))
		{
		case 1: m_Input->AllocateScalars(VTK_UNSIGNED_CHAR, 1); break;
		case 2: m_Input->AllocateScalars(VTK_UNSIGNED_SHORT, 1); break;
		default:
			std::cerr << "volumeviewer3D::volumeviewer3D: tissues_size_t not "
									 "implemented."
								<< endl;
		}
		m_Input->SetSpacing(ps.high, ps.low, m_Hand3D->GetSlicethickness());
		tissues_size_t* field =
				(tissues_size_t*)m_Input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < m_Hand3D->NumSlices(); i++)
		{
			m_Hand3D->Copyfromtissue(i, &(field[i * (unsigned long long)m_Hand3D->ReturnArea()]));
		}
	}

	double bounds[6], center[3];

	m_Input->GetBounds(bounds);
	m_Input->GetCenter(center);
	m_Input->GetScalarRange(m_Range);
	std::cerr << "input range = " << m_Range[0] << " " << m_Range[1] << endl;

	double level = 0.5 * (m_Range[1] + m_Range[0]);
	double window = m_Range[1] - m_Range[0];
	if (m_Range[1] > 250)
	{
		level = 125;
		window = 250;
		m_SlContr->setValue((101 * window / (m_Range[1] - m_Range[0])) - 1);
		m_SlBright->setValue(100 * (level - m_Range[0]) / (m_Range[1] - m_Range[0]));
	}

	//
	// Put a bounding box in the scene to help orient the user.
	//
	m_OutlineGrid = vtkSmartPointer<vtkOutlineFilter>::New();
	m_OutlineGrid->SetInputData(m_Input);
	m_OutlineGrid->Update();
	m_OutlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_OutlineMapper->SetInputConnection(m_OutlineGrid->GetOutputPort());
	m_OutlineActor = vtkSmartPointer<vtkActor>::New();
	m_OutlineActor->SetMapper(m_OutlineMapper);

	// Add a PlaneWidget to interactively move the slice plane.
	m_PlaneWidgetY = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
	m_PlaneWidgetY->SetInputData(m_Input);
	m_PlaneWidgetY->NormalToYAxisOn();
	m_PlaneWidgetY->DrawPlaneOff();
	m_PlaneWidgetY->SetScaleEnabled(0);
	m_PlaneWidgetY->SetOriginTranslation(0);
	m_PlaneWidgetY->SetOutsideBounds(0);

	// Attach a Cutter to the slice plane to sample the function.
	m_SliceCutterY = vtkSmartPointer<vtkCutter>::New();
	m_SliceCutterY->SetInputData(m_Input);
	m_SlicePlaneY = vtkSmartPointer<vtkPlane>::New();
	m_PlaneWidgetY->GetPlane(m_SlicePlaneY);
	m_SliceCutterY->SetCutFunction(m_SlicePlaneY);

	// Add a PlaneWidget to interactively move the slice plane.
	m_PlaneWidgetZ = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
	m_PlaneWidgetZ->SetInputData(m_Input);
	m_PlaneWidgetZ->NormalToZAxisOn();
	m_PlaneWidgetZ->DrawPlaneOff();
	m_PlaneWidgetZ->SetScaleEnabled(0);
	m_PlaneWidgetZ->SetOriginTranslation(0);
	m_PlaneWidgetZ->SetOutsideBounds(0);

	// Attach a Cutter to the slice plane to sample the function.
	m_SliceCutterZ = vtkSmartPointer<vtkCutter>::New();
	m_SliceCutterZ->SetInputData(m_Input);
	m_SlicePlaneZ = vtkSmartPointer<vtkPlane>::New();
	m_PlaneWidgetZ->GetPlane(m_SlicePlaneZ);
	m_SliceCutterZ->SetCutFunction(m_SlicePlaneZ);

	//
	// Create a lookup table to map the velocity magnitudes.
	//
	if (m_Bmportissue)
	{
		m_Lut = vtkSmartPointer<vtkLookupTable>::New();
		m_Lut->SetTableRange(m_Range);
		m_Lut->SetHueRange(0, 0);
		m_Lut->SetSaturationRange(0, 0);
		m_Lut->SetValueRange(0, 1);
		m_Lut->Build(); //effective build
	}
	else
	{
		m_Lut = vtkSmartPointer<vtkLookupTable>::New();
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		m_Lut->SetNumberOfColors(tissuecount + 1);
		m_Lut->Build();
		m_Lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			m_Lut->SetTableValue(i, tissuecolor[0], tissuecolor[1], tissuecolor[2], 1.0);
		}
	}
	//
	// Map the slice plane and create the geometry to be rendered.
	//
	m_SliceY = vtkSmartPointer<vtkPolyDataMapper>::New();
	if (!m_Bmportissue)
		m_SliceY->SetColorModeToMapScalars();
	m_SliceY->SetInputConnection(m_SliceCutterY->GetOutputPort());
	//   input->GetScalarRange( range );
	//range[1] *= 0.7; // reduce the upper range by 30%
	m_SliceY->SetScalarRange(m_Range);
	m_SliceY->SetLookupTable(m_Lut);
	//   sliceY->SelectColorArray(argv[2]);
	m_SliceY->Update();
	m_SliceActorY = vtkSmartPointer<vtkActor>::New();
	m_SliceActorY->SetMapper(m_SliceY);

	m_SliceZ = vtkSmartPointer<vtkPolyDataMapper>::New();
	if (!m_Bmportissue)
		m_SliceZ->SetColorModeToMapScalars();
	m_SliceZ->SetInputConnection(m_SliceCutterZ->GetOutputPort());
	//   input->GetScalarRange( range );
	//range[1] *= 0.7; // reduce the upper range by 30%
	m_SliceZ->SetScalarRange(m_Range);
	m_SliceZ->SetLookupTable(m_Lut);
	m_SliceZ->Update();
	m_SliceActorZ = vtkSmartPointer<vtkActor>::New();
	m_SliceActorZ->SetMapper(m_SliceZ);

	if (!m_Bmportissue)
	{
		m_OpacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		m_OpacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (m_SlTrans->value() > 50)
				opac1 = opac1 * (100 - m_SlTrans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * m_SlTrans->value() / 50;
			m_OpacityTransferFunction->AddPoint(i, opac1);
		}

		m_ColorTransferFunction =
				vtkSmartPointer<vtkColorTransferFunction>::New();
		m_ColorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			m_ColorTransferFunction->AddRGBPoint((double)i - 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
		}
		auto tissuecolor = TissueInfos::GetTissueColor(tissuecount);
		m_ColorTransferFunction->AddRGBPoint((double)tissuecount + 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
	}
	else
	{
		m_OpacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		m_OpacityTransferFunction->AddPoint(level - window / 2, 0.0);
		m_OpacityTransferFunction->AddPoint(level + window / 2, 1.0);
		m_ColorTransferFunction =
				vtkSmartPointer<vtkColorTransferFunction>::New();
		m_ColorTransferFunction->AddRGBPoint(level - window / 2, 0.0, 0.0, 0.0);
		m_ColorTransferFunction->AddRGBPoint(level + window / 2, 1.0, 1.0, 1.0);
	}

	m_VolumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
	m_VolumeProperty->SetColor(m_ColorTransferFunction);
	m_VolumeProperty->SetScalarOpacity(m_OpacityTransferFunction);
	if (shade1)
		m_VolumeProperty->ShadeOn();
	else
		m_VolumeProperty->ShadeOff();
	if (m_Bmportissue)
		m_VolumeProperty->SetInterpolationTypeToLinear();
	else
		m_VolumeProperty->SetInterpolationTypeToNearest();

	m_VolumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	m_VolumeMapper->SetRequestedRenderModeToDefault();

	m_Cast = vtkSmartPointer<vtkImageShiftScale>::New();
	if (m_Bmportissue)
	{
		double slope = 255 / (m_Range[1] - m_Range[0]);
		double shift = -m_Range[0];
		m_Cast->SetInputData(m_Input);
		m_Cast->SetShift(shift);
		m_Cast->SetScale(slope);
		m_Cast->SetOutputScalarTypeToUnsignedShort();
		m_VolumeMapper->SetInputConnection(m_Cast->GetOutputPort());
	}
	else
	{
		m_VolumeMapper->SetInputData(m_Input);
	}

	m_Volume = vtkSmartPointer<vtkVolume>::New();
	if (gpu_or_raycast)
		m_VolumeMapper->SetRequestedRenderModeToDefault();
	else
		m_VolumeMapper->SetRequestedRenderModeToRayCast();

	m_Volume->SetMapper(m_VolumeMapper);
	m_Volume->SetProperty(m_VolumeProperty);

	//
	// Create the renderers and assign actors to them.
	//
	m_Ren3D = vtkSmartPointer<vtkRenderer>::New();
	m_Ren3D->AddActor(m_SliceActorY);
	m_Ren3D->AddActor(m_SliceActorZ);
	m_Ren3D->AddVolume(m_Volume);

	m_Ren3D->AddActor(m_OutlineActor);
	m_Ren3D->SetBackground(0, 0, 0);
	m_Ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);

	m_RenWin = vtkSmartPointer<vtkRenderWindow>::New();

	m_VtkWidget->GetRenderWindow()->AddRenderer(m_Ren3D);
	m_VtkWidget->GetRenderWindow()->LineSmoothingOn();
	m_VtkWidget->GetRenderWindow()->SetSize(600, 600);

	m_Style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

	m_Iren = vtkSmartPointer<QVTKInteractor>::New();
	m_Iren->SetInteractorStyle(m_Style);

	m_Iren->SetRenderWindow(m_VtkWidget->GetRenderWindow());

	m_MySliceDataY = vtkSmartPointer<SliceCallbackY>::New();
	m_MySliceDataY->m_SlicePlaneY1 = m_SlicePlaneY;
	m_MySliceDataY->m_SliceCutterY1 = m_SliceCutterY;
	m_PlaneWidgetY->SetInteractor(m_Iren);
	m_PlaneWidgetY->AddObserver(vtkCommand::InteractionEvent, m_MySliceDataY);
	m_PlaneWidgetY->AddObserver(vtkCommand::EnableEvent, m_MySliceDataY);
	m_PlaneWidgetY->AddObserver(vtkCommand::StartInteractionEvent, m_MySliceDataY);
	m_PlaneWidgetY->AddObserver(vtkCommand::EndPickEvent, m_MySliceDataY);
	m_PlaneWidgetY->SetPlaceFactor(1.0);
	m_PlaneWidgetY->PlaceWidget();
	m_PlaneWidgetY->SetOrigin(center);
	m_PlaneWidgetY->SetEnabled(m_CbShowslice1->isChecked());
	m_PlaneWidgetY->PlaceWidget(bounds);

	m_MySliceDataZ = vtkSmartPointer<SliceCallbackZ>::New();
	m_MySliceDataZ->m_SlicePlaneZ1 = m_SlicePlaneZ;
	m_MySliceDataZ->m_SliceCutterZ1 = m_SliceCutterZ;
	m_PlaneWidgetZ->SetInteractor(m_Iren);
	m_PlaneWidgetZ->AddObserver(vtkCommand::InteractionEvent, m_MySliceDataZ);
	m_PlaneWidgetZ->AddObserver(vtkCommand::EnableEvent, m_MySliceDataZ);
	m_PlaneWidgetZ->AddObserver(vtkCommand::StartInteractionEvent, m_MySliceDataZ);
	m_PlaneWidgetZ->AddObserver(vtkCommand::EndPickEvent, m_MySliceDataZ);
	m_PlaneWidgetZ->SetPlaceFactor(1.0);
	m_PlaneWidgetZ->PlaceWidget();
	m_PlaneWidgetZ->SetOrigin(center);
	m_PlaneWidgetZ->SetEnabled(m_CbShowslice2->isChecked());
	m_PlaneWidgetZ->PlaceWidget(bounds);

	//
	// Jump into the event loop and capture mouse and keyboard events.
	//
	//iren->Start();
	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::resizeEvent(QResizeEvent* RE)
{
	QWidget::resizeEvent(RE);
	QSize size1 = RE->size();

	m_Vbox1->setFixedSize(size1);

	if (size1.height() > 300)
		size1.setHeight(size1.height() - 300);
	m_VtkWidget->setFixedSize(size1);
	m_VtkWidget->GetRenderWindow()->SetSize(size1.width(), size1.height());
	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::closeEvent(QCloseEvent* qce)
{
	emit Hasbeenclosed();
	QWidget::closeEvent(qce);
}

VolumeViewerWidget::~VolumeViewerWidget() { delete m_Vbox1; }

void VolumeViewerWidget::ShadeChanged()
{
	if (m_CbShade->isChecked())
	{
		m_VolumeProperty->ShadeOn();
	}
	else
	{
		m_VolumeProperty->ShadeOff();
	}

	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::RaytraceortexturemapChanged()
{
	if (m_CbRaytraceortexturemap->isChecked())
	{
		m_VolumeMapper->SetRequestedRenderModeToDefault();
	}
	else
	{
		m_VolumeMapper->SetRequestedRenderModeToRayCast();
	}

	m_Volume->Update();
	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::ShowslicesChanged()
{
	bool changed = false;
	if ((m_CbShowslices->isChecked() ? 1 : 0) != m_SliceActorY->GetVisibility())
	{
		m_SliceActorY->SetVisibility(m_CbShowslices->isChecked());
		m_SliceActorZ->SetVisibility(m_CbShowslices->isChecked());
		changed = true;
	}
	if ((m_CbShowslice1->isChecked() ? 1 : 0) != m_PlaneWidgetY->GetEnabled())
	{
		m_PlaneWidgetY->SetEnabled(m_CbShowslice1->isChecked());
		changed = true;
	}
	if ((m_CbShowslice2->isChecked() ? 1 : 0) != m_PlaneWidgetZ->GetEnabled())
	{
		m_PlaneWidgetZ->SetEnabled(m_CbShowslice2->isChecked());
		changed = true;
	}

	if (changed)
	{
		m_VtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::ShowvolumeChanged()
{
	if (m_CbShowvolume->isChecked())
	{
		m_Volume->SetVisibility(true);
		m_CbShade->show();
		m_CbRaytraceortexturemap->show();
		if (m_Bmportissue)
		{
			m_Hbox3->hide();
			m_Hbox1->show();
			m_Hbox2->show();
		}
		else
		{
			m_Hbox3->show();
			m_Hbox1->hide();
			m_Hbox2->show();
		}
	}
	else
	{
		m_Volume->SetVisibility(false);
		m_CbShade->hide();
		m_CbRaytraceortexturemap->hide();
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
	}
	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::TissueChanged()
{
	if (!m_Bmportissue)
	{
		m_ColorTransferFunction->RemoveAllPoints();
		m_ColorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			m_ColorTransferFunction->AddRGBPoint(i, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
		}

		m_OpacityTransferFunction->RemoveAllPoints();
		m_OpacityTransferFunction->AddPoint(0, 0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (m_SlTrans->value() > 50)
				opac1 = opac1 * (100 - m_SlTrans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * m_SlTrans->value() / 50;
			m_OpacityTransferFunction->AddPoint(i, opac1);
		}

		m_VtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::TranspChanged()
{
	if (!m_Bmportissue)
	{
		m_OpacityTransferFunction->RemoveAllPoints();
		m_OpacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (m_SlTrans->value() > 50)
				opac1 = opac1 * (100 - m_SlTrans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * m_SlTrans->value() / 50;
			m_OpacityTransferFunction->AddPoint(i, opac1);
		}

		m_VtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::ContrbrightChanged()
{
	if (m_Bmportissue)
	{
		double level =
				m_Range[0] + (m_SlBright->value() * (m_Range[1] - m_Range[0]) / 100);
		double window = (m_Range[1] - m_Range[0]) / (m_SlContr->value() + 1);

		level = m_Range[0] + (m_SlBright->value() * (m_Range[1] - m_Range[0]) / 100);
		window = (m_Range[1] - m_Range[0]) * (m_SlContr->value() + 1) / 101;

		m_OpacityTransferFunction->RemoveAllPoints();
		m_OpacityTransferFunction->AddPoint(level - window / 2, 0.0);
		m_OpacityTransferFunction->AddPoint(level + window / 2, 1.0);
		m_ColorTransferFunction->RemoveAllPoints();
		m_ColorTransferFunction->AddRGBPoint(level - window / 2, 0.0, 0.0, 0.0);
		m_ColorTransferFunction->AddRGBPoint(level + window / 2, 1.0, 1.0, 1.0);

		m_VtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::PixelsizeChanged(Pair p)
{
	m_Input->SetSpacing(p.high, p.low, m_Hand3D->GetSlicethickness());
	//BL?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	m_Input->Modified();

	double bounds[6], center[3];

	m_Input->GetBounds(bounds);
	m_Input->GetCenter(center);

	m_PlaneWidgetY->SetOrigin(center);
	m_PlaneWidgetY->PlaceWidget(bounds);

	m_PlaneWidgetZ->SetOrigin(center);
	m_PlaneWidgetZ->PlaceWidget(bounds);

	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::ThicknessChanged(float thick)
{
	Pair p = m_Hand3D->GetPixelsize();
	m_Input->SetSpacing(p.high, p.low, thick);
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	m_Input->Modified();

	double bounds[6], center[3];
	m_Input->GetBounds(bounds);
	m_Input->GetCenter(center);

	m_PlaneWidgetY->SetOrigin(center);
	m_PlaneWidgetY->PlaceWidget(bounds);

	m_PlaneWidgetZ->SetOrigin(center);
	m_PlaneWidgetZ->PlaceWidget(bounds);

	//	outlineGrid->SetInput(input);
	//	outlineGrid->Update();
	//	outlineMapper->SetInput(outlineGrid->GetOutput());
	//	outlineMapper->Update();

	/*outlineMapper->Modified();
	outlineActor->Modified();
	ren3D->Modified();

	volume->Update();*/

	m_VtkWidget->GetRenderWindow()->Render();

	/*iSAR_ShepInterpolate iSI;
	int nx=16;
	int ny=8;
	float valin[128];
	{
	int pos=0;
	for(int pos=0;pos<nx*ny;pos++) {
	valin[pos]=0;
	}
	valin[19]=1;
	}
	float valout[256];

	{
	FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");

	int pos=0;
	for(int i=0;i<ny;i++) {
	for(int j=0;j<nx;j++) {
	if(i%2==1)
	fprintf(fp3,"- %f ",valin[j*ny+i]);
	else
	fprintf(fp3,"%f - ",valin[j*ny+i]);
	pos++;
	}
	fprintf(fp3,"\n");
	}
	fprintf(fp3,"\n\n");

	fclose(fp3);
	}

	iSI.interpolate(16,8,15,10,7.5,valin,valout);*/
}

void VolumeViewerWidget::Reload()
{
	int xm, xp, ym, yp, zm, zp;									//xxxa
	m_Input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	int size1[3];
	m_Input->GetDimensions(size1);
	//int w = hand3D->width(); //xxxa
	if ((m_Hand3D->Width() != size1[0]) ||
			(m_Hand3D->Height() != size1[1]) ||
			(m_Hand3D->NumSlices() != size1[2]))
	{
		m_Input->SetExtent(0, (int)m_Hand3D->Width() - 1, 0, (int)m_Hand3D->Height() - 1, 0, (int)m_Hand3D->NumSlices() - 1);
		m_Input->AllocateScalars(m_Input->GetScalarType(), m_Input->GetNumberOfScalarComponents());
		m_OutlineGrid->SetInputData(m_Input);
		m_PlaneWidgetY->SetInputData(m_Input);
		m_SliceCutterY->SetInputData(m_Input);
		m_PlaneWidgetZ->SetInputData(m_Input);
		m_SliceCutterZ->SetInputData(m_Input);

		double bounds[6], center[3];
		m_Input->GetBounds(bounds);
		m_Input->GetCenter(center);
		m_PlaneWidgetY->SetOrigin(center);
		m_PlaneWidgetY->PlaceWidget(bounds);
		m_PlaneWidgetZ->SetOrigin(center);
		m_PlaneWidgetZ->PlaceWidget(bounds);
		if (!m_Bmportissue)
		{
			m_VolumeMapper->SetInputData(m_Input);
		}
		else
		{
			m_Cast->SetInputData(m_Input);
			m_VolumeMapper->SetInputConnection(m_Cast->GetOutputPort());
		}
	}
	m_Input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	m_Input->GetDimensions(size1);							//xxxa
	Pair ps = m_Hand3D->GetPixelsize();
	m_Input->SetSpacing(ps.high, ps.low, m_Hand3D->GetSlicethickness());

	if (m_Bmportissue)
	{
		float* field = (float*)m_Input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < m_Hand3D->NumSlices(); i++)
		{
			m_Hand3D->Copyfrombmp(i, &(field[i * (unsigned long long)m_Hand3D->ReturnArea()]));
		}
	}
	else
	{
		tissues_size_t* field =
				(tissues_size_t*)m_Input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < m_Hand3D->NumSlices(); i++)
		{
			m_Hand3D->Copyfromtissue(i, &(field[i * (unsigned long long)m_Hand3D->ReturnArea()]));
		}
	}

	/*vtkInformation* info = input->GetPipelineInformation();
	Pair p;
	hand3D->get_bmprange(&p);
	range[0]=p.low;
	range[1]=p.high;
	info->Set(vtkDataObject::FIELD_RANGE(),range,2);*/
	m_Input->Modified();
	m_Input->GetScalarRange(m_Range);
	if (m_Bmportissue)
	{
		Pair p;

		m_Hand3D->GetBmprange(&p);
		m_Range[0] = p.low;
		m_Range[1] = p.high;
		m_Lut->SetTableRange(m_Range);
		m_Lut->Build(); //effective build
		double slope = 255 / (m_Range[1] - m_Range[0]);
		double shift = -m_Range[0];
		m_Cast->SetShift(shift);
		m_Cast->SetScale(slope);
		m_Cast->SetOutputScalarTypeToUnsignedShort();
		m_Cast->Update();
	}
	else
	{
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		m_Lut->SetNumberOfColors(tissuecount + 1);
		m_Lut->Build();
		m_Lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			m_Lut->SetTableValue((double)i - 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2], 1.0);
		}
		auto tissuecolor = TissueInfos::GetTissueColor(tissuecount + 1);
		m_ColorTransferFunction->AddRGBPoint((double)tissuecount + 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
	}
	m_SliceY->SetScalarRange(m_Range);
	m_SliceY->SetLookupTable(m_Lut);
	m_SliceY->Update();
	m_SliceZ->SetScalarRange(m_Range);
	m_SliceZ->SetLookupTable(m_Lut);
	m_SliceZ->Update();
	//		sliceY->Update();
	//		sliceZ->Update();
	//	} else {
	//		tissues_size_t *field=(tissues_size_t *)input->GetScalarPointer(0,0,0);
	//		for(unsigned short i=0;i<hand3D->return_nrslices();i++) {
	//			hand3D->copyfromtissue(i,&(field[i*(unsigned long)hand3D->return_area()]));
	//		}
	if (!m_Bmportissue)
	{
		m_OpacityTransferFunction->RemoveAllPoints();
		m_OpacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (m_SlTrans->value() > 50)
				opac1 = opac1 * (100 - m_SlTrans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * m_SlTrans->value() / 50;
			m_OpacityTransferFunction->AddPoint(i, opac1);
		}

		m_ColorTransferFunction->RemoveAllPoints();
		m_ColorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			m_ColorTransferFunction->AddRGBPoint(i, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
		}
	}

	m_Input->Modified();
	m_OutlineActor->Modified();

	m_VtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::SliceCallbackY::Execute(vtkObject* caller, unsigned long, void*)
{
	std::cerr << "ExecuteY\n";
	//
	// The plane has moved, update the sampled data values.
	//
	vtkImplicitPlaneWidget* plane_widget =
			reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
	//planeWidget->GetPolyData( plane );
	plane_widget->GetPlane(m_SlicePlaneY1);
	m_SliceCutterY1->SetCutFunction(m_SlicePlaneY1);
}

void iseg::VolumeViewerWidget::SliceCallbackZ::Execute(vtkObject* caller, unsigned long, void*)
{
	std::cerr << "ExecuteZ\n";
	//
	// The plane has moved, update the sampled data values.
	//
	vtkImplicitPlaneWidget* plane_widget =
			reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
	//planeWidget->GetPolyData( plane );
	plane_widget->GetPlane(m_SlicePlaneZ1);
	m_SliceCutterZ1->SetCutFunction(m_SlicePlaneZ1);
}

} // namespace iseg
