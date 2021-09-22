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

#include "OutlineCorrectionParameterViews.h"
#include "OutlineCorrectionWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Data/AddConnected.h"
#include "Data/ExtractBoundary.h"
#include "Data/ItkProgressObserver.h"
#include "Data/ItkUtils.h"
#include "Data/Point.h"
#include "Data/SlicesHandlerITKInterface.h"
#include "Data/addLine.h"

#include "Core/SmoothTissues.h"
#include "Core/itkFixTopologyCarveInside.h"
#include "Core/itkFixTopologyCarveOutside.h"

#include "Interface/ProgressDialog.h"

#include <itkBinaryThresholdImageFilter.h>
#include <itkMeanImageFilter.h>
#include <itkSliceBySliceImageFilter.h>
#include <itkExtractImageFilter.h>

#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedWidget>

namespace iseg {

std::map<QAbstractButton*, int> widget_page;

OutlineCorrectionWidget::OutlineCorrectionWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("OutLine Correction routines that can be used to modify "
										"the result of a segmentation operation and to correct frequently "
										"occurring segmentation deficiencies."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	// methods
	m_Olcorr = new QRadioButton(tr("Outline Correction"));
	m_Olcorr->setToolTip(Format("Draw an alternative boundary segment for a region.This segment "
															"will be connected to the region using the shortest possible lines "
															"and will replace the boundary segment between the connection points."));
	m_Brush = new QRadioButton(tr("Brush"));
	m_Brush->setToolTip(Format("Manual correction and segmentation with brush tools."));

	m_Holefill = new QRadioButton(tr("Fill Holes"));
	m_Holefill->setToolTip(Format("Close all holes in the selected region or tissue that have a "
																"size smaller than the number of pixels specified by the Hole Size option."));

	m_Removeislands = new QRadioButton(tr("Remove Islands"));
	m_Removeislands->setToolTip(Format("Remove all islands (speckles and outliers) with the selected "
																		 "gray value or tissue assignment."));

	m_Gapfill = new QRadioButton(tr("Fill Gaps"));
	m_Gapfill->setToolTip(Format("Close gaps between multiple disconnected regions having the same "
															 "gray value or belonging to the same tissue."));

	m_Addskin = new QRadioButton(tr("Add Skin"));
	m_Addskin->setToolTip(Format("Add a skin layer to a segmentation with a "
															 "specified Thickness (in pixels)."));

	m_Fillskin = new QRadioButton(tr("Fill Skin"));
	m_Fillskin->setToolTip(Format("Thicken a skin layer to ensure it has a minimum Thickness."));

	m_Allfill = new QRadioButton(tr("Fill All"));
	m_Allfill->setToolTip(Format("Make sure that there are no unassigned regions "
															 "inside the segmented model."));

	m_Spherize = new QRadioButton("Spherize");
	m_Spherize->setToolTip(Format("Simplify voxel set by making small changes (add/remove voxels),"
																"which make the voxel set manifold and topologically equivalent to a sphere."));

	m_SmoothTissues = new QRadioButton(tr("Smooth Tissues"));
	m_SmoothTissues->setToolTip(Format("This tool smooths all non-locked tissues but does not modify locked tissues."));

	auto method_radio_buttons = {m_Brush, m_Olcorr, m_Holefill, m_Removeislands, m_Gapfill, m_Addskin, m_Fillskin, m_Allfill, m_Spherize, m_SmoothTissues};
	m_Methods = new QButtonGroup(this);
	for (auto w : method_radio_buttons)
	{
		m_Methods->addButton(w);
	}

	auto method_vbox = new QVBoxLayout;
	for (auto w : method_radio_buttons)
	{
		method_vbox->addWidget(w);
	}
	method_vbox->setMargin(5);

	auto method_area = new QFrame(this);
	method_area->setLayout(method_vbox);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	// parameter views
	m_OlcParams = new OLCorrParamView;
	m_BrushParams = new BrushParamView;
	m_FillHolesParams = new FillHolesParamView;
	m_RemoveIslandsParams = new FillHolesParamView;
	m_RemoveIslandsParams->m_ObjectSizeLabel->setText("Island size");
	m_FillGapsParams = new FillHolesParamView;
	m_FillGapsParams->m_ObjectSizeLabel->setText("Gap size");
	m_AddSkinParams = new AddSkinParamView;
	m_FillSkinParams = new FillSkinParamView(m_Handler3D);
	m_FillAllParams = new FillAllParamView;
	m_SpherizeParams = new SpherizeParamView;
	m_SmoothTissuesParams = new SmoothTissuesParamView;

	// layouts
	m_StackedParams = new QStackedWidget(this);
	m_StackedParams->addWidget(m_BrushParams);
	m_StackedParams->addWidget(m_OlcParams);
	m_StackedParams->addWidget(m_FillHolesParams);
	m_StackedParams->addWidget(m_RemoveIslandsParams);
	m_StackedParams->addWidget(m_FillGapsParams);
	m_StackedParams->addWidget(m_AddSkinParams);
	m_StackedParams->addWidget(m_FillSkinParams);
	m_StackedParams->addWidget(m_FillAllParams);
	m_StackedParams->addWidget(m_SpherizeParams);
	m_StackedParams->addWidget(m_SmoothTissuesParams);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addWidget(m_StackedParams);
	setLayout(top_layout);

	// remember QStackedLayout page where parameters of method are added
	for (int i = 0; i < m_Methods->buttons().size(); ++i)
	{
		widget_page[m_Methods->buttons().at(i)] = i;
	}

	// start with brush tool
	m_Brush->setChecked(true);
	m_StackedParams->setCurrentWidget(m_BrushParams);

	// create connections
	QObject_connect(m_Methods, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));
	QObject_connect(m_OlcParams->m_SelectObject, SIGNAL(clicked()), this, SLOT(SelectobjPushed()));
	QObject_connect(m_BrushParams->m_SelectObject, SIGNAL(clicked()), this, SLOT(SelectobjPushed()));
	QObject_connect(m_FillHolesParams->m_SelectObject, SIGNAL(clicked()), this, SLOT(SelectobjPushed()));
	QObject_connect(m_RemoveIslandsParams->m_SelectObject, SIGNAL(clicked()), this, SLOT(SelectobjPushed()));
	QObject_connect(m_FillGapsParams->m_SelectObject, SIGNAL(clicked()), this, SLOT(SelectobjPushed()));
	QObject_connect(m_SpherizeParams->m_SelectObject, SIGNAL(clicked()), this, SLOT(SelectobjPushed()));

	QObject_connect(m_BrushParams->m_CopyGuide, SIGNAL(clicked()), this, SLOT(CopyGuide()));
	QObject_connect(m_BrushParams->m_CopyPickGuide, SIGNAL(clicked()), this, SLOT(CopyPickPushed()));
	QObject_connect(m_BrushParams->m_ShowGuide, SIGNAL(clicked()), this, SLOT(DrawGuide()));
	QObject_connect(m_BrushParams->m_GuideOffset, SIGNAL(valueChanged(int)), this, SLOT(DrawGuide()));

	QObject_connect(m_FillHolesParams->m_Execute, SIGNAL(clicked()), this, SLOT(ExecutePushed()));
	QObject_connect(m_RemoveIslandsParams->m_Execute, SIGNAL(clicked()), this, SLOT(ExecutePushed()));
	QObject_connect(m_FillGapsParams->m_Execute, SIGNAL(clicked()), this, SLOT(ExecutePushed()));
	QObject_connect(m_AddSkinParams->m_Execute, SIGNAL(clicked()), this, SLOT(ExecutePushed()));
	QObject_connect(m_FillSkinParams->m_Execute, SIGNAL(clicked()), this, SLOT(ExecutePushed()));
	QObject_connect(m_FillAllParams->m_Execute, SIGNAL(clicked()), this, SLOT(ExecutePushed()));
	QObject_connect(m_SpherizeParams->m_Execute, SIGNAL(clicked()), this, SLOT(CarvePushed()));
	QObject_connect(m_SmoothTissuesParams->m_Execute, SIGNAL(clicked()), this, SLOT(SmoothTissuesPushed()));

	m_Selectobj = false;

	MethodChanged();
	WorkbitsChanged();
}

void OutlineCorrectionWidget::DrawCircle(Point p)
{
	m_Vpdyn = m_BrushParams->DrawCircle(p, m_Spacing[0], m_Spacing[1], m_Bmphand->ReturnWidth(), m_Bmphand->ReturnHeight());

	emit VpdynChanged(&m_Vpdyn);
	m_Vpdyn.clear();
}

void OutlineCorrectionWidget::DrawGuide()
{
	if (m_Brush->isChecked() && m_BrushParams->m_ShowGuide->isChecked())
	{
		int slice = static_cast<int>(m_Handler3D->ActiveSlice()) + m_BrushParams->m_GuideOffset->value();
		unsigned slice_clamped = std::min(std::max(slice, 0), m_Handler3D->NumSlices() - 1);
		if (slice == slice_clamped)
		{
			std::vector<Mark> marks;
			auto w = m_Handler3D->Width();
			auto h = m_Handler3D->Height();
			if (m_BrushParams->m_Target->isChecked())
			{
				Mark m(Mark::white);
				marks = extract_boundary<Mark, float>(m_Handler3D->TargetSlices().at(slice_clamped), w, h, m);
			}
			else
			{
				Mark m(m_Tissuenr);
				marks = extract_boundary<Mark, tissues_size_t>(m_Handler3D->TissueSlices(0).at(slice_clamped), w, h, m, [this](tissues_size_t v) { return (v == m_Tissuenr); });
			}

			emit VmChanged(&marks);

			return;
		}
	}

	std::vector<Mark> marks;
	emit VmChanged(&marks);
}

void OutlineCorrectionWidget::CopyGuide(Point* p)
{
	int slice = static_cast<int>(m_Handler3D->ActiveSlice()) + m_BrushParams->m_GuideOffset->value();
	unsigned slice_clamped = std::min(std::max(slice, 0), m_Handler3D->NumSlices() - 1);
	if (slice == slice_clamped)
	{
		unsigned w = m_Handler3D->Width();
		unsigned h = m_Handler3D->Height();

		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = m_BrushParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;
		emit BeginDatachange(data_selection, this);

		if (data_selection.work)
		{
			auto ref = m_Handler3D->TargetSlices().at(slice_clamped);
			auto current = m_Handler3D->TargetSlices().at(m_Handler3D->ActiveSlice());

			if (p)
			{
				unsigned pos = p->px + w * p->py;
				add_connected_2d(ref, current, w, h, pos, 255.f, [&](unsigned idx) { return (ref[idx] != 0.f); });
			}
			else
			{
				std::transform(ref, ref + w * h, current, current, [](float r, float c) { return (r != 0.f ? r : c); });
			}
		}
		else
		{
			auto ref = m_Handler3D->TissueSlices(0).at(slice_clamped);
			auto current = m_Handler3D->TissueSlices(0).at(m_Handler3D->ActiveSlice());

			if (p)
			{
				unsigned pos = p->px + w * p->py;
				std::vector<tissues_size_t> temp(w * h, 0);
				add_connected_2d(ref, temp.data(), w, h, pos, m_Tissuenr, [&](unsigned idx) { return ref[idx] == m_Tissuenr; });

				std::transform(temp.begin(), temp.end(), current, current, [this](tissues_size_t r, tissues_size_t c) {
					return (r == m_Tissuenr && !TissueInfos::GetTissueLocked(c)) ? r : c;
				});
			}
			else
			{
				std::transform(ref, ref + w * h, current, current, [this](tissues_size_t r, tissues_size_t c) {
					return (r == m_Tissuenr && !TissueInfos::GetTissueLocked(c)) ? r : c;
				});
			}
		}

		emit EndDatachange(this);
	}
	else
	{
		//BL
	}
}

void OutlineCorrectionWidget::OnMouseClicked(Point p)
{
	// update spacing when we start interaction
	m_Spacing = m_Handler3D->Spacing();

	if (m_CopyMode)
	{
		m_CopyMode = false;
		CopyGuide(&p);
		m_BrushParams->m_CopyPickGuide->setChecked(false);
		return;
	}
	if (m_Selectobj)
	{
		if (m_CurrentParams)
		{
			auto v = m_Bmphand->WorkPt(p);
			m_CurrentParams->SetObjectValue(v);
		}
		return;
	}

	if (m_Olcorr->isChecked())
	{
		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = m_OlcParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;

		m_Vpdyn.clear();
		m_LastPt = p;
		m_Vpdyn.push_back(p);
		emit BeginDatachange(data_selection, this);
	}
	else if (m_Brush->isChecked())
	{
		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = m_BrushParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;

		float const f = GetObjectValue();
		float const radius = m_BrushParams->m_Radius->text().toFloat();
		m_LastPt = p;

		if (m_BrushParams->m_Modify->isChecked())
		{
			if (m_BrushParams->m_Target->isChecked())
			{
				if (m_Bmphand->WorkPt(p) == f)
					m_Draw = true;
				else
					m_Draw = false;
			}
			else
			{
				if (m_Bmphand->TissuesPt(m_Handler3D->ActiveTissuelayer(), p) == m_Tissuenr)
				{
					m_Draw = true;
				}
				else
				{
					m_Draw = false;
					m_Tissuenrnew = m_Bmphand->TissuesPt(m_Handler3D->ActiveTissuelayer(), p);
				}
			}
		}
		else if (m_BrushParams->m_Draw->isChecked())
		{
			m_Draw = true;
		}
		else
		{
			m_Draw = false;
			if (m_Bmphand->TissuesPt(m_Handler3D->ActiveTissuelayer(), p) == m_Tissuenr)
				m_Tissuenrnew = 0;
			else
				m_Tissuenrnew = m_Bmphand->TissuesPt(m_Handler3D->ActiveTissuelayer(), p);
		}

		emit BeginDatachange(data_selection, this);
		if (m_BrushParams->m_Target->isChecked())
		{
			if (m_BrushParams->m_UnitMm->isChecked())
				m_Bmphand->Brush(f, p, radius, m_Spacing[0], m_Spacing[1], m_Draw);
			else
				m_Bmphand->Brush(f, p, static_cast<int>(radius), m_Draw);
		}
		else
		{
			auto idx = m_Handler3D->ActiveTissuelayer();
			if (m_BrushParams->m_UnitMm->isChecked())
				m_Bmphand->Brushtissue(idx, m_Tissuenr, p, radius, m_Spacing[0], m_Spacing[1], m_Draw, m_Tissuenrnew);
			else
				m_Bmphand->Brushtissue(idx, m_Tissuenr, p, static_cast<int>(radius), m_Draw, m_Tissuenrnew);
		}
		emit EndDatachange(this, iseg::NoUndo);

		DrawCircle(p);
	}
}

void OutlineCorrectionWidget::OnMouseMoved(Point p)
{
	if (!m_Selectobj && !m_CopyMode)
	{
		float const f = GetObjectValue();
		float const radius = m_BrushParams->m_Radius->text().toFloat();

		if (m_Olcorr->isChecked())
		{
			m_Vpdyn.pop_back();
			addLine(&m_Vpdyn, m_LastPt, p);
			m_LastPt = p;
			emit VpdynChanged(&m_Vpdyn);
		}
		else if (m_Brush->isChecked())
		{
			DrawCircle(p);

			std::vector<Point> vps;
			vps.clear();
			addLine(&vps, m_LastPt, p);
			for (auto it = ++(vps.begin()); it != vps.end(); it++)
			{
				if (m_BrushParams->m_Target->isChecked())
				{
					if (m_BrushParams->m_UnitMm->isChecked())
						m_Bmphand->Brush(f, *it, radius, m_Spacing[0], m_Spacing[1], m_Draw);
					else
						m_Bmphand->Brush(f, *it, static_cast<int>(radius), m_Draw);
				}
				else
				{
					auto idx = m_Handler3D->ActiveTissuelayer();
					if (m_BrushParams->m_UnitMm->isChecked())
						m_Bmphand->Brushtissue(idx, m_Tissuenr, *it, radius, m_Spacing[0], m_Spacing[1], m_Draw, m_Tissuenrnew);
					else
						m_Bmphand->Brushtissue(idx, m_Tissuenr, *it, static_cast<int>(radius), m_Draw, m_Tissuenrnew);
				}
			}
			emit EndDatachange(this, iseg::NoUndo);
			m_LastPt = p;
		}
	}
}

void OutlineCorrectionWidget::OnMouseReleased(Point p)
{
	if (m_Selectobj || m_CopyMode)
	{
		m_Selectobj = m_CopyMode = false;
	}
	else
	{
		float const f = GetObjectValue();

		if (m_Olcorr->isChecked())
		{
			m_Vpdyn.pop_back();
			addLine(&m_Vpdyn, m_LastPt, p);
			if (m_OlcParams->m_Target->isChecked())
				m_Bmphand->CorrectOutline(f, &m_Vpdyn);
			else
				m_Bmphand->CorrectOutlinetissue(m_Handler3D->ActiveTissuelayer(), m_Tissuenr, &m_Vpdyn);
			emit EndDatachange(this);

			m_Vpdyn.clear();
			emit VpdynChanged(&m_Vpdyn);
		}
		else if (m_Brush->isChecked())
		{
			float const radius = m_BrushParams->m_Radius->text().toFloat();

			m_Vpdyn.clear();
			addLine(&m_Vpdyn, m_LastPt, p);
			if (m_BrushParams->m_UnitMm->isChecked())
			{
				for (auto it = ++(m_Vpdyn.begin()); it != m_Vpdyn.end(); it++)
				{
					if (m_BrushParams->m_Target->isChecked())
					{
						m_Bmphand->Brush(f, *it, radius, m_Spacing[0], m_Spacing[1], m_Draw);
					}
					else
					{
						auto layer = m_Handler3D->ActiveTissuelayer();
						m_Bmphand->Brushtissue(layer, m_Tissuenr, *it, radius, m_Spacing[0], m_Spacing[1], m_Draw, m_Tissuenrnew);
					}
				}
			}
			else
			{
				auto pixel_radius = static_cast<int>(radius);
				for (auto it = ++(m_Vpdyn.begin()); it != m_Vpdyn.end(); it++)
				{
					if (m_BrushParams->m_Target->isChecked())
					{
						m_Bmphand->Brush(f, *it, pixel_radius, m_Draw);
					}
					else
					{
						auto layer = m_Handler3D->ActiveTissuelayer();
						m_Bmphand->Brushtissue(layer, m_Tissuenr, *it, pixel_radius, m_Draw, m_Tissuenrnew);
					}
				}
			}

			emit EndDatachange(this);

			m_Vpdyn.clear();
			emit VpdynChanged(&m_Vpdyn);
		}
	}
}

void OutlineCorrectionWidget::MethodChanged()
{
	if (widget_page.count(m_Methods->checkedButton()))
	{
		m_StackedParams->setCurrentIndex(widget_page[m_Methods->checkedButton()]);
	}

	// keep selected object definition across tools
	auto new_widget = dynamic_cast<ParamViewBase*>(m_StackedParams->currentWidget());
	if (m_CurrentParams)
	{
		new_widget->SetWork(m_CurrentParams->Work());
		new_widget->SetObjectValue(m_CurrentParams->ObjectValue());
	}
	m_CurrentParams = new_widget;
	m_CurrentParams->Init();

	// make sure we are not expecting a mouse click
	m_Selectobj = false;
	// ensure this is reset
	m_CopyMode = false;

	DrawGuide();
}

void OutlineCorrectionWidget::ExecutePushed()
{
	float const f = GetObjectValue();

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();

	if (m_Holefill->isChecked())
	{
		data_selection.allSlices = m_FillHolesParams->m_AllSlices->isChecked();
		data_selection.work = m_FillHolesParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;
		emit BeginDatachange(data_selection, this);

		auto hole_size = m_FillHolesParams->m_ObjectSize->value();
		if (data_selection.allSlices)
		{
			if (data_selection.work)
				m_Handler3D->FillHoles(f, hole_size);
			else
				m_Handler3D->FillHolestissue(m_Tissuenr, hole_size);
		}
		else
		{
			if (data_selection.work)
				m_Bmphand->FillHoles(f, hole_size);
			else
				m_Bmphand->FillHolestissue(m_Handler3D->ActiveTissuelayer(), m_Tissuenr, hole_size);
		}
	}
	else if (m_Removeislands->isChecked())
	{
		data_selection.allSlices = m_RemoveIslandsParams->m_AllSlices->isChecked();
		data_selection.work = m_RemoveIslandsParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;
		emit BeginDatachange(data_selection, this);

		auto island_size = m_RemoveIslandsParams->m_ObjectSize->value();
		if (data_selection.allSlices)
		{
			if (data_selection.work)
				m_Handler3D->RemoveIslands(f, island_size);
			else
				m_Handler3D->RemoveIslandstissue(m_Tissuenr, island_size);
		}
		else
		{
			if (data_selection.work)
				m_Bmphand->RemoveIslands(f, island_size);
			else
				m_Bmphand->RemoveIslandstissue(m_Handler3D->ActiveTissuelayer(), m_Tissuenr, island_size);
		}
	}
	else if (m_Gapfill->isChecked())
	{
		data_selection.allSlices = m_FillGapsParams->m_AllSlices->isChecked();
		data_selection.work = m_FillGapsParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;
		emit BeginDatachange(data_selection, this);

		auto gap_size = m_FillGapsParams->m_ObjectSize->value();
		if (data_selection.allSlices)
		{
			if (data_selection.work)
				m_Handler3D->FillGaps(gap_size, false);
			else
				m_Handler3D->FillGapstissue(gap_size, false);
		}
		else
		{
			if (data_selection.work)
				m_Bmphand->FillGaps(gap_size, false);
			else
				m_Bmphand->FillGapstissue(m_Handler3D->ActiveTissuelayer(), gap_size, false);
		}
	}
	else if (m_Addskin->isChecked())
	{
		data_selection.allSlices = m_AddSkinParams->m_AllSlices->isChecked();
		data_selection.work = m_AddSkinParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;
		emit BeginDatachange(data_selection, this);

		auto radius = m_AddSkinParams->m_Thickness->text().toFloat();
		auto mm_unit = m_AddSkinParams->m_UnitMm->isChecked();
		auto outside = m_AddSkinParams->m_Outside->isChecked();

		int const rx = mm_unit ? static_cast<int>(radius / m_Spacing[0] + 0.1f) : radius;
		int const ry = mm_unit ? static_cast<int>(radius / m_Spacing[1] + 0.1f) : radius;
		int const rz = mm_unit ? static_cast<int>(radius / m_Spacing[2] + 0.1f) : radius;
		bool at_boundary = false;
		if (outside)
		{
			if (data_selection.allSlices)
			{
				if (mm_unit)
				{
					// \warning creates block shaped kernel, instead of ellipsoid
					if (data_selection.work)
					{
						float setto = m_Handler3D->AddSkin3DOutside2(rx, ry, rz);
						at_boundary = m_Handler3D->ValueAtBoundary3D(setto);
					}
					else
					{
						m_Handler3D->AddSkintissue3DOutside2(rx, ry, rz, m_Tissuenr);
						at_boundary = m_Handler3D->TissuevalueAtBoundary3D(m_Tissuenr);
					}
				}
				else
				{
					if (data_selection.work)
					{
						float setto = m_Handler3D->AddSkin3DOutside(rx);
						at_boundary = m_Handler3D->ValueAtBoundary3D(setto);
					}
					else
					{
						m_Handler3D->AddSkintissue3DOutside(rx, m_Tissuenr);
						at_boundary = m_Handler3D->TissuevalueAtBoundary3D(m_Tissuenr);
					}
				}
			}
			else // active slice
			{
				if (data_selection.work)
				{
					float setto = m_Bmphand->AddSkinOutside(rx);
					at_boundary = m_Bmphand->ValueAtBoundary(setto);
				}
				else
				{
					m_Bmphand->AddSkintissueOutside(m_Handler3D->ActiveTissuelayer(), rx, m_Tissuenr);
					at_boundary = m_Bmphand->TissuevalueAtBoundary(m_Handler3D->ActiveTissuelayer(), m_Tissuenr);
				}
			}
		}
		else
		{
			if (data_selection.allSlices)
			{
				if (mm_unit)
				{
					// \warning creates block shaped kernel, instead of ellipsoid
					if (data_selection.work)
					{
						float setto = m_Handler3D->AddSkin3D(rx, ry, rz);
						at_boundary = m_Handler3D->ValueAtBoundary3D(setto);
					}
					else
					{
						m_Handler3D->AddSkintissue3D(rx, ry, rz, m_Tissuenr);
						at_boundary = m_Handler3D->TissuevalueAtBoundary3D(m_Tissuenr);
					}
				}
				else
				{
					if (data_selection.work)
					{
						float setto = m_Handler3D->AddSkin3D(rx);
						at_boundary = m_Handler3D->ValueAtBoundary3D(setto);
					}
					else
					{
						m_Handler3D->AddSkintissue3D(rx, ry, rz, m_Tissuenr);
						at_boundary = m_Handler3D->TissuevalueAtBoundary3D(m_Tissuenr);
					}
				}
			}
			else // active slice
			{
				if (mm_unit)
				{
					// \warning if spacing is anisotropic, skin thickness will be non-uniform
					if (data_selection.work)
					{
						float setto = m_Bmphand->AddSkin(rx);
						at_boundary = m_Bmphand->ValueAtBoundary(setto);
					}
					else
					{
						m_Bmphand->AddSkintissue(m_Handler3D->ActiveTissuelayer(), rx, m_Tissuenr);
						at_boundary = m_Bmphand->TissuevalueAtBoundary(m_Handler3D->ActiveTissuelayer(), m_Tissuenr);
					}
				}
				else
				{
					if (data_selection.work)
					{
						float setto = m_Bmphand->AddSkin(rx);
						at_boundary = m_Bmphand->ValueAtBoundary(setto);
					}
					else
					{
						m_Bmphand->AddSkintissue(m_Handler3D->ActiveTissuelayer(), rx, m_Tissuenr);
						at_boundary = m_Bmphand->TissuevalueAtBoundary(m_Handler3D->ActiveTissuelayer(), m_Tissuenr);
					}
				}
			}
		}
		if (at_boundary)
		{
			QMessageBox::information(this, "iSeg", "Information:\nThe skin partially touches "
																						 "or\nintersects with the boundary.");
		}
	}
	else if (m_Fillskin->isChecked())
	{
		float mm_rad = m_FillSkinParams->m_Thickness->text().toFloat();
		bool mm_unit = m_FillSkinParams->m_UnitMm->isChecked();
		int const x_thick = mm_unit ? static_cast<int>(mm_rad / m_Spacing[0] + 0.1f) : mm_rad;
		int const y_thick = mm_unit ? static_cast<int>(mm_rad / m_Spacing[1] + 0.1f) : mm_rad;
		int const z_thick = mm_unit ? static_cast<int>(mm_rad / m_Spacing[2] + 0.1f) : mm_rad;

		auto selected_background_id = m_FillSkinParams->m_BackgroundValue->text().toInt();
		auto selected_skin_id = m_FillSkinParams->m_SkinValue->text().toInt();
		if (m_FillSkinParams->m_AllSlices->isChecked())
			m_Handler3D->FillSkin3d(x_thick, y_thick, z_thick, selected_background_id, selected_skin_id);
		else
			m_Bmphand->FillSkin(x_thick, y_thick, selected_background_id, selected_skin_id);
	}
	else if (m_Allfill->isChecked())
	{
		data_selection.allSlices = m_FillAllParams->m_AllSlices->isChecked();
		data_selection.work = m_FillAllParams->m_Target->isChecked();
		data_selection.tissues = !data_selection.work;
		emit BeginDatachange(data_selection, this);

		if (data_selection.allSlices)
		{
			if (data_selection.work)
				m_Handler3D->FillUnassigned();
			else
				m_Handler3D->FillUnassignedtissue(m_Tissuenr);
		}
		else
		{
			if (data_selection.work)
				m_Bmphand->FillUnassigned();
			else
				m_Bmphand->FillUnassignedtissue(m_Handler3D->ActiveTissuelayer(), m_Tissuenr);
		}
	}

	emit EndDatachange(this);
}

void OutlineCorrectionWidget::SelectobjPushed()
{
	m_Selectobj = true;
}

void OutlineCorrectionWidget::CopyPickPushed()
{
	m_Selectobj = true;
	m_CopyMode = true;
	m_BrushParams->m_CopyPickGuide->setChecked(true);
}

void OutlineCorrectionWidget::WorkbitsChanged()
{
	float const f = GetObjectValue();

	m_Bmphand = m_Handler3D->GetActivebmphandler();
	float* workbits = m_Bmphand->ReturnWork();
	unsigned area = unsigned(m_Bmphand->ReturnWidth()) * m_Bmphand->ReturnHeight();
	unsigned i = 0;
	while (i < area && workbits[i] != f)
		i++;
	if (i == area)
	{
		Pair p;
		m_Bmphand->GetRange(&p);
		if (p.high > p.low)
		{
			m_CurrentParams->SetObjectValue(p.high);
		}
	}
}

void OutlineCorrectionWidget::OnSlicenrChanged()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		BmphandChanged(m_Handler3D->GetActivebmphandler());
	}
	else
	{
		WorkbitsChanged();
	}
	DrawGuide();
}

void OutlineCorrectionWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
	WorkbitsChanged();
}

void OutlineCorrectionWidget::WorkChanged()
{
	WorkbitsChanged();
}

void OutlineCorrectionWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
	m_Spacing = m_Handler3D->Spacing();
}

void OutlineCorrectionWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_Spacing = m_Handler3D->Spacing();
	DrawGuide();
}

void OutlineCorrectionWidget::OnTissuenrChanged(int tissuenr1)
{
	m_Tissuenr = (tissues_size_t)(tissuenr1 + 1);
	DrawGuide();
}

void OutlineCorrectionWidget::Cleanup()
{
	m_Vpdyn.clear();
	emit VpdynChanged(&m_Vpdyn);
	std::vector<Mark> vm;
	emit VmChanged(&vm);
}

FILE* OutlineCorrectionWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = static_cast<int>(m_BrushParams->m_Radius->text().toFloat());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_FillHolesParams->m_ObjectSize->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_FillGapsParams->m_ObjectSize->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Brush->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Olcorr->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Holefill->isChecked() || m_Removeislands->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Gapfill->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Allfill->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Addskin->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Fillskin->isChecked());
		//fwrite(&(dummy), 1,sizeof(int), fp);
		dummy = (m_CurrentParams->Work() ? 0 : 1);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (m_CurrentParams->Work() ? 1 : 0);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_BrushParams->m_Erase->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_BrushParams->m_Draw->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_BrushParams->m_Modify->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_SmoothTissues->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_AddSkinParams->m_Inside->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_AddSkinParams->m_Outside->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_FillHolesParams->m_AllSlices->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* OutlineCorrectionWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_Methods, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_BrushParams->m_Radius->setText(QString::number(dummy));
		fread(&dummy, sizeof(int), 1, fp);
		m_FillHolesParams->m_ObjectSize->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_FillGapsParams->m_ObjectSize->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Brush->setChecked(dummy != 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Olcorr->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Holefill->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Gapfill->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Allfill->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Addskin->setChecked(dummy > 0);

		//fread(&dummy,sizeof(int), 1, fp);
		m_Fillskin->setChecked(false);

		fread(&dummy, sizeof(int), 1, fp);
		// tissue & work is exclusive;
		fread(&dummy, sizeof(int), 1, fp);
		m_CurrentParams->SetWork(dummy != 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_BrushParams->m_Erase->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_BrushParams->m_Draw->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_BrushParams->m_Modify->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_SmoothTissues->setChecked(dummy > 0); //BL used to be 'adapt'
		fread(&dummy, sizeof(int), 1, fp);
		m_AddSkinParams->m_Inside->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_AddSkinParams->m_Outside->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_FillHolesParams->m_AllSlices->setChecked(dummy > 0);

		MethodChanged();

		QObject_connect(m_Methods, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));
	}
	return fp;
}

void OutlineCorrectionWidget::HideParamsChanged() { MethodChanged(); }

float OutlineCorrectionWidget::GetObjectValue() const
{
	if (m_CurrentParams)
	{
		return m_CurrentParams->ObjectValue();
	}
	return 0.f;
}

void OutlineCorrectionWidget::CarvePushed()
{
	ISEG_INFO_MSG("Spherize ...");

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	using label_image_type = itk::SliceContiguousImage<unsigned short>;
	using target_image_type = itk::SliceContiguousImage<float>;
	using image_type = itk::Image<unsigned char, 3>;

	SlicesHandlerITKInterface wrapper(m_Handler3D);
	auto tissues = wrapper.GetTissues(true);
	auto target = wrapper.GetTarget(true);

	const unsigned char fg = 255;

	ProgressDialog progress("Spherize ...", this);
	auto observer = iseg::ItkProgressObserver::New();
	observer->SetProgressInfo(&progress);

	image_type::Pointer input;
	if (m_SpherizeParams->Work())
	{
		auto threshold = itk::BinaryThresholdImageFilter<target_image_type, image_type>::New();
		threshold->SetInput(target);
		threshold->SetLowerThreshold(m_SpherizeParams->ObjectValue());
		threshold->SetUpperThreshold(m_SpherizeParams->ObjectValue());
		threshold->SetInsideValue(fg);
		threshold->SetOutsideValue(0);
		threshold->Update();
		input = threshold->GetOutput();
	}
	else if (m_Handler3D->TissueSelection().size() == 1)
	{
		auto selected_tissue = m_Handler3D->TissueSelection().front();

		auto threshold = itk::BinaryThresholdImageFilter<label_image_type, image_type>::New();
		threshold->SetInput(tissues);
		threshold->SetLowerThreshold(selected_tissue);
		threshold->SetUpperThreshold(selected_tissue);
		threshold->SetInsideValue(fg);
		threshold->SetOutsideValue(0);
		threshold->Update();
		input = threshold->GetOutput();
	}
	ISEG_INFO_MSG("Casted/threshold done");

	// crop to reduce computation time
	auto input_region = input->GetBufferedRegion();
	auto working_region = itk::GetLabelRegion<image_type>(input, fg);
	working_region.PadByRadius(1);
	working_region.Crop(input_region);

	auto crop = itk::ExtractImageFilter<image_type, image_type>::New();
	crop->SetInput(input);
	crop->SetExtractionRegion(working_region);
	SAFE_UPDATE(crop, return);
	auto input_cropped = crop->GetOutput();
	ISEG_INFO_MSG("Cropping done");

	image_type::Pointer output;
	if (m_SpherizeParams->m_CarveInside->isChecked())
	{
		auto carve = itk::FixTopologyCarveInside<image_type, image_type>::New();
		carve->SetInput(input_cropped);
		carve->SetInsideValue(fg);
		carve->SetOutsideValue(0);
		carve->AddObserver(itk::ProgressEvent(), observer);
		SAFE_UPDATE(carve, return );
		output = carve->GetOutput();
	}
	else
	{
		auto carve = itk::FixTopologyCarveOutside<image_type, image_type>::New();
		carve->SetInput(input_cropped);
		carve->SetInsideValue(fg);
		carve->SetRadius(m_SpherizeParams->m_Radius->text().toFloat());
		carve->AddObserver(itk::ProgressEvent(), observer);
		SAFE_UPDATE(carve, return );
		output = carve->GetOutput();
	}
	ISEG_INFO_MSG("Skeletonization done");

	if (output)
	{
		iseg::Paste<image_type, target_image_type>(output, target, working_region);
		m_Handler3D->SetModeall(eScaleMode::kFixedRange, false);
	}

	emit EndDatachange(this);
}

// \note this is maybe a model of how execute callbacks could be implemented in the future, instead of all via code in Bmphandler/slicehandler...
void OutlineCorrectionWidget::SmoothTissuesPushed()
{
	size_t start_slice = m_Handler3D->StartSlice();
	size_t end_slice = m_Handler3D->EndSlice();

	if (m_SmoothTissuesParams->m_ActiveSlice->isChecked())
	{
		start_slice = m_Handler3D->ActiveSlice();
		end_slice = start_slice + 1;
	}

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.allSlices = !m_SmoothTissuesParams->m_ActiveSlice->isChecked();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	ProgressDialog progress("Smooth tissues", this);
	int const split_limit = m_SmoothTissuesParams->m_SplitLimit->text().toInt();
	if (end_slice > start_slice + split_limit &&
			m_SmoothTissuesParams->m_3D->isChecked())
	{
		for (size_t i = start_slice; i < end_slice; i += split_limit)
		{
			SmoothTissues(m_Handler3D, i, std::min(i + split_limit, end_slice), m_SmoothTissuesParams->m_Sigma->text().toDouble(), m_SmoothTissuesParams->m_3D->isChecked(), &progress);
		}
	}
	else
	{
		SmoothTissues(m_Handler3D, start_slice, end_slice, m_SmoothTissuesParams->m_Sigma->text().toDouble(), m_SmoothTissuesParams->m_3D->isChecked(), &progress);
	}

	emit EndDatachange(this);
}

} // namespace iseg
