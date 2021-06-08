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
#include "TransformWidget.h"

#include "Interface/PropertyWidget.h"

#include <QHBoxLayout>

namespace iseg {

TransformWidget::TransformWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Shift/rotate/scale the Source, Target or Tissue."));

	m_SliceTransform = new SliceTransform(hand3D);
	m_SliceTransform->SelectTransformData(true, true, true);

	// options
	auto group = PropertyGroup::Create("Settings");

	m_TransformType = group->Add("Transform", PropertyEnum::Create({"Translate", "Rotate", "Scale", "Shear", "Flip"}, 0));
	m_TransformType->SetToolTip("Select the transform to apply.");

	m_TransformSourceCheckBox = group->Add("Source", PropertyBool::Create(true));
	m_TransformTargetCheckBox = group->Add("Target", PropertyBool::Create(true));
	m_TransformTissuesCheckBox = group->Add("Tissues", PropertyBool::Create(true));
	m_AllSlicesCheckBox = group->Add("Apply to all slices", PropertyBool::Create(false));

	// translate
	m_TranslateX = group->Add("Translate X", PropertyInt::Create(0));
	m_TranslateY = group->Add("Translate Y", PropertyInt::Create(0));

	// scale
	m_ScaleX = group->Add("Scale X", PropertyReal::Create(1));
	m_ScaleY = group->Add("Scale Y", PropertyReal::Create(1));

	// rotate
	m_RotationAngle = group->Add("Angle", PropertyReal::Create(0));

	// shear
	m_ShearingAngle = group->Add("Shear Angle", PropertyReal::Create(0));

	// flip
	m_FlipCheckBox = group->Add("Flip", PropertyBool::Create(false));

	// axis for
	m_AxisSelection = group->Add("Axis", PropertyEnum::Create({"X", "Y"}, 0));

	// Center
	m_CenterSelectPushButton = group->Add("Select Center", PropertyButton::Create([]() {}));
	m_CenterSelectPushButton->SetCheckable(true);
	m_CenterSelectPushButton->SetChecked(false);
	m_CenterCoordsLabel = group->Add("Center", PropertyString::Create("Center: (0000, 0000)"));
	m_CenterCoordsLabel->SetEnabled(false);
	SetCenterDefault();

	m_ExecutePushButton = group->Add("Execute", PropertyButton::Create([this]() { Execute(); }));
	m_CancelPushButton = group->Add("Cancel", PropertyButton::Create([this]() { CancelTransform(); }));

	// initialize
	TransformChanged();

	// add widget and layout
	auto property_view = new PropertyWidget(group);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view, 2);
	layout->addStretch(1);
	setLayout(layout);

	// connections
	m_TransformType->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			TransformChanged();
	});

	m_TransformSourceCheckBox->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			SelectSourceChanged();
	});
	m_TransformTargetCheckBox->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			SelectTargetChanged();
	});
	m_TransformTissuesCheckBox->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			SelectTissuesChanged();
	});

	group->onChildModified.connect([this](Property_ptr prop, Property::eChangeType type) {
		if (type == Property::kValueChanged)
		{
			if (prop == m_TranslateX || prop == m_TranslateY || prop == m_ScaleX || prop == m_ScaleY || prop == m_RotationAngle || prop == m_ShearingAngle || prop == m_AxisSelection || prop == m_FlipCheckBox)
			{
				UpdatePreview();
			}
		}
	});

	/*
	QObject_connect(m_Slider1, SIGNAL(valueChanged(int)), this, SLOT(Slider1Changed(int)));
	QObject_connect(m_Slider2, SIGNAL(valueChanged(int)), this, SLOT(Slider2Changed(int)));

	QObject_connect(m_LineEdit1, SIGNAL(editingFinished()), this, SLOT(LineEdit1Edited()));
	QObject_connect(m_LineEdit2, SIGNAL(editingFinished()), this, SLOT(LineEdit2Edited()));
	*/
}

TransformWidget::~TransformWidget() { delete m_SliceTransform; }

void TransformWidget::UpdatePreview()
{
	if (m_DisableUpdatePreview)
	{
		return;
	}

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->Value();
	data_selection.work = m_TransformTargetCheckBox->Value();
	data_selection.tissues = m_TransformTissuesCheckBox->Value();
	emit BeginDatachange(data_selection, this, false);

	if (m_TransformType->Value() == kTranslate)
	{
		// Translate
		m_SliceTransform->Translate(m_TranslateX->Value() - m_TransformParameters.m_TranslationOffset[0], m_TranslateY->Value() - m_TransformParameters.m_TranslationOffset[1]);
		m_TransformParameters.m_TranslationOffset[0] = m_TranslateX->Value();
		m_TransformParameters.m_TranslationOffset[1] = m_TranslateY->Value();
	}
	else if (m_TransformType->Value() == kRotate)
	{
		// Rotate
		m_SliceTransform->Rotate(m_RotationAngle->Value() - m_TransformParameters.m_RotationAngle, m_TransformParameters.m_TransformCenter[0], m_TransformParameters.m_TransformCenter[1]);
		m_TransformParameters.m_RotationAngle = m_RotationAngle->Value();
	}
	else if (m_TransformType->Value() == kScale)
	{
		// Scale
		m_SliceTransform->Scale(m_ScaleX->Value() / m_TransformParameters.m_ScalingFactor[0], m_ScaleY->Value() / m_TransformParameters.m_ScalingFactor[1], m_TransformParameters.m_TransformCenter[0], m_TransformParameters.m_TransformCenter[1]);
		m_TransformParameters.m_ScalingFactor[0] = m_ScaleX->Value();
		m_TransformParameters.m_ScalingFactor[1] = m_ScaleY->Value();
	}
	else if (m_TransformType->Value() == kShear)
	{
		// Shear
		if (m_AxisSelection->Value() == kX)
		{
			m_SliceTransform->Shear(m_ShearingAngle->Value() - m_TransformParameters.m_ShearingAngle, true, m_TransformParameters.m_TransformCenter[0]);
		}
		else
		{
			m_SliceTransform->Shear(m_ShearingAngle->Value() - m_TransformParameters.m_ShearingAngle, false, m_TransformParameters.m_TransformCenter[1]);
		}
		m_TransformParameters.m_ShearingAngle = m_ShearingAngle->Value();
	}
	else if (m_TransformType->Value() == kFlip)
	{
		// Update flipping
		if (m_AxisSelection->Value() == kX)
		{
			m_SliceTransform->Flip(true, m_TransformParameters.m_TransformCenter[0]);
		}
		else
		{
			m_SliceTransform->Flip(false, m_TransformParameters.m_TransformCenter[1]);
		}
	}

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::TransformChanged()
{
	// Reset the tab widgets
	ResetWidgets();

	// Show the corresponding widgets
	m_TranslateX->SetVisible(m_TransformType->Value() == kTranslate);
	m_TranslateY->SetVisible(m_TransformType->Value() == kTranslate);
	m_ScaleX->SetVisible(m_TransformType->Value() == kScale);
	m_ScaleY->SetVisible(m_TransformType->Value() == kScale);
	m_RotationAngle->SetVisible(m_TransformType->Value() == kRotate);
	m_FlipCheckBox->SetVisible(m_TransformType->Value() == kFlip);
	m_ShearingAngle->SetVisible(m_TransformType->Value() == kShear);

	m_AxisSelection->SetVisible(m_TransformType->Value() == kFlip || m_TransformType->Value() == kShear);
}

void TransformWidget::ResetWidgets()
{
	m_DisableUpdatePreview = true;

	// Reset transform parameters
	m_TransformParameters.m_TranslationOffset[0] = m_TransformParameters.m_TranslationOffset[1] = 0;
	m_TransformParameters.m_RotationAngle = 0.0;
	m_TransformParameters.m_ScalingFactor[0] = m_TransformParameters.m_ScalingFactor[1] = 1.0;
	m_TransformParameters.m_ShearingAngle = 0.0;

	// Reset properties
	m_TranslateX->SetValue(0.0);
	m_TranslateX->SetRange(-m_Handler3D->Width(), m_Handler3D->Width());
	m_TranslateY->SetValue(0.0);
	m_TranslateX->SetRange(-m_Handler3D->Height(), m_Handler3D->Height());
	m_ScaleX->SetValue(1.0);
	m_ScaleY->SetValue(1.0);
	m_RotationAngle->SetValue(0.0);
	m_ShearingAngle->SetValue(0.0);

	m_DisableUpdatePreview = false;
}

void TransformWidget::SelectSourceChanged()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = true;
	emit BeginDatachange(data_selection, this, false);

	// Set data to be transformed
	m_SliceTransform->SelectTransformData(m_TransformSourceCheckBox->Value(), m_TransformTargetCheckBox->Value(), m_TransformTissuesCheckBox->Value());

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::SelectTargetChanged()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this, false);

	// Set data to be transformed
	m_SliceTransform->SelectTransformData(m_TransformSourceCheckBox->Value(), m_TransformTargetCheckBox->Value(), m_TransformTissuesCheckBox->Value());

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::SelectTissuesChanged()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	// Set data to be transformed
	m_SliceTransform->SelectTransformData(m_TransformSourceCheckBox->Value(), m_TransformTargetCheckBox->Value(), m_TransformTissuesCheckBox->Value());

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::OnSlicenrChanged()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->Value();
	data_selection.work = m_TransformTargetCheckBox->Value();
	data_selection.tissues = m_TransformTissuesCheckBox->Value();
	emit BeginDatachange(data_selection, this, false);

	// Change active slice in transform object
	m_SliceTransform->ActiveSliceChanged();

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::Init()
{
	// Initialize transformation
	m_SliceTransform->Initialize();

	// Show tab
	HideParamsChanged();
}

void TransformWidget::Cleanup()
{
	// Cancel transform
	CancelTransform();

	// Hide tab
	HideParamsChanged();
}

void TransformWidget::NewLoaded()
{
	// Reload transform data
	m_SliceTransform->NewDataLoaded();

	// Reset the tab widgets
	ResetWidgets();

	// Reset center to default
	SetCenterDefault();
}

void TransformWidget::HideParamsChanged()
{
}

bool TransformWidget::GetIsIdentityTransform()
{
	return m_SliceTransform->GetIsIdentityTransform();
}

void TransformWidget::GetDataSelection(bool& source, bool& target, bool& tissues)
{
	source = m_TransformSourceCheckBox->Value();
	target = m_TransformTargetCheckBox->Value();
	tissues = m_TransformTissuesCheckBox->Value();
}

void TransformWidget::OnMouseClicked(Point p)
{
	if (m_CenterSelectPushButton->Checked())
	{
		ResetWidgets();
		SetCenter(p.px, p.py);
		m_CenterSelectPushButton->SetChecked(false);
	}
}

void TransformWidget::BitsChanged() { NewLoaded(); }

void TransformWidget::BmpChanged() { BitsChanged(); }

void TransformWidget::WorkChanged() { BitsChanged(); }

void TransformWidget::TissuesChanged() { BitsChanged(); }

void TransformWidget::SetCenter(int x, int y)
{
	m_TransformParameters.m_TransformCenter[0] = x;
	m_TransformParameters.m_TransformCenter[1] = y;

	// Display center coordinates
	m_CenterCoordsLabel->SetValue(QString("(%1, %2)").arg(x).arg(y).toStdString());
}

void TransformWidget::SetCenterDefault()
{
	SetCenter(m_Handler3D->Width() / 2, m_Handler3D->Height() / 2);
}

void TransformWidget::FlipPushButtonClicked()
{
	// Update transformation preview
	UpdatePreview();
}

void TransformWidget::Execute()
{
	DataSelection data_selection;
	data_selection.allSlices = m_AllSlicesCheckBox->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->Value();
	data_selection.work = m_TransformTargetCheckBox->Value();
	data_selection.tissues = m_TransformTissuesCheckBox->Value();

	// Undo start point
	m_SliceTransform->SwapDataPointers();
	emit BeginDatachange(data_selection, this);
	m_SliceTransform->SwapDataPointers();

	// Apply the transformation
	m_SliceTransform->Execute(m_AllSlicesCheckBox->Value());

	// Reset the widgets
	ResetWidgets();

	// Undo end point
	emit EndDatachange(this);
}

void TransformWidget::CancelTransform()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->Value();
	data_selection.work = m_TransformTargetCheckBox->Value();
	data_selection.tissues = m_TransformTissuesCheckBox->Value();
	emit BeginDatachange(data_selection, this, false);

	// Cancel the transformation
	m_SliceTransform->Cancel();

	// Reset the widgets
	ResetWidgets();

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

} // namespace iseg
