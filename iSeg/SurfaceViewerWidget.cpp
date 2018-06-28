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
#include "SurfaceViewerWidget.h"
#include "TissueInfos.h"

#include "QVTKWidget.h"

#include <QAction>
#include <QMenu>
#include <QResizeEvent>

#include <vtkFlyingEdges3D.h>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkGeometryFilter.h>
#include <vtkImageAccumulate.h>
#include <vtkMaskFields.h>
#include <vtkPointData.h>
#include <vtkThreshold.h>
#include <vtkWindowedSincPolyDataFilter.h>

#include <vtkActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <vtkAutoInit.h>
#ifdef ISEG_VTK_OPENGL2
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
#else
VTK_MODULE_INIT(vtkRenderingOpenGL); // VTK was built with vtkRenderingOpenGL
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL);
#endif
VTK_MODULE_INIT(vtkInteractionStyle);

using namespace iseg;

class MyInteractor : public vtkInteractorStyleTrackballCamera
{
public:
	static MyInteractor *New();
	vtkTypeMacro(MyInteractor, vtkInteractorStyleTrackballCamera);
	void PrintSelf(ostream& os, vtkIndent indent) override 
	{
		vtkInteractorStyleTrackballCamera::PrintSelf(os, indent);
	}
	
	void OnRightButtonDown() override
	{
		this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
							this->Interactor->GetEventPosition()[1]);
		if (this->CurrentRenderer == nullptr)
		{
			return;
		}
		// pick prop

		// if no prop
		this->GrabFocus(this->EventCallbackCommand);
		this->StartDolly();
	}
};

vtkStandardNewMacro(MyInteractor);


SurfaceViewerWidget::SurfaceViewerWidget(SlicesHandler* hand3D1, bool bmportissue1, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	bmportissue = bmportissue1;
	hand3D = hand3D1;
	vbox1 = new Q3VBox(this);
	vtkWidget = new QVTKWidget(vbox1);
	vtkWidget->setFixedSize(600, 600);

	hbox1 = new Q3HBox(vbox1);
	lb_trans = new QLabel("Transp.:", hbox1);
	sl_trans = new QSlider(Qt::Horizontal, hbox1);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(50);
	hbox1->setFixedHeight(hbox1->sizeHint().height());

	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this,
			SLOT(transp_changed()));

	if (bmportissue)
	{
		hbox2 = new Q3HBox(vbox1);
		lb_thresh = new QLabel("Thresh.:", hbox2);
		sl_thresh = new QSlider(Qt::Horizontal, hbox2);
		sl_thresh->setRange(0, 100);
		sl_thresh->setValue(50);
		hbox2->setFixedHeight(hbox2->sizeHint().height());

		QObject::connect(sl_thresh, SIGNAL(sliderReleased()), this,
				SLOT(thresh_changed()));
	}

	bt_update = new QPushButton("Update", vbox1);
	bt_update->show();

	vbox1->show();

	resize(vbox1->sizeHint().expandedTo(minimumSizeHint()));

	QObject::connect(bt_update, SIGNAL(clicked()), this, SLOT(reload()));

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
			hand3D->copyfrombmp(i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		switch (sizeof(tissues_size_t))
		{
		case 1: input->AllocateScalars(VTK_UNSIGNED_CHAR, 1); break;
		case 2: input->AllocateScalars(VTK_UNSIGNED_SHORT, 1); break;
		default:
			cerr << "surfaceviewer3D::surfaceviewer3D: tissues_size_t not implemented.\n";
		}
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		tissues_size_t* field =(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->num_slices(); i++)
		{
			hand3D->copyfromtissue(i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	double bounds[6], center[3];
	input->GetBounds(bounds);
	input->GetCenter(center);
	input->GetScalarRange(range);

	double level = 0.5 * (range[1] + range[0]);
	double window = range[1] - range[0];

	smoother = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	scalarsOff = vtkSmartPointer<vtkMaskFields>::New();

	// Define all of the variables
	startLabel = range[0];
	endLabel = range[1];
	startLabel = 1;
	unsigned int smoothingIterations = 15;
	double passBand = 0.001;
	double featureAngle = 120.0;

	// Generate models from labels
	// 1) Read the meta file
	// 2) Generate a histogram of the labels
	// 3) Generate models from the labeled volume
	// 4) Smooth the models
	// 5) Output each model into a separate file

	ren3D = vtkSmartPointer<vtkRenderer>::New();

	if (bmportissue)
	{
		cubes = vtkSmartPointer<vtkFlyingEdges3D>::New();
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) * sl_thresh->value());
		cubes->SetInputData(input);

		PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
		(*PolyDataMapper.rbegin())->SetInputData(cubes->GetOutput());
		(*PolyDataMapper.rbegin())->ScalarVisibilityOff();

		Actor.push_back(vtkSmartPointer<vtkActor>::New());
		(*Actor.rbegin())->SetMapper((*PolyDataMapper.rbegin()));
		(*Actor.rbegin())->GetProperty()->SetOpacity(1.0 - 0.01 * sl_trans->value());

		ren3D->AddActor((*Actor.rbegin()));
	}
	else
	{
		discreteCubes = vtkSmartPointer<vtkDiscreteFlyingEdges3D>::New();
		discreteCubes->SetInputData(input);

		histogram = vtkSmartPointer<vtkImageAccumulate>::New();
		histogram->SetInputData(input);
		histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
		histogram->SetComponentOrigin(0, 0, 0);
		histogram->SetComponentSpacing(1, 1, 1);
		histogram->Update();

		discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel,
				endLabel);
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::surfaceviewer3D: Out of range "
							"tissues_size_t."
					 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			geometry.push_back(vtkSmartPointer<vtkGeometryFilter>::New());

			selector.push_back(vtkSmartPointer<vtkThreshold>::New());

			(*selector.rbegin())->SetInputConnection(discreteCubes->GetOutputPort());

			(*geometry.rbegin())->SetInputConnection((*selector.rbegin())->GetOutputPort());

			// see if the label exists, if not skip it
			double frequency = histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(i);
			if (frequency == 0.0)
			{
				continue;
			}

			indices.push_back(i);

			// select the cells for a given label
			(*selector.rbegin())->ThresholdBetween(i, i);

			PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
			(*PolyDataMapper.rbegin())->SetInputConnection((*geometry.rbegin())->GetOutputPort());
			(*PolyDataMapper.rbegin())->ScalarVisibilityOff();

			Actor.push_back(vtkSmartPointer<vtkActor>::New());
			(*Actor.rbegin())->SetMapper((*PolyDataMapper.rbegin()));
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			(*Actor.rbegin())->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			(*Actor.rbegin())->GetProperty()->SetColor(tissuecolor[0], tissuecolor[1], tissuecolor[2]);
			ren3D->AddActor((*Actor.rbegin()));
		}
	}

	//smoother->SetInput(discreteCubes->GetOutput());
	//smoother->SetNumberOfIterations(smoothingIterations);
	//smoother->BoundarySmoothingOff();
	//smoother->FeatureEdgeSmoothingOff();
	//smoother->SetFeatureAngle(featureAngle);
	//smoother->SetPassBand(passBand);
	//smoother->NonManifoldSmoothingOn();
	//smoother->NormalizeCoordinatesOn();
	//smoother->Update();

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

	//
	// Jump into the event loop and capture mouse and keyboard events.
	//
	//iren->Start();
	vtkWidget->GetRenderWindow()->Render();

	vtkWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(vtkWidget, SIGNAL(customContextMenuRequested(const QPoint &)), 
        this, SLOT(show_context_menu(const QPoint &)));
}

SurfaceViewerWidget::~SurfaceViewerWidget() { delete vbox1; }

void SurfaceViewerWidget::show_context_menu(const QPoint &pos)
{
	// maybe override vtk interactor to get signal (and picked object/coordinates?)

	QMenu contextMenu(tr("Context menu"), this);
	contextMenu.setStyleSheet("QWidget { color: white; }");

	QAction select("Select Tissue", this);
	connect(&select, SIGNAL(triggered()), this, SLOT(select_tissue()));
	contextMenu.addAction(&select);

	QAction get_slice("Go to Slice", this);
	connect(&get_slice, SIGNAL(triggered()), this, SLOT(select_slice()));
	contextMenu.addAction(&get_slice);
	
	contextMenu.exec(mapToGlobal(pos));
}

void SurfaceViewerWidget::select_tissue()
{
	;
}

void SurfaceViewerWidget::select_slice()
{
	;
}

void SurfaceViewerWidget::tissue_changed()
{
	if (!bmportissue)
	{
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::tissue_changed: Out of range "
							"tissues_size_t."
					 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			size_t j = 0;
			while (j < indices.size() && indices[j] != i)
				j++;
			if (j == indices.size())
				continue;
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			Actor[j]->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			Actor[j]->GetProperty()->SetColor(tissuecolor[0], tissuecolor[1],
					tissuecolor[2]);
		}

		vtkWidget->GetRenderWindow()->Render();
	}
}

void SurfaceViewerWidget::pixelsize_changed(Pair p)
{
	input->SetSpacing(p.high, p.low, hand3D->get_slicethickness());
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::thickness_changed(float thick)
{
	Pair p = hand3D->get_pixelsize();
	input->SetSpacing(p.high, p.low, thick);
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::reload()
{
	if (!bmportissue)
	{
		for (size_t i = 0; i < Actor.size(); i++)
			ren3D->RemoveActor(Actor[i]);
		geometry.clear();
		selector.clear();
		indices.clear();
		PolyDataMapper.clear();
		Actor.clear();
	}

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
		if (bmportissue)
		{
			cubes->SetInputData(input);
		}
		else
		{
			discreteCubes->SetInputData(input);
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

	if (bmportissue)
	{
		Pair p;
		hand3D->get_bmprange(&p);
		range[0] = p.low;
		range[1] = p.high;
	}
	else
	{
		range[0] = 0;
		range[1] = TissueInfos::GetTissueCount();
	}

	startLabel = range[0];
	endLabel = range[1];
	startLabel = 1;

	if (bmportissue)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) *
																			sl_thresh->value());
	}
	else
	{
		histogram->SetInputData(input);
		histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
		histogram->Update();

		discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel,
				endLabel);

#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::reload: Out of range tissues_size_t."
					 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			geometry.push_back(vtkSmartPointer<vtkGeometryFilter>::New());
			selector.push_back(vtkSmartPointer<vtkThreshold>::New());

			//selector->SetInput(smoother->GetOutput());
			(*selector.rbegin())
					->SetInputConnection(discreteCubes->GetOutputPort());
			//selector->SetInputArrayToProcess(0, 0, 0,
			//	vtkDataObject::FIELD_ASSOCIATION_CELLS,
			//	vtkDataSetAttributes::SCALARS);

			//// Strip the scalars from the output
			//scalarsOff->SetInput(selector->GetOutput());
			//scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA,
			//	vtkDataSetAttributes::SCALARS);
			//scalarsOff->CopyAttributeOff(vtkMaskFields::CELL_DATA,
			//	vtkDataSetAttributes::SCALARS);

			//geometry->SetInput(scalarsOff->GetOutput());
			(*geometry.rbegin())
					->SetInputConnection((*selector.rbegin())->GetOutputPort());

			// see if the label exists, if not skip it
			double frequency =
					histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(
							i);
			if (frequency == 0.0)
			{
				continue;
			}

			indices.push_back(i);

			// select the cells for a given label
			(*selector.rbegin())->ThresholdBetween(i, i);

			PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
			(*PolyDataMapper.rbegin())
					->SetInputConnection((*geometry.rbegin())->GetOutputPort());
			(*PolyDataMapper.rbegin())->ScalarVisibilityOff();

			Actor.push_back(vtkSmartPointer<vtkActor>::New());
			(*Actor.rbegin())->SetMapper((*PolyDataMapper.rbegin()));
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			(*Actor.rbegin())->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			(*Actor.rbegin())
					->GetProperty()
					->SetColor(tissuecolor[0], tissuecolor[1], tissuecolor[2]);
			ren3D->AddActor((*Actor.rbegin()));
		}
	}

	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::closeEvent(QCloseEvent* qce)
{
	emit hasbeenclosed();
	QWidget::closeEvent(qce);
}

void SurfaceViewerWidget::resizeEvent(QResizeEvent* RE)
{
	QWidget::resizeEvent(RE);
	QSize size1 = RE->size();
	vbox1->setFixedSize(size1);
	if (size1.height() > 150)
		size1.setHeight(size1.height() - 150);
	vtkWidget->setFixedSize(size1);
	vtkWidget->GetRenderWindow()->SetSize(size1.width(), size1.height());
	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::transp_changed()
{
	if (bmportissue)
	{
		(*Actor.rbegin())
				->GetProperty()
				->SetOpacity(1.0 - 0.01 * sl_trans->value());
	}
	else
	{
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::transp_changed: Out of range "
							"tissues_size_t."
					 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			size_t j = 0;
			while (j < indices.size() && indices[j] != i)
				j++;
			if (j == indices.size())
				continue;
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			Actor[j]->GetProperty()->SetOpacity(opac1);
		}
	}
	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::thresh_changed()
{
	if (bmportissue)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) *
																			sl_thresh->value());

		vtkWidget->GetRenderWindow()->Render();
	}
}
