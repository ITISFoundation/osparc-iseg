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

#include <vtkCellData.h>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkFlyingEdges3D.h>
#include <vtkGeometryFilter.h>
#include <vtkImageAccumulate.h>
#include <vtkMaskFields.h>
#include <vtkPointData.h>
#include <vtkThreshold.h>
#include <vtkUnsignedShortArray.h>
#include <vtkBitArray.h>
#include <vtkWindowedSincPolyDataFilter.h>

#include <vtkActor.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
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

namespace
{
	template<typename TIn, typename TOut, typename TMap>
	void transform_slices(const std::vector<TIn*>& slices, size_t slice_size, TOut* out, const TMap& map)
	{
		for (auto slice: slices)
		{
			std::transform(slice, slice + slice_size, out, map);
			std::advance(out, slice_size);
		}
	}
	template<typename TIn, typename TMap>
	void transform_slices(const std::vector<TIn*>& slices, size_t slice_size, vtkBitArray* out, const TMap& map)
	{
		size_t idx = 0, i;
		for (auto& slice: slices)
		{
			for (i = 0; i < slice_size; ++i)
			{
				out->SetValue(idx++, map(slice[i]));
			}
		}
	}
}

SurfaceViewerWidget::SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType inputtype, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	input_type = inputtype;
	hand3D = hand3D1;
	vbox1 = new Q3VBox(this);
	vtkWidget = new QVTKWidget(vbox1);
	vtkWidget->setFixedSize(1000, 1000);

	hbox1 = new Q3HBox(vbox1);
	lb_trans = new QLabel("Transparency", hbox1);
	sl_trans = new QSlider(Qt::Horizontal, hbox1);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(00);
	hbox1->setFixedHeight(hbox1->sizeHint().height());

	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this,
			SLOT(transp_changed()));

	if (input_type == kSource)
	{
		hbox2 = new Q3HBox(vbox1);
		lb_thresh = new QLabel("Threshold", hbox2);
		sl_thresh = new QSlider(Qt::Horizontal, hbox2);
		sl_thresh->setRange(0, 100);
		sl_thresh->setValue(50);
		hbox2->setFixedHeight(hbox2->sizeHint().height());

		QObject::connect(sl_thresh, SIGNAL(sliderReleased()), this, SLOT(thresh_changed()));
	}

	bt_update = new QPushButton("Update", vbox1);
	bt_update->show();

	vbox1->show();

	resize(vbox1->sizeHint().expandedTo(minimumSizeHint()));

	QObject::connect(bt_update, SIGNAL(clicked()), this, SLOT(reload()));

	auto tissue_selection = hand3D->tissue_selection();
	auto spacing = hand3D->spacing();
	size_t slice_size = static_cast<size_t>(hand3D->width()) * hand3D->height();
	
	input = vtkSmartPointer<vtkImageData>::New();
	input->SetExtent(0, (int)hand3D->width() - 1, 0,
			(int)hand3D->height() - 1, 0,
			(int)hand3D->num_slices() - 1);
	input->SetSpacing(spacing[0], spacing[1], spacing[2]);

	if (input_type == kSource) // iso-surface
	{
		auto slices = hand3D->source_slices();
		input->AllocateScalars(VTK_FLOAT, 1);
		auto field = (float*)input->GetScalarPointer();
		transform_slices(slices, slice_size, field, [](float v){ return v; });
	}
	else if (input_type == kTarget) // foreground
	{
		auto slices = hand3D->target_slices();
		input->AllocateScalars(VTK_BIT, 1);
		auto field = vtkBitArray::SafeDownCast(input->GetPointData()->GetScalars());
		transform_slices(slices, slice_size, field, [](float v){ return v>0.f?1:0; });
	}
	else if (input_type == kTissues || tissue_selection.size() > 254) // all tissues
	{
		auto slices = hand3D->tissue_slices(0);
		input->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		auto field = static_cast<tissues_size_t*>(input->GetScalarPointer());
		transform_slices(slices, slice_size, field, [](tissues_size_t v){ return v; });
	}
	else if (tissue_selection.size() > 1) // [2, 254]
	{
		auto slices = hand3D->tissue_slices(0);

		unsigned char count = 1;
		std::vector<unsigned char> tissue_index_map(TissueInfos::GetTissueCount() + 1, 0);
		for (auto tissue_type: tissue_selection)
		{
			index_tissue_map[count] = tissue_type;
			tissue_index_map[tissue_type] = count++;
		}

		input->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		auto field = static_cast<unsigned char*>(input->GetScalarPointer());
		try
		{
			transform_slices(slices, slice_size, field, [tissue_index_map](tissues_size_t v){ return tissue_index_map.at(v); });
		}
		catch(std::exception& e)
		{
			std::cerr << "ERROR: bad tissue index map " << e.what() << "\n";
			std::fill_n(field, slices.size() * slice_size, 0);
		}
	}
	else if (tissue_selection.size() == 1)
	{
		auto tissue_type = tissue_selection[0];
		index_tissue_map[0] = tissue_type;

		auto slices = hand3D->tissue_slices(0);
		input->AllocateScalars(VTK_BIT, 1);
		auto field = vtkBitArray::SafeDownCast(input->GetPointData()->GetScalars());

		transform_slices(slices, slice_size, field, [tissue_type](tissues_size_t v){ return v==tissue_type; });
	}
	else
	{
		input->AllocateScalars(VTK_BIT, 1);
		auto field = vtkBitArray::SafeDownCast(input->GetPointData()->GetScalars());
		field->FillComponent(0, 0);
	}

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

	if (input_type == kSource)
	{
		cubes = vtkSmartPointer<vtkFlyingEdges3D>::New();
		cubes->SetValue(0, range[0] + 0.01 * (range[1] - range[0]) * sl_thresh->value());
		cubes->SetInputData(input);

		PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
		PolyDataMapper.back()->SetInputData(cubes->GetOutput());
		PolyDataMapper.back()->ScalarVisibilityOff();

		Actor.push_back(vtkSmartPointer<vtkActor>::New());
		Actor.back()->SetMapper(PolyDataMapper.back());
		Actor.back()->GetProperty()->SetOpacity(1.0 - 0.01 * sl_trans->value());

		ren3D->AddActor(Actor.back());
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

		discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel, endLabel);
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::surfaceviewer3D: Out of range tissues_size_t.\n";
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			selector.push_back(vtkSmartPointer<vtkThreshold>::New());
			selector.back()->SetInputConnection(discreteCubes->GetOutputPort());

			geometry.push_back(vtkSmartPointer<vtkGeometryFilter>::New());
			geometry.back()->SetInputConnection(selector.back()->GetOutputPort());

			// see if the label exists, if not skip it
			double frequency = histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(i);
			if (frequency == 0.0)
			{
				continue;
			}

			indices.push_back(i);

			// select the cells for a given label
			selector.back()->ThresholdBetween(i, i);

			PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
			PolyDataMapper.back()->SetInputConnection(geometry.back()->GetOutputPort());
			PolyDataMapper.back()->ScalarVisibilityOff();

			Actor.push_back(vtkSmartPointer<vtkActor>::New());
			Actor.back()->SetMapper(PolyDataMapper.back());
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			Actor.back()->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			Actor.back()->GetProperty()->SetColor(tissuecolor[0], tissuecolor[1], tissuecolor[2]);
			ren3D->AddActor(Actor.back());
		}
	}

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

	QMenu* popup_actions = new QMenu(vtkWidget);
	popup_actions->addAction("Select Tissue");
	popup_actions->addAction("Select Slice");
	connect(popup_actions, SIGNAL(triggered(QAction*)), this, SLOT(select_action(QAction*)));

	connections = vtkSmartPointer<vtkEventQtSlotConnect>::New();

	connections->Connect(vtkWidget->GetRenderWindow()->GetInteractor(), vtkCommand::RightButtonPressEvent,
			this, SLOT(popup(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
			popup_actions, 1.0);

	vtkWidget->GetRenderWindow()->Render();
}

SurfaceViewerWidget::~SurfaceViewerWidget() { delete vbox1; }

void SurfaceViewerWidget::popup(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command)
{
	// A note about context menus in Qt and the QVTKWidget
	// You may find it easy to just do context menus on right button up,
	// due to the event proxy mechanism in place.

	// That usually works, except in some cases.
	// One case is where you capture context menu events that
	// child windows don't process.  You could end up with a second
	// context menu after the first one.

	// See QVTKWidget::ContextMenuEvent enum which was added after the
	// writing of this example.

	if (!picker)
	{
		picker = vtkSmartPointer<vtkPropPicker>::New();
	}

	// get interactor
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);

	// get event location
	int* position = iren->GetEventPosition();

	// pick from this location.
	if (picker->Pick(position[0], position[1], 0, ren3D) != 0)
	{
		// consume event so the interactor style doesn't get it
		command->AbortFlagOn();
		// get popup menu
		QMenu* popupMenu = static_cast<QMenu*>(client_data);
		// remember to flip y
		int* sz = iren->GetSize();
		QPoint pt = QPoint(position[0], sz[1] - position[1]);
		// map to global
		QPoint global_pt = popupMenu->parentWidget()->mapToGlobal(pt);

		for (auto action : popupMenu->actions())
		{
			if (action->text() == "Select Tissue")
			{
				action->setVisible(input_type == kTissues || input_type == kSelectedTissues);
			}
		}

		// show popup menu at global point
		popupMenu->popup(global_pt);
	}
}

void SurfaceViewerWidget::select_action(QAction* action)
{
	if (picker)
	{
		double* worldPosition = picker->GetPickPosition();

		if (action->text() == "Select Tissue")
		{
			auto surface = discreteCubes->GetOutput();
			vtkIdType pointId = surface->FindPoint(worldPosition);

			if (pointId != -1 && discreteCubes)
			{
				// get tissue type, then select tissue
				if (auto labels = surface->GetPointData()->GetScalars())
				{
					auto tissue_type = static_cast<int>(labels->GetTuple1(pointId));
					hand3D->set_tissue_selection(std::vector<tissues_size_t>(1, tissue_type));
				}
			}
		}
		else if (action->text() == "Select Slice")
		{
			if (input)
			{
				// compute closest slice
				int dims[3];
				double spacing[3], origin[3];
				input->GetDimensions(dims);
				input->GetSpacing(spacing);
				input->GetOrigin(origin);
				int slice = (spacing[2] > 0) ? static_cast<int>(std::round((worldPosition[2] - origin[2]) / spacing[2])) : 0;

				// jump to slice
				hand3D->set_active_slice(std::min(slice, dims[2] - 1), true);
			}
		}
	}
}

void SurfaceViewerWidget::tissue_changed()
{
	if (input_type == kTissues || input_type == kSelectedTissues)
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
	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::thickness_changed(float thick)
{
	Pair p = hand3D->get_pixelsize();
	input->SetSpacing(p.high, p.low, thick);
	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::reload()
{
	// \todo redo this function
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
		input->AllocateScalars(input->GetScalarType(), 1);
		if (input_type == kSource)
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

	if (input_type == kSource)
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

	if (input_type == kSource)
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

	if (input_type == kSource)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) * sl_thresh->value());
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
	if (input_type == kSource)
	{
		(*Actor.rbegin())->GetProperty()->SetOpacity(1.0 - 0.01 * sl_trans->value());
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
	if (input_type == kSource)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) * sl_thresh->value());

		vtkWidget->GetRenderWindow()->Render();
	}
}
