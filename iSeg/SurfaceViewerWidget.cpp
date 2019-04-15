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

#include "../Data/Color.h"

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

#include <vtkPolyDataConnectivityFilter.h>

#include <vtkAutoInit.h>
#ifdef ISEG_VTK_OPENGL2
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
#else
VTK_MODULE_INIT(vtkRenderingOpenGL); // VTK was built with vtkRenderingOpenGL
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL);
#endif
VTK_MODULE_INIT(vtkInteractionStyle);

namespace iseg {

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

enum eActions
{
	kSelectTissue,
	kGotoSlice,
	kMarkPoint
};

} // namespace

SurfaceViewerWidget::SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType inputtype, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	input_type = inputtype;
	hand3D = hand3D1;

	vtkWidget = new QVTKWidget;
	vtkWidget->setMinimumSize(600, 600);

	auto lb_trans = new QLabel("Transparency");
	sl_trans = new QSlider(Qt::Horizontal);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(00);

	bt_update = new QPushButton("Update");
	bt_update->setToolTip("Re-extract surface from updated image.");
	bt_update->setMaximumWidth(200);

	bt_connectivity = new QPushButton("Compute connectivity");
	bt_connectivity->setToolTip("Compute connectivity and show in different colors");
	bt_connectivity->setMaximumWidth(200);

	// layout
	auto vbox = new QVBoxLayout;
	vbox->addWidget(vtkWidget);

	auto transparency_hbox = new QHBoxLayout;
	transparency_hbox->addWidget(lb_trans);
	transparency_hbox->addWidget(sl_trans);
	vbox->addLayout(transparency_hbox);

	if (input_type == kSource)
	{
		auto lb_thresh = new QLabel("Contour iso-value");
		sl_thresh = new QSlider(Qt::Horizontal);
		sl_thresh->setRange(0, 100);
		sl_thresh->setValue(50);

		auto threshold_hbox = new QHBoxLayout;
		threshold_hbox->addWidget(lb_thresh);
		threshold_hbox->addWidget(sl_thresh);
		vbox->addLayout(threshold_hbox);

		QObject::connect(sl_thresh, SIGNAL(sliderReleased()), this, SLOT(thresh_changed()));
	}

	vbox->addWidget(bt_update);
	vbox->addWidget(bt_connectivity);

	setLayout(vbox);

	// connections
	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this, SLOT(transp_changed()));
	QObject::connect(bt_update, SIGNAL(clicked()), this, SLOT(reload()));
	QObject::connect(bt_connectivity, SIGNAL(clicked()), this, SLOT(split_surface()));

	// setup vtk scene
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
	popup_actions->addAction("Select tissue")->setData(eActions::kSelectTissue);
	popup_actions->addAction("Go to slice")->setData(eActions::kGotoSlice);
	popup_actions->addAction("Add mark")->setData(eActions::kMarkPoint);
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

SurfaceViewerWidget::~SurfaceViewerWidget() 
{

}

bool SurfaceViewerWidget::isOpenGLSupported()
{
	// todo: check e.g. via some helper process 
	return true;
}

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
	else if (tissue_selection.size() > 254) // all tissues
	{
		auto slices = hand3D->tissue_slices(0);
		input->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		auto field = static_cast<tissues_size_t*>(input->GetScalarPointer());

		std::vector<tissues_size_t> tissue_index_map(TissueInfos::GetTissueCount() + 1, 0);
		for (auto tissue_type : tissue_selection)
		{
			tissue_index_map[tissue_type] = tissue_type;
		}
		transform_slices(slices, slice_size, field, [tissue_index_map](tissues_size_t v) { return tissue_index_map.at(v); });
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

		// if split surface
		// merge duplicate points (check if necessary)
		// connectivity filter & set random colors
		// mapper set input to connectivity output

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

void SurfaceViewerWidget::split_surface()
{
	auto connectivity = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
	connectivity->SetInputConnection(mapper->GetInputConnection(0, 0));
	connectivity->SetExtractionModeToAllRegions();
	connectivity->ScalarConnectivityOff();
	connectivity->ColorRegionsOn();
	connectivity->Update();

	auto num_regions = connectivity->GetNumberOfExtractedRegions();
	ISEG_INFO("Number of disconnected regions: " << num_regions);
	
	// attach lookuptable
	auto lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetNumberOfTableValues(num_regions);
	lut->SetNumberOfColors(num_regions);
	Color c(0.1f, 0.9f, 0.1f);
	for (vtkIdType i = 0; i < num_regions; i++)
	{
		lut->SetTableValue(i, c[0], c[1], c[2], 1.0);
		c = Color::nextRandom(c);
	}

	auto output = vtkSmartPointer<vtkPolyData>::New();
	output->ShallowCopy(connectivity->GetOutput());
	
	mapper->SetInputData(output);
	mapper->ScalarVisibilityOn();
	mapper->SetColorModeToMapScalars();
	mapper->SetScalarRange(0, num_regions - 1);
	mapper->SetLookupTable(lut);

	vtkWidget->GetRenderWindow()->Render();
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
			if (action->data() == eActions::kSelectTissue)
			{
				action->setVisible(input_type == kSelectedTissues);
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
		if (action->data() == eActions::kSelectTissue)
		{
			int tissue_type = get_picked_tissue();
			if (tissue_type != -1)
			{
				hand3D->set_tissue_selection(std::vector<tissues_size_t>(1, tissue_type));
			}
		}
		else // eActions::kMarkPoint or kGotoSlice
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

				if (action->data() == eActions::kMarkPoint)
				{
					Point p;
					p.px = static_cast<int>(std::round((worldPosition[0] - origin[0]) / spacing[0]));
					p.py = static_cast<int>(std::round((worldPosition[1] - origin[1]) / spacing[1]));

					iseg::DataSelection dataSelection;
					dataSelection.sliceNr = slice;
					dataSelection.marks = true;
					emit begin_datachange(dataSelection, this, false);

					hand3D->add_mark(p, Mark::RED, slice);

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
	if (input_type == kSelectedTissues)
	{
		// only update colors, don't auto update surface
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

}// namespace iseg