/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

using namespace iseg;

VolumeViewerWidget::VolumeViewerWidget(SlicesHandler* hand3D1, bool bmportissue1,
		bool gpu_or_raycast, bool shade1,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	bmportissue = bmportissue1;
	hand3D = hand3D1;
	vbox1 = new Q3VBox(this);
	vtkWidget = new QVTKWidget(vbox1);
	vtkWidget->setFixedSize(600, 600);
	//	resize( QSize(600, 600).expandedTo(minimumSizeHint()) );

	cb_shade = new QCheckBox("Shade", vbox1);
	cb_shade->setChecked(shade1);
	cb_shade->show();
	cb_raytraceortexturemap = new QCheckBox("GPU / Software", vbox1);
	cb_raytraceortexturemap->setChecked(gpu_or_raycast);
	cb_raytraceortexturemap->show();

	cb_showslices = new QCheckBox("Show Slices", vbox1);
	cb_showslices->setChecked(true);
	cb_showslices->show();
	cb_showslice1 = new QCheckBox("Enable Slicer 1", vbox1);
	cb_showslice1->setChecked(false);
	cb_showslice1->show();
	cb_showslice2 = new QCheckBox("Enable Slicer 2", vbox1);
	cb_showslice2->setChecked(false);
	cb_showslice2->show();
	cb_showvolume = new QCheckBox("Show Volume", vbox1);
	cb_showvolume->setChecked(true);
	cb_showvolume->show();

	hbox1 = new Q3HBox(vbox1);
	lb_contr = new QLabel("Window:", hbox1);
	sl_contr = new QSlider(Qt::Horizontal, hbox1);
	sl_contr->setRange(0, 100);
	sl_contr->setValue(100);
	hbox1->setFixedHeight(hbox1->sizeHint().height());
	hbox2 = new Q3HBox(vbox1);
	lb_bright = new QLabel("Level:", hbox2);
	sl_bright = new QSlider(Qt::Horizontal, hbox2);
	sl_bright->setRange(0, 100);
	sl_bright->setValue(50);
	hbox2->setFixedHeight(hbox2->sizeHint().height());
	hbox3 = new Q3HBox(vbox1);
	lb_trans = new QLabel("Transp.:", hbox3);
	sl_trans = new QSlider(Qt::Horizontal, hbox3);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(50);
	hbox3->setFixedHeight(hbox3->sizeHint().height());

	bt_update = new QPushButton("Update", vbox1);
	bt_update->show();

	if (bmportissue)
	{
		hbox3->hide();
		hbox1->show();
		hbox2->show();
	}
	else
	{
		hbox3->show();
		hbox1->hide();
		hbox2->hide();
	}

	vbox1->show();

	//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	//	vbox1->setFixedSize(vbox1->sizeHint());
	resize(vbox1->sizeHint().expandedTo(minimumSizeHint()));

	QObject::connect(cb_shade, SIGNAL(clicked()), this, SLOT(shade_changed()));
	QObject::connect(cb_raytraceortexturemap, SIGNAL(clicked()), this,
			SLOT(raytraceortexturemap_changed()));
	QObject::connect(bt_update, SIGNAL(clicked()), this, SLOT(reload()));
	QObject::connect(cb_showslices, SIGNAL(clicked()), this,
			SLOT(showslices_changed()));
	QObject::connect(cb_showslice1, SIGNAL(clicked()), this,
			SLOT(showslices_changed()));
	QObject::connect(cb_showslice2, SIGNAL(clicked()), this,
			SLOT(showslices_changed()));
	QObject::connect(cb_showvolume, SIGNAL(clicked()), this,
			SLOT(showvolume_changed()));

	QObject::connect(sl_bright, SIGNAL(sliderReleased()), this,
			SLOT(contrbright_changed()));
	QObject::connect(sl_contr, SIGNAL(sliderReleased()), this,
			SLOT(contrbright_changed()));
	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this,
			SLOT(transp_changed()));

	//   vtkStructuredPointsReader* reader = vtkStructuredPointsReader::New();
	//  reader= vtkSmartPointer<vtkXMLImageDataReader>::New();
	//  fnamei="D:\\Development\\bmp_read_QT\\bmp_read_3D_8\\qvtkViewer\\test2.vti";
	//  reader->SetFileName(fnamei.c_str());

	//  vtkImageData* input = (vtkImageData*)reader->GetOutput();
	//  input->Update();

	input = vtkSmartPointer<vtkImageData>::New();
	input->SetExtent(0, (int)hand3D->width() - 1, 0,
			(int)hand3D->height() - 1, 0,
			(int)hand3D->num_slices() - 1);
	Pair ps = hand3D->get_pixelsize();
	if (bmportissue)
	{
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		input->AllocateScalars(VTK_FLOAT, 1);
		float* field = (float*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->num_slices(); i++)
		{
			hand3D->copyfrombmp(
					i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		switch (sizeof(tissues_size_t))
		{
		case 1: input->AllocateScalars(VTK_UNSIGNED_CHAR, 1); break;
		case 2: input->AllocateScalars(VTK_UNSIGNED_SHORT, 1); break;
		default:
			std::cerr << "volumeviewer3D::volumeviewer3D: tissues_size_t not "
							"implemented."
					 << endl;
		}
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		tissues_size_t* field =
				(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->num_slices(); i++)
		{
			hand3D->copyfromtissue(
					i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	double bounds[6], center[3];

	input->GetBounds(bounds);
	input->GetCenter(center);
	input->GetScalarRange(range);
	std::cerr << "input range = " << range[0] << " " << range[1] << endl;

	double level = 0.5 * (range[1] + range[0]);
	double window = range[1] - range[0];
	if (range[1] > 250)
	{
		level = 125;
		window = 250;
		sl_contr->setValue((101 * window / (range[1] - range[0])) - 1);
		sl_bright->setValue(100 * (level - range[0]) / (range[1] - range[0]));
	}

	//
	// Put a bounding box in the scene to help orient the user.
	//
	outlineGrid = vtkSmartPointer<vtkOutlineFilter>::New();
	outlineGrid->SetInputData(input);
	outlineGrid->Update();
	outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection(outlineGrid->GetOutputPort());
	outlineActor = vtkSmartPointer<vtkActor>::New();
	outlineActor->SetMapper(outlineMapper);

	// Add a PlaneWidget to interactively move the slice plane.
	planeWidgetY = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
	planeWidgetY->SetInputData(input);
	planeWidgetY->NormalToYAxisOn();
	planeWidgetY->DrawPlaneOff();
	planeWidgetY->SetScaleEnabled(0);
	planeWidgetY->SetOriginTranslation(0);
	planeWidgetY->SetOutsideBounds(0);

	// Attach a Cutter to the slice plane to sample the function.
	sliceCutterY = vtkSmartPointer<vtkCutter>::New();
	sliceCutterY->SetInputData(input);
	slicePlaneY = vtkSmartPointer<vtkPlane>::New();
	planeWidgetY->GetPlane(slicePlaneY);
	sliceCutterY->SetCutFunction(slicePlaneY);

	// Add a PlaneWidget to interactively move the slice plane.
	planeWidgetZ = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
	planeWidgetZ->SetInputData(input);
	planeWidgetZ->NormalToZAxisOn();
	planeWidgetZ->DrawPlaneOff();
	planeWidgetZ->SetScaleEnabled(0);
	planeWidgetZ->SetOriginTranslation(0);
	planeWidgetZ->SetOutsideBounds(0);

	// Attach a Cutter to the slice plane to sample the function.
	sliceCutterZ = vtkSmartPointer<vtkCutter>::New();
	sliceCutterZ->SetInputData(input);
	slicePlaneZ = vtkSmartPointer<vtkPlane>::New();
	planeWidgetZ->GetPlane(slicePlaneZ);
	sliceCutterZ->SetCutFunction(slicePlaneZ);

	//
	// Create a lookup table to map the velocity magnitudes.
	//
	if (bmportissue)
	{
		lut = vtkSmartPointer<vtkLookupTable>::New();
		lut->SetTableRange(range);
		lut->SetHueRange(0, 0);
		lut->SetSaturationRange(0, 0);
		lut->SetValueRange(0, 1);
		lut->Build(); //effective build
	}
	else
	{
		lut = vtkSmartPointer<vtkLookupTable>::New();
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		lut->SetNumberOfColors(tissuecount + 1);
		lut->Build();
		lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			lut->SetTableValue(i, tissuecolor[0], tissuecolor[1], tissuecolor[2], 1.0);
		}
	}
	//
	// Map the slice plane and create the geometry to be rendered.
	//
	sliceY = vtkSmartPointer<vtkPolyDataMapper>::New();
	if (!bmportissue)
		sliceY->SetColorModeToMapScalars();
	sliceY->SetInputConnection(sliceCutterY->GetOutputPort());
	//   input->GetScalarRange( range );
	//range[1] *= 0.7; // reduce the upper range by 30%
	sliceY->SetScalarRange(range);
	sliceY->SetLookupTable(lut);
	//   sliceY->SelectColorArray(argv[2]);
	sliceY->Update();
	sliceActorY = vtkSmartPointer<vtkActor>::New();
	sliceActorY->SetMapper(sliceY);

	sliceZ = vtkSmartPointer<vtkPolyDataMapper>::New();
	if (!bmportissue)
		sliceZ->SetColorModeToMapScalars();
	sliceZ->SetInputConnection(sliceCutterZ->GetOutputPort());
	//   input->GetScalarRange( range );
	//range[1] *= 0.7; // reduce the upper range by 30%
	sliceZ->SetScalarRange(range);
	sliceZ->SetLookupTable(lut);
	sliceZ->Update();
	sliceActorZ = vtkSmartPointer<vtkActor>::New();
	sliceActorZ->SetMapper(sliceZ);

	if (!bmportissue)
	{
		opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		opacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		colorTransferFunction =
				vtkSmartPointer<vtkColorTransferFunction>::New();
		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			colorTransferFunction->AddRGBPoint((double)i - 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
		}
		auto tissuecolor = TissueInfos::GetTissueColor(tissuecount);
		colorTransferFunction->AddRGBPoint((double)tissuecount + 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
	}
	else
	{
		opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		opacityTransferFunction->AddPoint(level - window / 2, 0.0);
		opacityTransferFunction->AddPoint(level + window / 2, 1.0);
		colorTransferFunction =
				vtkSmartPointer<vtkColorTransferFunction>::New();
		colorTransferFunction->AddRGBPoint(level - window / 2, 0.0, 0.0, 0.0);
		colorTransferFunction->AddRGBPoint(level + window / 2, 1.0, 1.0, 1.0);
	}

	volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	if (shade1)
		volumeProperty->ShadeOn();
	else
		volumeProperty->ShadeOff();
	if (bmportissue)
		volumeProperty->SetInterpolationTypeToLinear();
	else
		volumeProperty->SetInterpolationTypeToNearest();

	volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	volumeMapper->SetRequestedRenderModeToDefault();

	cast = vtkSmartPointer<vtkImageShiftScale>::New();
	if (bmportissue)
	{
		double slope = 255 / (range[1] - range[0]);
		double shift = -range[0];
		cast->SetInputData(input);
		cast->SetShift(shift);
		cast->SetScale(slope);
		cast->SetOutputScalarTypeToUnsignedShort();
		volumeMapper->SetInputConnection(cast->GetOutputPort());
	}
	else
	{
		volumeMapper->SetInputData(input);
	}

	volume = vtkSmartPointer<vtkVolume>::New();
	if (gpu_or_raycast)
		volumeMapper->SetRequestedRenderModeToDefault();
	else
		volumeMapper->SetRequestedRenderModeToRayCast();

	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

	//
	// Create the renderers and assign actors to them.
	//
	ren3D = vtkSmartPointer<vtkRenderer>::New();
	ren3D->AddActor(sliceActorY);
	ren3D->AddActor(sliceActorZ);
	ren3D->AddVolume(volume);

	ren3D->AddActor(outlineActor);
	ren3D->SetBackground(0, 0, 0);
	ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);

	renWin = vtkSmartPointer<vtkRenderWindow>::New();

	vtkWidget->GetRenderWindow()->AddRenderer(ren3D);
	vtkWidget->GetRenderWindow()->LineSmoothingOn();
	vtkWidget->GetRenderWindow()->SetSize(600, 600);

	style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

	iren = vtkSmartPointer<QVTKInteractor>::New();
	iren->SetInteractorStyle(style);

	iren->SetRenderWindow(vtkWidget->GetRenderWindow());

	my_sliceDataY = vtkSmartPointer<vtkMySliceCallbackY>::New();
	my_sliceDataY->slicePlaneY1 = slicePlaneY;
	my_sliceDataY->sliceCutterY1 = sliceCutterY;
	planeWidgetY->SetInteractor(iren);
	planeWidgetY->AddObserver(vtkCommand::InteractionEvent, my_sliceDataY);
	planeWidgetY->AddObserver(vtkCommand::EnableEvent, my_sliceDataY);
	planeWidgetY->AddObserver(vtkCommand::StartInteractionEvent, my_sliceDataY);
	planeWidgetY->AddObserver(vtkCommand::EndPickEvent, my_sliceDataY);
	planeWidgetY->SetPlaceFactor(1.0);
	planeWidgetY->PlaceWidget();
	planeWidgetY->SetOrigin(center);
	planeWidgetY->SetEnabled(cb_showslice1->isOn());
	planeWidgetY->PlaceWidget(bounds);

	my_sliceDataZ = vtkSmartPointer<vtkMySliceCallbackZ>::New();
	my_sliceDataZ->slicePlaneZ1 = slicePlaneZ;
	my_sliceDataZ->sliceCutterZ1 = sliceCutterZ;
	planeWidgetZ->SetInteractor(iren);
	planeWidgetZ->AddObserver(vtkCommand::InteractionEvent, my_sliceDataZ);
	planeWidgetZ->AddObserver(vtkCommand::EnableEvent, my_sliceDataZ);
	planeWidgetZ->AddObserver(vtkCommand::StartInteractionEvent, my_sliceDataZ);
	planeWidgetZ->AddObserver(vtkCommand::EndPickEvent, my_sliceDataZ);
	planeWidgetZ->SetPlaceFactor(1.0);
	planeWidgetZ->PlaceWidget();
	planeWidgetZ->SetOrigin(center);
	planeWidgetZ->SetEnabled(cb_showslice2->isOn());
	planeWidgetZ->PlaceWidget(bounds);

	//
	// Jump into the event loop and capture mouse and keyboard events.
	//
	//iren->Start();
	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::resizeEvent(QResizeEvent* RE)
{
	QWidget::resizeEvent(RE);
	QSize size1 = RE->size();

	vbox1->setFixedSize(size1);

	if (size1.height() > 300)
		size1.setHeight(size1.height() - 300);
	vtkWidget->setFixedSize(size1);
	vtkWidget->GetRenderWindow()->SetSize(size1.width(), size1.height());
	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::closeEvent(QCloseEvent* qce)
{
	emit hasbeenclosed();
	QWidget::closeEvent(qce);
}

VolumeViewerWidget::~VolumeViewerWidget() { delete vbox1; }

void VolumeViewerWidget::shade_changed()
{
	if (cb_shade->isOn())
	{
		volumeProperty->ShadeOn();
	}
	else
	{
		volumeProperty->ShadeOff();
	}

	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::raytraceortexturemap_changed()
{
	if (cb_raytraceortexturemap->isOn())
	{
		volumeMapper->SetRequestedRenderModeToDefault();
	}
	else
	{
		volumeMapper->SetRequestedRenderModeToRayCast();
	}

	volume->Update();
	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::showslices_changed()
{
	bool changed = false;
	if ((cb_showslices->isOn() ? 1 : 0) != sliceActorY->GetVisibility())
	{
		sliceActorY->SetVisibility(cb_showslices->isOn());
		sliceActorZ->SetVisibility(cb_showslices->isOn());
		changed = true;
	}
	if ((cb_showslice1->isOn() ? 1 : 0) != planeWidgetY->GetEnabled())
	{
		planeWidgetY->SetEnabled(cb_showslice1->isOn());
		changed = true;
	}
	if ((cb_showslice2->isOn() ? 1 : 0) != planeWidgetZ->GetEnabled())
	{
		planeWidgetZ->SetEnabled(cb_showslice2->isOn());
		changed = true;
	}

	if (changed)
	{
		vtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::showvolume_changed()
{
	if (cb_showvolume->isOn())
	{
		volume->SetVisibility(true);
		cb_shade->show();
		cb_raytraceortexturemap->show();
		if (bmportissue)
		{
			hbox3->hide();
			hbox1->show();
			hbox2->show();
		}
		else
		{
			hbox3->show();
			hbox1->hide();
			hbox2->show();
		}
	}
	else
	{
		volume->SetVisibility(false);
		cb_shade->hide();
		cb_raytraceortexturemap->hide();
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
	}
	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::tissue_changed()
{
	if (!bmportissue)
	{
		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			colorTransferFunction->AddRGBPoint(i, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
		}

		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(0, 0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		vtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::transp_changed()
{
	if (!bmportissue)
	{
		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		vtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::contrbright_changed()
{
	if (bmportissue)
	{
		double level =
				range[0] + (sl_bright->value() * (range[1] - range[0]) / 100);
		double window = (range[1] - range[0]) / (sl_contr->value() + 1);

		level = range[0] + (sl_bright->value() * (range[1] - range[0]) / 100);
		window = (range[1] - range[0]) * (sl_contr->value() + 1) / 101;

		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(level - window / 2, 0.0);
		opacityTransferFunction->AddPoint(level + window / 2, 1.0);
		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(level - window / 2, 0.0, 0.0, 0.0);
		colorTransferFunction->AddRGBPoint(level + window / 2, 1.0, 1.0, 1.0);

		vtkWidget->GetRenderWindow()->Render();
	}
}

void VolumeViewerWidget::pixelsize_changed(Pair p)
{
	input->SetSpacing(p.high, p.low, hand3D->get_slicethickness());
	//BL?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	double bounds[6], center[3];

	input->GetBounds(bounds);
	input->GetCenter(center);

	planeWidgetY->SetOrigin(center);
	planeWidgetY->PlaceWidget(bounds);

	planeWidgetZ->SetOrigin(center);
	planeWidgetZ->PlaceWidget(bounds);

	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::thickness_changed(float thick)
{
	Pair p = hand3D->get_pixelsize();
	input->SetSpacing(p.high, p.low, thick);
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	double bounds[6], center[3];
	input->GetBounds(bounds);
	input->GetCenter(center);

	planeWidgetY->SetOrigin(center);
	planeWidgetY->PlaceWidget(bounds);

	planeWidgetZ->SetOrigin(center);
	planeWidgetZ->PlaceWidget(bounds);

	//	outlineGrid->SetInput(input);
	//	outlineGrid->Update();
	//	outlineMapper->SetInput(outlineGrid->GetOutput());
	//	outlineMapper->Update();

	/*outlineMapper->Modified();
	outlineActor->Modified();
	ren3D->Modified();

	volume->Update();*/

	vtkWidget->GetRenderWindow()->Render();

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

void VolumeViewerWidget::reload()
{
	int xm, xp, ym, yp, zm, zp;								//xxxa
	input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	int size1[3];
	input->GetDimensions(size1);
	int w = hand3D->width(); //xxxa
	if ((hand3D->width() != size1[0]) ||
			(hand3D->height() != size1[1]) ||
			(hand3D->num_slices() != size1[2]))
	{
		input->SetExtent(0, (int)hand3D->width() - 1, 0,
				(int)hand3D->height() - 1, 0,
				(int)hand3D->num_slices() - 1);
		input->AllocateScalars(input->GetScalarType(),
				input->GetNumberOfScalarComponents());
		outlineGrid->SetInputData(input);
		planeWidgetY->SetInputData(input);
		sliceCutterY->SetInputData(input);
		planeWidgetZ->SetInputData(input);
		sliceCutterZ->SetInputData(input);

		double bounds[6], center[3];
		input->GetBounds(bounds);
		input->GetCenter(center);
		planeWidgetY->SetOrigin(center);
		planeWidgetY->PlaceWidget(bounds);
		planeWidgetZ->SetOrigin(center);
		planeWidgetZ->PlaceWidget(bounds);
		if (!bmportissue)
		{
			volumeMapper->SetInputData(input);
		}
		else
		{
			cast->SetInputData(input);
			volumeMapper->SetInputConnection(cast->GetOutputPort());
		}
	}
	input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	input->GetDimensions(size1);							//xxxa
	Pair ps = hand3D->get_pixelsize();
	input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());

	if (bmportissue)
	{
		float* field = (float*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->num_slices(); i++)
		{
			hand3D->copyfrombmp(
					i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		tissues_size_t* field =
				(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->num_slices(); i++)
		{
			hand3D->copyfromtissue(
					i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	/*vtkInformation* info = input->GetPipelineInformation();
	Pair p;
	hand3D->get_bmprange(&p);
	range[0]=p.low;
	range[1]=p.high;
	info->Set(vtkDataObject::FIELD_RANGE(),range,2);*/
	input->Modified();
	input->GetScalarRange(range);
	if (bmportissue)
	{
		Pair p;

		hand3D->get_bmprange(&p);
		range[0] = p.low;
		range[1] = p.high;
		lut->SetTableRange(range);
		lut->Build(); //effective build
		double slope = 255 / (range[1] - range[0]);
		double shift = -range[0];
		cast->SetShift(shift);
		cast->SetScale(slope);
		cast->SetOutputScalarTypeToUnsignedShort();
		cast->Update();
	}
	else
	{
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		lut->SetNumberOfColors(tissuecount + 1);
		lut->Build();
		lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			lut->SetTableValue((double)i - 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2], 1.0);
		}
		auto tissuecolor = TissueInfos::GetTissueColor(tissuecount + 1);
		colorTransferFunction->AddRGBPoint((double)tissuecount + 0.1, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
	}
	sliceY->SetScalarRange(range);
	sliceY->SetLookupTable(lut);
	sliceY->Update();
	sliceZ->SetScalarRange(range);
	sliceZ->SetLookupTable(lut);
	sliceZ->Update();
	//		sliceY->Update();
	//		sliceZ->Update();
	//	} else {
	//		tissues_size_t *field=(tissues_size_t *)input->GetScalarPointer(0,0,0);
	//		for(unsigned short i=0;i<hand3D->return_nrslices();i++) {
	//			hand3D->copyfromtissue(i,&(field[i*(unsigned long)hand3D->return_area()]));
	//		}
	if (!bmportissue)
	{
		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			auto tissuecolor = TissueInfos::GetTissueColor(i);
			colorTransferFunction->AddRGBPoint(i, tissuecolor[0], tissuecolor[1], tissuecolor[2]);
		}
	}

	input->Modified();
	outlineActor->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void VolumeViewerWidget::vtkMySliceCallbackY::Execute(vtkObject* caller, unsigned long, void*)
{
	std::cerr << "ExecuteY\n";
	//
	// The plane has moved, update the sampled data values.
	//
	vtkImplicitPlaneWidget* planeWidget =
			reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
	//planeWidget->GetPolyData( plane );
	planeWidget->GetPlane(slicePlaneY1);
	sliceCutterY1->SetCutFunction(slicePlaneY1);
}

void iseg::VolumeViewerWidget::vtkMySliceCallbackZ::Execute(vtkObject* caller, unsigned long, void*)
{
	std::cerr << "ExecuteZ\n";
	//
	// The plane has moved, update the sampled data values.
	//
	vtkImplicitPlaneWidget* planeWidget =
			reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
	//planeWidget->GetPolyData( plane );
	planeWidget->GetPlane(slicePlaneZ1);
	sliceCutterZ1->SetCutFunction(slicePlaneZ1);
}
