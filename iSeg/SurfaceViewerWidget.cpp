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

#include <vtkBitArray.h>
#include <vtkCellData.h>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkFlyingEdges3D.h>
#include <vtkPointData.h>
#include <vtkUnsignedShortArray.h>

#include <vtkEventQtSlotConnect.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkQuadricLODActor.h>
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

namespace {
template<typename TIn, typename TOut, typename TMap>
void transform_slices(const std::vector<TIn*>& slices, size_t slice_size, TOut* out, const TMap& map)
{
	for (auto slice : slices)
	{
		std::transform(slice, slice + slice_size, out, map);
		std::advance(out, slice_size);
	}
}
template<typename TIn, typename TArray, typename TMap>
void transform_slices_vtk(const std::vector<TIn*>& slices, size_t slice_size, TArray* out, const TMap& map)
{
	size_t idx = 0, i;
	for (auto& slice : slices)
	{
		for (i = 0; i < slice_size; ++i)
		{
			out->SetValue(idx++, map(slice[i]));
		}
	}
}
} // namespace

SurfaceViewerWidget::SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType inputtype, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	input_type = inputtype;
	hand3D = hand3D1;
	vbox1 = new Q3VBox(this);
	vtkWidget = new QVTKWidget(vbox1);
	vtkWidget->setFixedSize(800, 800);

	hbox1 = new Q3HBox(vbox1);
	lb_trans = new QLabel("Transparency", hbox1);
	sl_trans = new QSlider(Qt::Horizontal, hbox1);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(00);
	hbox1->setFixedHeight(hbox1->sizeHint().height());

	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this, SLOT(transp_changed()));

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

	ren3D = vtkSmartPointer<vtkRenderer>::New();
	ren3D->SetBackground(0, 0, 0);
	ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);

	vtkWidget->GetRenderWindow()->AddRenderer(ren3D);
	vtkWidget->GetRenderWindow()->LineSmoothingOn();
	vtkWidget->GetRenderWindow()->SetSize(600, 600);

	style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	iren = vtkSmartPointer<QVTKInteractor>::New();
	iren->SetInteractorStyle(style);
	iren->SetRenderWindow(vtkWidget->GetRenderWindow());

	QMenu* popup_actions = new QMenu(vtkWidget);
	popup_actions->addAction("Select tissue");
	popup_actions->addAction("Go to slice");
	popup_actions->addAction("Mark Point in target");
	connect(popup_actions, SIGNAL(triggered(QAction*)), this, SLOT(select_action(QAction*)));

	connections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
	connections->Connect(vtkWidget->GetRenderWindow()->GetInteractor(), vtkCommand::RightButtonPressEvent,
			this, SLOT(popup(vtkObject*, unsigned long, void*, void*, vtkCommand*)), popup_actions, 1.0);

	// copy input data and setup VTK pipeline
	input = vtkSmartPointer<vtkImageData>::New();
	discreteCubes = vtkSmartPointer<vtkDiscreteFlyingEdges3D>::New();
	cubes = vtkSmartPointer<vtkFlyingEdges3D>::New();

	load();

	vtkWidget->GetRenderWindow()->Render();
}

SurfaceViewerWidget::~SurfaceViewerWidget() { delete vbox1; }

void SurfaceViewerWidget::load()
{
	auto tissue_selection = hand3D->tissue_selection();
	auto spacing = hand3D->spacing();
	size_t slice_size = static_cast<size_t>(hand3D->width()) * hand3D->height();

	input->SetExtent(0, (int)hand3D->width() - 1, 0,
			(int)hand3D->height() - 1, 0,
			(int)hand3D->num_slices() - 1);
	input->SetSpacing(spacing[0], spacing[1], spacing[2]);

	index_tissue_map.clear();

	if (input_type == kSource) // iso-surface
	{
		auto slices = hand3D->source_slices();
		input->AllocateScalars(VTK_FLOAT, 1);
		auto field = (float*)input->GetScalarPointer();
		transform_slices(slices, slice_size, field, [](float v) { return v; });
	}
	else if (input_type == kTarget) // foreground
	{
		auto slices = hand3D->target_slices();
		input->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		auto field = vtkUnsignedCharArray::SafeDownCast(input->GetPointData()->GetScalars());
		transform_slices_vtk(slices, slice_size, field, [](float v) { return v > 0.f ? 1 : 0; });
	}
	else if (input_type == kTissues || tissue_selection.size() > 254) // all tissues
	{
		auto slices = hand3D->tissue_slices(0);
		input->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		auto field = static_cast<tissues_size_t*>(input->GetScalarPointer());

		if (input_type == kTissues)
		{
			transform_slices(slices, slice_size, field, [](tissues_size_t v) { return v; });
		}
		else // selection only
		{
			std::vector<tissues_size_t> tissue_index_map(TissueInfos::GetTissueCount() + 1, 0);
			for (auto tissue_type : tissue_selection)
			{
				tissue_index_map[tissue_type] = tissue_type;
			}
			transform_slices(slices, slice_size, field, [tissue_index_map](tissues_size_t v) { return tissue_index_map.at(v); });
		}
	}
	else if (tissue_selection.size() >= 1) // [1, 254]
	{

		unsigned char count = 1;
		std::vector<unsigned char> tissue_index_map(TissueInfos::GetTissueCount() + 1, 0);
		for (auto tissue_type : tissue_selection)
		{
			index_tissue_map[count] = tissue_type;
			tissue_index_map[tissue_type] = count++;
		}

		auto slices = hand3D->tissue_slices(0);
		input->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		auto field = static_cast<unsigned char*>(input->GetScalarPointer());
		try
		{
			transform_slices(slices, slice_size, field, [tissue_index_map](tissues_size_t v) { return tissue_index_map.at(v); });
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("bad tissue index map " << e.what());
			std::fill_n(field, slices.size() * slice_size, 0);
		}
	}
	else
	{
		input->AllocateScalars(VTK_BIT, 1);
		auto field = vtkBitArray::SafeDownCast(input->GetPointData()->GetScalars());
		field->FillComponent(0, 0);
	}

	// Define all of the variables
	input->GetScalarRange(range);
	startLabel = range[0];
	endLabel = range[1];
	startLabel = 1;

	mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	actor = vtkSmartPointer<vtkQuadricLODActor>::New();

	if (input_type == kSource)
	{
		cubes->SetInputData(input);
		cubes->SetValue(0, range[0] + 0.01 * (range[1] - range[0]) * sl_thresh->value());

		mapper->SetInputConnection(cubes->GetOutputPort());
		mapper->ScalarVisibilityOff();
	}
	else
	{
		discreteCubes->SetInputData(input);
		discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel, endLabel);

		mapper->SetInputConnection(discreteCubes->GetOutputPort());
		if (input_type == kTarget)
		{
			mapper->ScalarVisibilityOff();
		}
		else
		{
			mapper->ScalarVisibilityOn();
			mapper->SetColorModeToMapScalars();
		}

		build_lookuptable();
	}

	actor->GetProperty()->SetOpacity(1.0 - 0.01 * sl_trans->value());

	actor->SetMapper(mapper);
	ren3D->AddActor(actor);
}

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
	auto interactor = vtkRenderWindowInteractor::SafeDownCast(obj);

	// get event location
	int* position = interactor->GetEventPosition();

	// pick from this location.
	if (picker->Pick(position[0], position[1], 0, ren3D) != 0)
	{
		// consume event so the interactor style doesn't get it
		command->AbortFlagOn();
		// get popup menu
		QMenu* popupMenu = static_cast<QMenu*>(client_data);
		// remember to flip y
		int* sz = interactor->GetSize();
		QPoint pt = QPoint(position[0], sz[1] - position[1]);
		// map to global
		QPoint global_pt = popupMenu->parentWidget()->mapToGlobal(pt);

		for (auto action : popupMenu->actions())
		{
			if (action->text().startsWith(QString("Select tissue")))
			{
				action->setVisible(input_type == kTissues || input_type == kSelectedTissues);
				if (action->isVisible())
				{
					int tissue_type = get_picked_tissue();
					if (tissue_type != -1)
					{
						QString dummy;
						auto name = TissueInfos::GetTissueName(tissue_type);
						action->setText(dummy.sprintf("Select tissue (%s)", name.c_str()));
					}
				}
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
		if (action->text().startsWith(QString("Select tissue")))
		{
			int tissue_type = get_picked_tissue();
			if (tissue_type != -1)
			{
				hand3D->set_tissue_selection(std::vector<tissues_size_t>(1, tissue_type));
			}
		}
		else
		{
			if (input)
			{
				double* worldPosition = picker->GetPickPosition();
				int dims[3];
				double spacing[3], origin[3];
				input->GetDimensions(dims);
				input->GetSpacing(spacing);
				input->GetOrigin(origin);
				// compute closest slice
				int slice = static_cast<int>(std::round((worldPosition[2] - origin[2]) / spacing[2]));
				slice = std::max(0, std::min(slice, dims[2] - 1));

				if (action->text() == "Mark Point in target")
				{
					std::vector<float> work(hand3D->return_area(), 0);

					auto i = static_cast<int>(std::round((worldPosition[0] - origin[0]) / spacing[0]));
					auto j = static_cast<int>(std::round((worldPosition[1] - origin[1]) / spacing[1]));
					auto idx = static_cast<unsigned>(i + j * hand3D->width());
					if (idx < hand3D->return_area())
					{
						work[idx] = 255.f;
					}
					iseg::DataSelection dataSelection;
					dataSelection.sliceNr = slice;
					dataSelection.work = true;
					emit begin_datachange(dataSelection, this, false);

					hand3D->copy2work(slice, work.data(), 2);

					emit end_datachange(this, iseg::ClearUndo);
				}

				// jump to slice
				hand3D->set_active_slice(slice, true);
			}
		}
	}
}

void SurfaceViewerWidget::tissue_changed()
{
	if (input_type == kTissues || input_type == kSelectedTissues)
	{
		build_lookuptable();

		vtkWidget->GetRenderWindow()->Render();
	}
}

void SurfaceViewerWidget::build_lookuptable()
{
	int const tableSize = endLabel + 1;

	lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetNumberOfTableValues(tableSize);
	lut->Build();

	lut->SetTableValue(0, 0.1, 0.1, 0.1, 1); // background
	if (input_type == kTarget)
	{
		lut->SetTableValue(1, 1, 1, 1, 1); // foreground
	}
	else
	{
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			Color tissuecolor;
			if (index_tissue_map.count(i) != 0)
			{
				tissuecolor = TissueInfos::GetTissueColor(index_tissue_map.at(i));
			}
			else
			{
				tissuecolor = TissueInfos::GetTissueColor(i);
			}

			lut->SetTableValue(i, tissuecolor[0], tissuecolor[1], tissuecolor[2], 1);
		}
	}

	mapper->SetScalarRange(0, tableSize - 1);
	mapper->SetLookupTable(lut);
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
	ren3D->RemoveActor(actor);

	load();

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
	actor->GetProperty()->SetOpacity(1.0 - 0.01 * sl_trans->value());

	vtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::thresh_changed()
{
	if (input_type == kSource)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[1] - range[0]) * sl_thresh->value());

		vtkWidget->GetRenderWindow()->Render();
	}
}

int SurfaceViewerWidget::get_picked_tissue() const
{
	double* worldPosition = picker->GetPickPosition();
	if (discreteCubes)
	{
		auto surface = discreteCubes->GetOutput();
		vtkIdType pointId = surface->FindPoint(worldPosition);

		if (pointId != -1)
		{
			// get tissue type, then select tissue
			if (auto labels = surface->GetPointData()->GetScalars())
			{
				auto tissue_type = static_cast<int>(labels->GetTuple1(pointId));
				if (index_tissue_map.count(tissue_type) != 0)
				{
					tissue_type = index_tissue_map.at(tissue_type);
				}
				return tissue_type;
			}
		}
	}
	return -1;
}
