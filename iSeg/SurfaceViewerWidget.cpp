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
#include "SurfaceViewerWidget.h"
#include "TissueInfos.h"

#include "QVTKWidget.h"

#include "../Interface/QtConnect.h"

#include "../Data/Color.h"

#include <QAction>
#include <QHBoxLayout>
#include <QMenu>
#include <QResizeEvent>
#include <QVBoxLayout>

#include <vtkBitArray.h>
#include <vtkCellData.h>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkFlyingEdges3D.h>
#include <vtkPointData.h>
#include <vtkUnsignedShortArray.h>

#include <vtkDecimatePro.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataConnectivityFilter.h>
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

enum eActions {
	kSelectTissue,
	kGotoSlice,
	kMarkPoint
};

} // namespace

SurfaceViewerWidget::SurfaceViewerWidget(SlicesHandler* hand3D1, eInputType inputtype, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	m_InputType = inputtype;
	m_Hand3D = hand3D1;

	m_VtkWidget = new QVTKWidget;
	m_VtkWidget->setMinimumSize(600, 600);

	auto lb_trans = new QLabel("Transparency");
	m_SlTrans = new QSlider(Qt::Horizontal);
	m_SlTrans->setRange(0, 100);
	m_SlTrans->setValue(00);

	m_BtUpdate = new QPushButton("Update");
	m_BtUpdate->setToolTip("Re-extract surface from updated image.");
	m_BtUpdate->setMaximumWidth(200);

	m_BtConnectivity = new QPushButton("Compute connectivity");
	m_BtConnectivity->setToolTip("Compute connectivity and show in different colors");
	m_BtConnectivity->setMaximumWidth(200);

	m_LbConnectivityCount = new QLabel("");
	m_LbConnectivityCount->setToolTip("Number of connected regions");

	m_Reduction = new QLineEdit(QString::number(0));
	m_Reduction->setValidator(new QIntValidator(0, 100));
	m_Reduction->setToolTip("Reduction of number triangles in percent.");

	// set default reduction depending number of voxels.
	const auto n = static_cast<float>(m_Hand3D->ReturnArea()) * m_Hand3D->NumSlices();
	if (n > 1e8)
	{
		m_Reduction->setText(QString::number(80));
	}
	else if (n > 1e6)
	{
		m_Reduction->setText(QString::number(50));
	}

	// layout
	auto vbox = new QVBoxLayout;
	vbox->addWidget(m_VtkWidget);

	auto transparency_hbox = new QHBoxLayout;
	transparency_hbox->addWidget(lb_trans);
	transparency_hbox->addWidget(m_SlTrans);
	vbox->addLayout(transparency_hbox);

	auto reduction_hbox = new QHBoxLayout;
	reduction_hbox->addWidget(m_Reduction);
	vbox->addLayout(reduction_hbox);

	if (m_InputType == kSource)
	{
		auto lb_thresh = new QLabel("Contour iso-value");
		m_SlThresh = new QSlider(Qt::Horizontal);
		m_SlThresh->setRange(0, 100);
		m_SlThresh->setValue(50);

		auto threshold_hbox = new QHBoxLayout;
		threshold_hbox->addWidget(lb_thresh);
		threshold_hbox->addWidget(m_SlThresh);
		vbox->addLayout(threshold_hbox);

		QObject_connect(m_SlThresh, SIGNAL(sliderReleased()), this, SLOT(ThreshChanged()));
	}

	vbox->addWidget(m_BtUpdate);

	auto connectivity_hbox = new QHBoxLayout;
	connectivity_hbox->addWidget(m_BtConnectivity);
	connectivity_hbox->addWidget(m_LbConnectivityCount);
	vbox->addLayout(connectivity_hbox);

	setLayout(vbox);

	// connections
	QObject_connect(m_SlTrans, SIGNAL(sliderReleased()), this, SLOT(TranspChanged()));
	QObject_connect(m_BtUpdate, SIGNAL(clicked()), this, SLOT(Reload()));
	QObject_connect(m_BtConnectivity, SIGNAL(clicked()), this, SLOT(SplitSurface()));
	QObject_connect(m_Reduction, SIGNAL(editingFinished()), this, SLOT(reduction_changed));

	// setup vtk scene
	m_Ren3D = vtkSmartPointer<vtkRenderer>::New();
	m_Ren3D->SetBackground(0, 0, 0);
	m_Ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);

	m_VtkWidget->GetRenderWindow()->AddRenderer(m_Ren3D);
	m_VtkWidget->GetRenderWindow()->LineSmoothingOn();
	m_VtkWidget->GetRenderWindow()->SetSize(600, 600);

	m_Style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	m_Iren = vtkSmartPointer<QVTKInteractor>::New();
	m_Iren->SetInteractorStyle(m_Style);
	m_Iren->SetRenderWindow(m_VtkWidget->GetRenderWindow());

	QMenu* popup_actions = new QMenu(m_VtkWidget);
	popup_actions->addAction("Select tissue")->setData(eActions::kSelectTissue);
	popup_actions->addAction("Go to slice")->setData(eActions::kGotoSlice);
	popup_actions->addAction("Add mark")->setData(eActions::kMarkPoint);
	QObject_connect(popup_actions, SIGNAL(triggered(QAction*)), this, SLOT(SelectAction(QAction*)));

	m_Connections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
	m_Connections->Connect(m_VtkWidget->GetRenderWindow()->GetInteractor(), vtkCommand::RightButtonPressEvent, this, SLOT(Popup(vtkObject*,unsigned long,void*,void*,vtkCommand*)), popup_actions, 1.0);

	// copy input data and setup VTK pipeline
	m_Input = vtkSmartPointer<vtkImageData>::New();
	m_DiscreteCubes = vtkSmartPointer<vtkDiscreteFlyingEdges3D>::New();
	m_Cubes = vtkSmartPointer<vtkFlyingEdges3D>::New();
	m_Decimate = vtkSmartPointer<vtkDecimatePro>::New();
	m_Decimate->PreserveTopologyOn();
	m_Decimate->BoundaryVertexDeletionOff();
	m_Decimate->SplittingOff();
	m_Decimate->SetTargetReduction(m_Reduction->text().toDouble() / 100.0);

	Load();

	m_VtkWidget->GetRenderWindow()->Render();
}

SurfaceViewerWidget::~SurfaceViewerWidget()
= default;

bool SurfaceViewerWidget::IsOpenGlSupported()
{
	// todo: check e.g. via some helper process
	return true;
}

void SurfaceViewerWidget::Load()
{
	auto tissue_selection = m_Hand3D->TissueSelection();
	auto spacing = m_Hand3D->Spacing();
	size_t slice_size = static_cast<size_t>(m_Hand3D->Width()) * m_Hand3D->Height();

	m_Input->SetExtent(0, (int)m_Hand3D->Width() - 1, 0, (int)m_Hand3D->Height() - 1, 0, (int)m_Hand3D->NumSlices() - 1);
	m_Input->SetSpacing(spacing[0], spacing[1], spacing[2]);

	m_IndexTissueMap.clear();

	if (m_InputType == kSource) // iso-surface
	{
		auto slices = m_Hand3D->SourceSlices();
		m_Input->AllocateScalars(VTK_FLOAT, 1);
		auto field = (float*)m_Input->GetScalarPointer();
		transform_slices(slices, slice_size, field, [](float v) { return v; });
	}
	else if (m_InputType == kTarget) // foreground
	{
		auto slices = m_Hand3D->TargetSlices();
		m_Input->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		auto field = vtkUnsignedCharArray::SafeDownCast(m_Input->GetPointData()->GetScalars());
		transform_slices_vtk(slices, slice_size, field, [](float v) { return v > 0.f ? 1 : 0; });
	}
	else if (tissue_selection.size() > 254) // all tissues
	{
		auto slices = m_Hand3D->TissueSlices(0);
		m_Input->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		auto field = static_cast<tissues_size_t*>(m_Input->GetScalarPointer());

		std::vector<tissues_size_t> tissue_index_map(TissueInfos::GetTissueCount() + 1, 0);
		for (auto tissue_type : tissue_selection)
		{
			tissue_index_map[tissue_type] = tissue_type;
		}
		transform_slices(slices, slice_size, field, [tissue_index_map](tissues_size_t v) { return tissue_index_map.at(v); });
	}
	else if (!tissue_selection.empty()) // [1, 254]
	{
		unsigned char count = 1;
		std::vector<unsigned char> tissue_index_map(TissueInfos::GetTissueCount() + 1, 0);
		for (auto tissue_type : tissue_selection)
		{
			m_IndexTissueMap[count] = tissue_type;
			tissue_index_map[tissue_type] = count++;
		}

		auto slices = m_Hand3D->TissueSlices(0);
		m_Input->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		auto field = static_cast<unsigned char*>(m_Input->GetScalarPointer());
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
		m_Input->AllocateScalars(VTK_BIT, 1);
		auto field = vtkBitArray::SafeDownCast(m_Input->GetPointData()->GetScalars());
		field->FillComponent(0, 0);
	}

	// Define all of the variables
	m_Input->GetScalarRange(m_Range);
	m_StartLabel = m_Range[0];
	m_EndLabel = m_Range[1];
	m_StartLabel = 1;

	m_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_Actor = vtkSmartPointer<vtkQuadricLODActor>::New();

	if (m_InputType == kSource)
	{
		m_Cubes->SetInputData(m_Input);
		m_Cubes->SetValue(0, m_Range[0] + 0.01 * (m_Range[1] - m_Range[0]) * m_SlThresh->value());

		m_Decimate->SetInputConnection(m_Cubes->GetOutputPort());

		m_Mapper->SetInputConnection(m_Decimate->GetOutputPort());
		m_Mapper->ScalarVisibilityOff();
	}
	else
	{
		m_DiscreteCubes->SetInputData(m_Input);
		m_DiscreteCubes->GenerateValues(m_EndLabel - m_StartLabel + 1, m_StartLabel, m_EndLabel);

		// if split surface
		// merge duplicate points (check if necessary)
		// connectivity filter & set random colors
		// mapper set input to connectivity output
		m_Decimate->SetInputConnection(m_DiscreteCubes->GetOutputPort());

		m_Mapper->SetInputConnection(m_Decimate->GetOutputPort());
		if (m_InputType == kTarget)
		{
			m_Mapper->ScalarVisibilityOff();
		}
		else
		{
			m_Mapper->ScalarVisibilityOn();
			m_Mapper->SetColorModeToMapScalars();
		}

		BuildLookuptable();
	}

	m_Actor->GetProperty()->SetOpacity(1.0 - 0.01 * m_SlTrans->value());

	m_Actor->SetMapper(m_Mapper);
	m_Ren3D->AddActor(m_Actor);
}

void SurfaceViewerWidget::SplitSurface()
{
	auto connectivity = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
	connectivity->SetInputConnection(m_Mapper->GetInputConnection(0, 0));
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
		c = Color::NextRandom(c);
	}

	auto output = vtkSmartPointer<vtkPolyData>::New();
	output->ShallowCopy(connectivity->GetOutput());

	m_Mapper->SetInputData(output);
	m_Mapper->ScalarVisibilityOn();
	m_Mapper->SetColorModeToMapScalars();
	m_Mapper->SetScalarRange(0, num_regions - 1);
	m_Mapper->SetLookupTable(lut);

	m_VtkWidget->GetRenderWindow()->Render();

	m_LbConnectivityCount->setText(QString("Disconnected Regions = %1").arg(num_regions));
}

void SurfaceViewerWidget::Popup(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command)
{
	// A note about context menus in Qt and the QVTKWidget
	// You may find it easy to just do context menus on right button up, // due to the event proxy mechanism in place.

	// That usually works, except in some cases.
	// One case is where you capture context menu events that
	// child windows don't process.  You could end up with a second
	// context menu after the first one.

	// See QVTKWidget::ContextMenuEvent enum which was added after the
	// writing of this example.

	if (!m_Picker)
	{
		m_Picker = vtkSmartPointer<vtkPropPicker>::New();
	}

	// get interactor
	auto interactor = vtkRenderWindowInteractor::SafeDownCast(obj);

	// get event location
	int* position = interactor->GetEventPosition();

	// pick from this location.
	if (m_Picker->Pick(position[0], position[1], 0, m_Ren3D) != 0)
	{
		// consume event so the interactor style doesn't get it
		command->AbortFlagOn();
		// get popup menu
		QMenu* popup_menu = static_cast<QMenu*>(client_data);
		// remember to flip y
		int* sz = interactor->GetSize();
		QPoint pt = QPoint(position[0], sz[1] - position[1]);
		// map to global
		QPoint global_pt = popup_menu->parentWidget()->mapToGlobal(pt);

		for (auto action : popup_menu->actions())
		{
			if (action->data() == eActions::kSelectTissue)
			{
				action->setVisible(m_InputType == kSelectedTissues);
				if (action->isVisible())
				{
					int tissue_type = GetPickedTissue();
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
		popup_menu->popup(global_pt);
	}
}

void SurfaceViewerWidget::SelectAction(QAction* action)
{
	if (m_Picker)
	{
		if (action->data() == eActions::kSelectTissue)
		{
			int tissue_type = GetPickedTissue();
			if (tissue_type != -1)
			{
				m_Hand3D->SetTissueSelection(std::vector<tissues_size_t>(1, tissue_type));
			}
		}
		else // eActions::kMarkPoint or kGotoSlice
		{
			if (m_Input)
			{
				double* world_position = m_Picker->GetPickPosition();
				int dims[3];
				double spacing[3], origin[3];
				m_Input->GetDimensions(dims);
				m_Input->GetSpacing(spacing);
				m_Input->GetOrigin(origin);
				// compute closest slice
				int slice = static_cast<int>(std::round((world_position[2] - origin[2]) / spacing[2]));
				slice = std::max(0, std::min(slice, dims[2] - 1));

				if (action->data() == eActions::kMarkPoint)
				{
					Point p;
					p.px = static_cast<int>(std::round((world_position[0] - origin[0]) / spacing[0]));
					p.py = static_cast<int>(std::round((world_position[1] - origin[1]) / spacing[1]));

					DataSelection data_selection;
					data_selection.sliceNr = slice;
					data_selection.marks = true;
					emit BeginDatachange(data_selection, this, false);

					m_Hand3D->AddMark(p, Mark::red, slice);

					emit EndDatachange(this, iseg::ClearUndo);
				}

				// jump to slice
				m_Hand3D->SetActiveSlice(slice, true);
			}
		}
	}
}

void SurfaceViewerWidget::TissueChanged()
{
	if (m_InputType == kSelectedTissues)
	{
		// only update colors, don't auto update surface
		BuildLookuptable();

		m_VtkWidget->GetRenderWindow()->Render();
	}
}

void SurfaceViewerWidget::BuildLookuptable()
{
	int const table_size = m_EndLabel + 1;

	m_Lut = vtkSmartPointer<vtkLookupTable>::New();
	m_Lut->SetNumberOfTableValues(table_size);
	m_Lut->Build();

	m_Lut->SetTableValue(0, 0.1, 0.1, 0.1, 1); // background
	if (m_InputType == kTarget)
	{
		m_Lut->SetTableValue(1, 1, 1, 1, 1); // foreground
	}
	else
	{
		for (unsigned int i = m_StartLabel; i <= m_EndLabel; i++)
		{
			Color tissuecolor;
			if (m_IndexTissueMap.count(i) != 0)
			{
				tissuecolor = TissueInfos::GetTissueColor(m_IndexTissueMap.at(i));
			}
			else
			{
				tissuecolor = TissueInfos::GetTissueColor(i);
			}

			m_Lut->SetTableValue(i, tissuecolor[0], tissuecolor[1], tissuecolor[2], 1);
		}
	}

	m_Mapper->SetScalarRange(0, table_size - 1);
	m_Mapper->SetLookupTable(m_Lut);
}

void SurfaceViewerWidget::PixelsizeChanged(Pair p)
{
	m_Input->SetSpacing(p.high, p.low, m_Hand3D->GetSlicethickness());
	m_Input->Modified();

	m_VtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::ThicknessChanged(float thick)
{
	Pair p = m_Hand3D->GetPixelsize();
	m_Input->SetSpacing(p.high, p.low, thick);
	m_Input->Modified();

	m_VtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::Reload()
{
	m_Ren3D->RemoveActor(m_Actor);

	Load();

	m_Input->Modified();

	m_VtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::closeEvent(QCloseEvent* qce)
{
	emit Hasbeenclosed();
	QWidget::closeEvent(qce);
}

void SurfaceViewerWidget::TranspChanged()
{
	m_Actor->GetProperty()->SetOpacity(1.0 - 0.01 * m_SlTrans->value());

	m_VtkWidget->GetRenderWindow()->Render();
}

void SurfaceViewerWidget::ThreshChanged()
{
	if (m_InputType == kSource)
	{
		m_Cubes->SetValue(0, m_Range[0] + 0.01 * (m_Range[1] - m_Range[0]) * m_SlThresh->value());

		m_VtkWidget->GetRenderWindow()->Render();
	}
}

void SurfaceViewerWidget::ReductionChanged()
{
	m_Decimate->SetTargetReduction(m_Reduction->text().toDouble() / 100.0);

	m_VtkWidget->GetRenderWindow()->Render();
}

int SurfaceViewerWidget::GetPickedTissue() const
{
	double* world_position = m_Picker->GetPickPosition();
	if (m_DiscreteCubes)
	{
		auto surface = m_DiscreteCubes->GetOutput();
		vtkIdType point_id = surface->FindPoint(world_position);

		if (point_id != -1)
		{
			// get tissue type, then select tissue
			if (auto labels = surface->GetPointData()->GetScalars())
			{
				auto tissue_type = static_cast<int>(labels->GetTuple1(point_id));
				if (m_IndexTissueMap.count(tissue_type) != 0)
				{
					tissue_type = m_IndexTissueMap.at(tissue_type);
				}
				return tissue_type;
			}
		}
	}
	return -1;
}

} // namespace iseg
