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

#include "InterpolationWidget.h"
#include "SlicesHandler.h"

#include "Interface/PropertyWidget.h"

#include "Data/BrushInteraction.h"

#include <QHBoxLayout>

#include <algorithm>
#include <array>

namespace iseg {

InterpolationWidget::InterpolationWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Interpolate/extrapolate between segmented slices."));

	m_Nrslices = m_Handler3D->NumSlices();

	auto group = PropertyGroup::Create("Settings");

	m_Modegroup = group->Add("Mode", PropertyEnum::Create({"Interpolation", "Extrapolation", "Batch Interp."}, 0));
	m_Modegroup->SetDescription("Mode");
	m_Modegroup->SetToolTip("Select the interpolation mode.");

	m_Sourcegroup = group->Add("Input", PropertyEnum::Create({"Selected Tissue", "All Tissues", "Target"}, 0));
	m_Sourcegroup->SetDescription("Input Image");
	m_Sourcegroup->SetToolTip(
			"Selected Tissue: Interpolate currently selected tissue."
			"<br>"
			"All Tissues: Interpolate all tissue at once (not available for extrapolation)."
			"<br>"
			"Target: Interpolat Target image");

	auto extrapol_group = PropertyGroup::Create("Extrapolation Settings");

	m_SbSlicenr = group->Add("TargetSlice", PropertyInt::Create(1, 1, m_Nrslices));
	m_SbSlicenr->SetDescription("Target Slice");
	m_SbSlicenr->SetToolTip(
			"Set the slice index where the Tissue/Target "
			"distribution will be extrapolated.");

	auto batch_group = PropertyGroup::Create("Batch Settings");

	m_SbBatchstride = group->Add("Stride", PropertyInt::Create(2, 2, m_Nrslices - 1));
	m_SbBatchstride->SetDescription("Stride");
	m_SbBatchstride->SetToolTip(
			"The stride is the distance between segmented slices. For example, "
			"you may segment every fifth slice (stride=5), then interpolate in "
			"between.");

	m_CbConnectedshapebased = group->Add("ShapeBased", PropertyBool::Create(false));
	m_CbConnectedshapebased->SetDescription("Connected Shape-Based");
	m_CbConnectedshapebased->SetToolTip(
			"Align corresponding foreground objects by their center of mass, "
			"to ensure shape-based interpolation connects these objects");

	m_CbMedianset = group->Add("MedianSet", PropertyBool::Create(false));
	m_CbMedianset->SetDescription("Median Set");
	m_CbMedianset->SetToolTip(
			"If Median Set is ON, the algorithm described in [1] "
			"is employed. Otherwise shape-based interpolation [2] is used. "
			"Multiple objects (with different gray values or tissue assignments, "
			"respectively) can be interpolated jointly without introducing gaps or overlap."
			"<br>"
			"[1] S. Beucher. Sets, partitions and functions interpolations. 1998.<br>"
			"[2] S. P. Raya and J. K. Udupa. Shape-based interpolation of multidimensional objects. 1990.");

	m_Connectivitygroup = group->Add("Connectivity", PropertyEnum::Create({"4-connectivity", "8-connectivity"}, 1));

	m_CbBrush = group->Add("EnableBrush", PropertyBool::Create(false));
	m_CbBrush->SetDescription("Enable Brush");

	m_BrushRadius = group->Add("BrushRadius", PropertyReal::Create(1.0, 0.0));
	m_BrushRadius->SetToolTip("Set the radius of the brush in physical units, i.e. typically mm.");

	// Execute
	auto push_start = group->Add("StartSlice", PropertyButton::Create("Start Slice", [this]() { StartslicePressed(); }));
	push_start->SetToolTip(
			"Interpolation/extrapolation is based on 2 slices. Click start to "
			"select the first slice and Execute to select the second slice. Interpolation "
			"automatically interpolates the intermediate slices."
			"<br>"
			"Note:<br>The result is displayed in the Target but is not directly "
			"added to the tissue distribution. "
			"The user can add it with Adder function. The 'All Tissues' option "
			"adds the result directly to the tissue.");

	m_Pushexec = group->Add("Execute", PropertyButton::Create("Execute", [this]() { Execute(); }));
	m_Pushexec->SetEnabled(false);

	// create signal-slot connections
	m_CbBrush->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			MethodChanged();
	});
	m_BrushRadius->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			BrushChanged();
	});

	m_Modegroup->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			BrushChanged();
	});

	m_SourceConnection = m_Sourcegroup->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			SourceChanged();
	});

	m_CbConnectedshapebased->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
		{
			if (m_CbConnectedshapebased->Value())
				m_CbMedianset->SetValue(false);
			MethodChanged();
		}
	});
	m_CbMedianset->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			MethodChanged();
	});
	m_ModeConnection = m_Modegroup->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			MethodChanged();
	});

	// add widget and layout
	auto property_view = new PropertyWidget(group);
	//property_view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view, 2);
	layout->addStretch(1);
	setLayout(layout);

	MethodChanged();
	SourceChanged();
}

InterpolationWidget::~InterpolationWidget() {}

void InterpolationWidget::Init()
{
	if (m_Handler3D->NumSlices() != m_Nrslices)
	{
		m_Nrslices = m_Handler3D->NumSlices();
		m_SbSlicenr->SetMaximum((int)m_Nrslices);
		m_SbBatchstride->SetMaximum((int)m_Nrslices - 1);
		m_Pushexec->SetEnabled(false);
	}

	if (!m_Brush)
	{
		m_Brush = new BrushInteraction(
				m_Handler3D,
				[this](DataSelection sel) { emit BeginDatachange(sel, this); },
				[this](iseg::eEndUndoAction a) { emit EndDatachange(this, a); },
				[this](std::vector<Point>* vp) { emit VpdynChanged(vp); });
		BrushChanged();
	}
	else
	{
		m_Brush->Init(m_Handler3D);
	}
}

void InterpolationWidget::NewLoaded() {}

void InterpolationWidget::OnSlicenrChanged() {}

void InterpolationWidget::OnTissuenrChanged(int tissuetype)
{
	m_Tissuenr = (tissues_size_t)tissuetype + 1;
}

void InterpolationWidget::Handler3DChanged()
{
	if (m_Handler3D->NumSlices() != m_Nrslices)
	{
		m_Nrslices = m_Handler3D->NumSlices();
		m_SbSlicenr->SetMaximum((int)m_Nrslices);
		m_SbBatchstride->SetMaximum((int)m_Nrslices - 1);
	}
	m_Pushexec->SetEnabled(false);
}

void InterpolationWidget::StartslicePressed()
{
	m_Startnr = m_Handler3D->ActiveSlice();
	m_Pushexec->SetEnabled(true);
}

void InterpolationWidget::OnMouseClicked(Point p)
{
	if (!m_CbBrush->Value())
	{
		WidgetInterface::OnMouseClicked(p);
	}
	else if (m_Sourcegroup->Value() != eSourceType::kTissueAll)
	{
		m_Brush->SetTissueValue(m_Tissuenr);
		m_Brush->OnMouseClicked(p);
	}
}

void InterpolationWidget::OnMouseReleased(Point p)
{
	if (!m_CbBrush->Value())
	{
		WidgetInterface::OnMouseReleased(p);
	}
	else if (m_Sourcegroup->Value() != eSourceType::kTissueAll)
	{
		m_Brush->OnMouseReleased(p);
	}
}

void InterpolationWidget::OnMouseMoved(Point p)
{
	if (!m_CbBrush->Value())
	{
		WidgetInterface::OnMouseMoved(p);
	}
	else if (m_Sourcegroup->Value() != eSourceType::kTissueAll)
	{
		m_Brush->OnMouseMoved(p);
	}
}

void InterpolationWidget::Execute()
{
	const auto batchstride = static_cast<unsigned short>(m_SbBatchstride->Value());
	const bool connected = m_CbConnectedshapebased->Visible() && m_CbConnectedshapebased->Value();
	const bool chess_connectivity = (m_Connectivitygroup->Value() == eConnectivityType::k8Connectivity);

	unsigned short current = m_Handler3D->ActiveSlice();
	if (current != m_Startnr)
	{
		DataSelection data_selection;
		if (m_Modegroup->Value() == eModeType::kExtrapolation)
		{
			data_selection.sliceNr = (unsigned short)m_SbSlicenr->Value() - 1;
			data_selection.work = true;
			emit BeginDatachange(data_selection, this);
			if (m_Sourcegroup->Value()==eSourceType::kWork)
			{
				m_Handler3D->Extrapolatework(m_Startnr, current, (unsigned short)m_SbSlicenr->Value() - 1);
			}
			else
			{
				m_Handler3D->Extrapolatetissue(m_Startnr, current, (unsigned short)m_SbSlicenr->Value() - 1, m_Tissuenr);
			}
			emit EndDatachange(this);
		}
		else if (m_Modegroup->Value() == eModeType::kInterpolation)
		{
			data_selection.allSlices = true;

			if (m_Sourcegroup->Value()==eSourceType::kWork)
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);
				if (m_CbMedianset->Value())
				{
					m_Handler3D->InterpolateworkgreyMedianset(m_Startnr, current, chess_connectivity);
				}
				else
				{
					m_Handler3D->Interpolateworkgrey(m_Startnr, current, connected);
				}
			}
			else if (m_Sourcegroup->Value()==eSourceType::kTissue)
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);
				if (m_CbMedianset->Value())
				{
					m_Handler3D->InterpolatetissueMedianset(m_Startnr, current, m_Tissuenr, chess_connectivity);
				}
				else
				{
					m_Handler3D->Interpolatetissue(m_Startnr, current, m_Tissuenr, connected);
				}
			}
			else if (m_Sourcegroup->Value() == eSourceType::kTissueAll)
			{
				data_selection.tissues = true;
				emit BeginDatachange(data_selection, this);
				if (m_CbMedianset->Value())
				{
					m_Handler3D->InterpolatetissuegreyMedianset(m_Startnr, current, chess_connectivity);
				}
				else
				{
					m_Handler3D->Interpolatetissuegrey(m_Startnr, current);
				}
			}
			emit EndDatachange(this);
		}
		else if (m_Modegroup->Value() == eModeType::kBatchInterpolation)
		{
			data_selection.allSlices = true;

			if (m_Sourcegroup->Value()==eSourceType::kWork)
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				if (m_CbMedianset->Value())
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr; batchstart <= current - batchstride; batchstart += batchstride)
					{
						m_Handler3D->InterpolateworkgreyMedianset(batchstart, batchstart + batchstride, chess_connectivity);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->InterpolateworkgreyMedianset(batchstart - batchstride, current, chess_connectivity);
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr; batchstart <= current - batchstride; batchstart += batchstride)
					{
						m_Handler3D->Interpolateworkgrey(batchstart, batchstart + batchstride, connected);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->Interpolateworkgrey(batchstart - batchstride, current, connected);
					}
				}
			}
			else if (m_Sourcegroup->Value()==eSourceType::kTissue)
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				if (m_CbMedianset->Value())
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr; batchstart <= current - batchstride; batchstart += batchstride)
					{
						m_Handler3D->InterpolatetissueMedianset(batchstart, batchstart + batchstride, m_Tissuenr, chess_connectivity);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->InterpolatetissueMedianset(batchstart - batchstride, current, m_Tissuenr, chess_connectivity);
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr; batchstart <= current - batchstride; batchstart += batchstride)
					{
						m_Handler3D->Interpolatetissue(batchstart, batchstart + batchstride, m_Tissuenr, connected);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->Interpolatetissue(batchstart - batchstride, current, m_Tissuenr, connected);
					}
				}
			}
			else if (m_Sourcegroup->Value() == eSourceType::kTissueAll)
			{
				data_selection.tissues = true;
				emit BeginDatachange(data_selection, this);

				if (m_CbMedianset->Value())
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr; batchstart <= current - batchstride; batchstart += batchstride)
					{
						m_Handler3D->InterpolatetissuegreyMedianset(batchstart, batchstart + batchstride, chess_connectivity);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->InterpolatetissuegreyMedianset(batchstart - batchstride, current, chess_connectivity);
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr; batchstart <= current - batchstride; batchstart += batchstride)
					{
						m_Handler3D->Interpolatetissuegrey(batchstart, batchstart + batchstride);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->Interpolatetissuegrey(batchstart - batchstride, current);
					}
				}
			}
			emit EndDatachange(this);
		}
		m_Pushexec->SetEnabled(false);
	}
}

void InterpolationWidget::BrushChanged()
{
	if (m_Brush)
	{
		m_Brush->SetBrushTarget(m_Sourcegroup->Value()==eSourceType::kWork);
		m_Brush->SetRadius(m_BrushRadius->Value());
	}
}

void InterpolationWidget::MethodChanged()
{
	m_SbSlicenr->SetVisible(m_Modegroup->Value() == eModeType::kExtrapolation);
	m_SbBatchstride->SetVisible(m_Modegroup->Value() == eModeType::kBatchInterpolation);

	m_CbConnectedshapebased->SetVisible(m_Modegroup->Value() == eModeType::kInterpolation);

	m_CbMedianset->SetVisible(m_Modegroup->Value() != eModeType::kExtrapolation);
	m_Connectivitygroup->SetVisible(m_CbMedianset->Value() && m_Modegroup->Value() != eModeType::kExtrapolation);

	m_CbBrush->SetEnabled(m_Modegroup->Value() == eModeType::kInterpolation);
	m_BrushRadius->SetEnabled(m_CbBrush->Value() && m_Modegroup->Value() == eModeType::kInterpolation);

	if (m_Modegroup->Value() == eModeType::kExtrapolation && m_Sourcegroup->Value() == eSourceType::kTissueAll)
	{
		m_Sourcegroup->SetValue(eSourceType::kTissue);
	}
	m_Sourcegroup->SetEnabled(eSourceType::kTissueAll, m_Modegroup->Value() != eModeType::kExtrapolation);
}

void InterpolationWidget::SourceChanged()
{
	// MethodChanged(); // TODO, is everything updated as expected?

	if (m_Brush)
	{
		m_Brush->SetBrushTarget(m_Sourcegroup->Value()==eSourceType::kWork);
	}
}

FILE* InterpolationWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SbSlicenr->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Sourcegroup->Value()==eSourceType::kTissue);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Sourcegroup->Value()==eSourceType::kWork);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == eModeType::kInterpolation);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == eModeType::kExtrapolation);
		fwrite(&(dummy), 1, sizeof(int), fp);
		if (version >= 11)
		{
			dummy = (int)(m_Sourcegroup->Value() == eSourceType::kTissueAll);
			fwrite(&(dummy), 1, sizeof(int), fp);
			dummy = (int)(m_Modegroup->Value() == eModeType::kBatchInterpolation);
			fwrite(&(dummy), 1, sizeof(int), fp);
			if (version >= 12)
			{
				dummy = (int)(m_CbMedianset->Value());
				fwrite(&(dummy), 1, sizeof(int), fp);
				dummy = (int)(m_Connectivitygroup->Value() == eConnectivityType::k4Connectivity);
				fwrite(&(dummy), 1, sizeof(int), fp);
				dummy = (int)(m_Connectivitygroup->Value() == eConnectivityType::k8Connectivity);
				fwrite(&(dummy), 1, sizeof(int), fp);
			}
		}
	}

	return fp;
}

FILE* InterpolationWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		std::array<int, 3> source;
		std::array<int, 3> mode;
		int slicenr;
		int medianset;
		int cityblock, chessboard;

		fread(&slicenr, sizeof(int), 1, fp);
		fread(&source[kTissue], sizeof(int), 1, fp);
		fread(&source[kWork], sizeof(int), 1, fp);
		fread(&mode[kInterpolation], sizeof(int), 1, fp);
		fread(&mode[kExtrapolation], sizeof(int), 1, fp);
		if (version >= 11)
		{
			fread(&source[kTissueAll], sizeof(int), 1, fp);
			fread(&mode[kBatchInterpolation], sizeof(int), 1, fp);
			if (version >= 12)
			{
				fread(&medianset, sizeof(int), 1, fp);
				m_CbMedianset->SetValue(medianset > 0);
				fread(&cityblock, sizeof(int), 1, fp);
				fread(&chessboard, sizeof(int), 1, fp);
			}
		}

		{
			boost::signals2::shared_connection_block mode_block(m_ModeConnection);
			boost::signals2::shared_connection_block source_block(m_SourceConnection);
			m_SbSlicenr->SetValue(slicenr);
			m_Sourcegroup->SetValue(source[kWork] ? kWork : (source[kTissue] ? kTissue : kTissueAll));
			m_Modegroup->SetValue(mode[kInterpolation] ? kInterpolation : (mode[kExtrapolation] ? kExtrapolation : kBatchInterpolation));
			m_Connectivitygroup->SetValue(cityblock ? k4Connectivity : k8Connectivity);
		}

		MethodChanged();
		SourceChanged();
	}
	return fp;
}

} // namespace iseg
