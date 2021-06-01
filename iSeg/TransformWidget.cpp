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

#include <q3vbox.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <qlayout.h>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <qsize.h>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

TransformWidget::TransformWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Shift/rotate/scale the Source, Target or Tissue."));

	m_SliceTransform = new SliceTransform(hand3D);
	m_SliceTransform->SelectTransformData(true, true, true);
	m_UpdateParameter1 = m_UpdateParameter2 = 0.0;
	m_DisableUpdatePreview = true;

	// Boxes
	m_HBoxOverall = new Q3HBox(this);
	m_HBoxOverall->setMargin(8);
	m_VBoxTransforms = new Q3VBox(m_HBoxOverall);
	m_VBoxParams = new Q3VBox(m_HBoxOverall);
	m_HBoxSelectData = new Q3HBox(m_VBoxParams);
	m_HBoxSlider1 = new Q3HBox(m_VBoxParams);
	m_HBoxSlider2 = new Q3HBox(m_VBoxParams);
	m_HBoxFlip = new Q3HBox(m_VBoxParams);
	m_HBoxAxisSelection = new Q3HBox(m_VBoxParams);
	m_HBoxCenter = new Q3HBox(m_VBoxParams);

	// Transformation selection radio buttons
	m_TranslateRadioButton =
			new QRadioButton(QString("Translate"), m_VBoxTransforms);
	m_RotateRadioButton = new QRadioButton(QString("Rotate"), m_VBoxTransforms);
	m_ScaleRadioButton = new QRadioButton(QString("Scale"), m_VBoxTransforms);
	m_ShearRadioButton = new QRadioButton(QString("Shear"), m_VBoxTransforms);
	m_FlipRadioButton = new QRadioButton(QString("Flip"), m_VBoxTransforms);
	//matrixRadioButton = new QRadioButton(QString("Matrix"), vBoxTransforms);
	m_TransformButtonGroup = new QButtonGroup(m_VBoxTransforms);
	m_TransformButtonGroup->insert(m_TranslateRadioButton);
	m_TransformButtonGroup->insert(m_RotateRadioButton);
	m_TransformButtonGroup->insert(m_ScaleRadioButton);
	m_TransformButtonGroup->insert(m_ShearRadioButton);
	m_TransformButtonGroup->insert(m_FlipRadioButton);
	//transformButtonGroup->insert(matrixRadioButton);
	m_TranslateRadioButton->setChecked(TRUE);

	// Data selection check boxes
	m_TransformSourceCheckBox = new QCheckBox(QString("Source"), m_HBoxSelectData);
	m_TransformTargetCheckBox = new QCheckBox(QString("Target"), m_HBoxSelectData);
	m_TransformTissuesCheckBox =
			new QCheckBox(QString("Tissues"), m_HBoxSelectData);
	m_TransformSourceCheckBox->setChecked(TRUE);
	m_TransformTargetCheckBox->setChecked(TRUE);
	m_TransformTissuesCheckBox->setChecked(TRUE);

	// Axis selection radio buttons
	m_XAxisRadioButton = new QRadioButton(QString("x axis "), m_HBoxAxisSelection);
	m_YAxisRadioButton = new QRadioButton(QString("y axis "), m_HBoxAxisSelection);
	m_AxisButtonGroup = new QButtonGroup(m_HBoxAxisSelection);
	m_AxisButtonGroup->insert(m_XAxisRadioButton);
	m_AxisButtonGroup->insert(m_YAxisRadioButton);
	m_XAxisRadioButton->setChecked(TRUE);

	// Sliders
	m_Slider1Label = new QLabel("Offset x ", m_HBoxSlider1);
	m_Slider1 = new QSlider(Qt::Horizontal, m_HBoxSlider1);
	m_LineEdit1 = new QLineEdit("-00000", m_HBoxSlider1);
	m_LineEdit1->setAlignment(Qt::AlignRight);
	m_Slider2Label = new QLabel("Offset y ", m_HBoxSlider2);
	m_Slider2 = new QSlider(Qt::Horizontal, m_HBoxSlider2);
	m_LineEdit2 = new QLineEdit("-00000", m_HBoxSlider2);
	m_LineEdit2->setAlignment(Qt::AlignRight);

	// Flip button
	m_FlipPushButton = new QPushButton("Flip", m_HBoxFlip);

	// Center
	m_CenterLabel = new QLabel("Center coordinates: ", m_HBoxCenter);
	m_CenterCoordsLabel = new QLabel("(0000, 0000)", m_HBoxCenter);
	m_CenterSelectPushButton = new QPushButton("Select", m_HBoxCenter);
	m_CenterSelectPushButton->setCheckable(true);
	SetCenterDefault();

	// Common widgets
	m_HBoxExecute = new Q3HBox(m_VBoxParams);
	m_ExecutePushButton = new QPushButton("Execute", m_HBoxExecute);
	m_AllSlicesCheckBox =
			new QCheckBox(QString("Apply to all slices"), m_HBoxExecute);
	m_AllSlicesCheckBox->setLayoutDirection(Qt::RightToLeft);
	m_CancelPushButton = new QPushButton("Cancel", m_VBoxParams);

	// Layout
	m_VBoxParams->setMargin(5);
	m_VBoxTransforms->setMargin(5);
	m_VBoxTransforms->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	m_VBoxTransforms->setLineWidth(1);

	m_Slider1->setFixedWidth(300);
	m_Slider2->setFixedWidth(300);

	m_ExecutePushButton->setFixedWidth(100);
	m_CancelPushButton->setFixedWidth(100);

	m_VBoxTransforms->setFixedSize(m_VBoxTransforms->sizeHint());
	m_VBoxParams->setFixedSize(m_VBoxParams->sizeHint());
	m_HBoxSelectData->setFixedSize(m_HBoxSelectData->sizeHint());
	m_HBoxSlider1->setFixedSize(m_HBoxSlider1->sizeHint());
	m_HBoxSlider2->setFixedSize(m_HBoxSlider2->sizeHint());
	m_HBoxFlip->setFixedSize(m_HBoxFlip->sizeHint());
	m_HBoxAxisSelection->setFixedSize(m_HBoxAxisSelection->sizeHint());
	m_HBoxCenter->setFixedSize(m_HBoxCenter->sizeHint());
	m_HBoxExecute->setFixedSize(m_HBoxExecute->sizeHint());

	m_CenterCoordsLabel->setFixedSize(m_CenterCoordsLabel->sizeHint());
	m_LineEdit1->setFixedSize(m_LineEdit1->sizeHint());
	m_LineEdit2->setFixedSize(m_LineEdit2->sizeHint());

	m_HBoxSelectData->show();
	m_HBoxSlider1->show();
	m_HBoxSlider2->show();
	m_HBoxFlip->hide();
	m_HBoxAxisSelection->hide();
	m_HBoxCenter->hide();

	// Signals
	QObject_connect(m_TransformButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(TransformChanged(int)));

	QObject_connect(m_TransformSourceCheckBox, SIGNAL(stateChanged(int)), this, SLOT(SelectSourceChanged(int)));
	QObject_connect(m_TransformTargetCheckBox, SIGNAL(stateChanged(int)), this, SLOT(SelectTargetChanged(int)));
	QObject_connect(m_TransformTissuesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(SelectTissuesChanged(int)));

	QObject_connect(m_Slider1, SIGNAL(valueChanged(int)), this, SLOT(Slider1Changed(int)));
	QObject_connect(m_Slider2, SIGNAL(valueChanged(int)), this, SLOT(Slider2Changed(int)));

	QObject_connect(m_LineEdit1, SIGNAL(editingFinished()), this, SLOT(LineEdit1Edited()));
	QObject_connect(m_LineEdit2, SIGNAL(editingFinished()), this, SLOT(LineEdit2Edited()));

	QObject_connect(m_FlipPushButton, SIGNAL(clicked()), this, SLOT(FlipPushButtonClicked()));
	QObject_connect(m_ExecutePushButton, SIGNAL(clicked()), this, SLOT(ExecutePushButtonClicked()));
	QObject_connect(m_CancelPushButton, SIGNAL(clicked()), this, SLOT(CancelPushButtonClicked()));
}

TransformWidget::~TransformWidget() { delete m_SliceTransform; }

void TransformWidget::Slider1Changed(int value)
{
	m_LineEdit1->blockSignals(true);
	m_Slider1->blockSignals(true);

	// Update parameter and label values
	if (m_TranslateRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-width, width]
		m_UpdateParameter1 = value;
		m_LineEdit1->setText(QString("%1").arg(m_UpdateParameter1, 0, 'f', 2));
	}
	else if (m_RotateRadioButton->isChecked())
	{
		// Rotate: Linear scale: y = x, x in [-180, 180]
		m_UpdateParameter1 = value;
		m_LineEdit1->setText(QString("%1").arg(m_UpdateParameter1, 0, 'f', 2));
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		m_UpdateParameter1 = std::pow(10, value / 50.0 - 1.0);
		m_UpdateParameter1 = std::floor(m_UpdateParameter1 * 100) /
												 100.0; // Round to two fractional digits
		m_LineEdit1->setText(QString("%1").arg(m_UpdateParameter1, 0, 'f', 2));
	}
	else if (m_ShearRadioButton->isChecked())
	{
		// Shear: Linear scale: y = x, x in [-45, 45]
		m_UpdateParameter1 = value;
		m_LineEdit1->setText(QString("%1").arg(m_UpdateParameter1, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	m_LineEdit1->blockSignals(false);
	m_Slider1->blockSignals(false);
}

void TransformWidget::Slider2Changed(int value)
{
	m_LineEdit2->blockSignals(true);
	m_Slider2->blockSignals(true);

	if (m_TranslateRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-height, height]
		m_UpdateParameter2 = value;
		m_LineEdit2->setText(QString("%1").arg(m_UpdateParameter2, 0, 'f', 2));
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		m_UpdateParameter2 = std::pow(10, value / 50.0 - 1.0);
		m_UpdateParameter2 = std::floor(m_UpdateParameter2 * 100) /
												 100.0; // Round to two fractional digits
		m_LineEdit2->setText(QString("%1").arg(m_UpdateParameter2, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	m_LineEdit2->blockSignals(false);
	m_Slider2->blockSignals(false);
}

void TransformWidget::LineEdit1Edited()
{
	m_LineEdit1->blockSignals(true);
	m_Slider1->blockSignals(true);

	double value = m_LineEdit1->text().toDouble();
	if (m_TranslateRadioButton->isChecked() || m_RotateRadioButton->isChecked() ||
			m_ShearRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-width, width]
		// Rotate: Linear scale: y = x, x in [-180, 180]
		// Shear: Linear scale: y = x, x in [-45, 45]
		m_UpdateParameter1 = std::max((double)m_Slider1->minimum(), std::min(value, (double)m_Slider1->maximum()));
		m_Slider1->setValue(std::floor(m_UpdateParameter1 + 0.5));
		m_LineEdit1->setText(QString("%1").arg(m_UpdateParameter1, 0, 'f', 2));
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		m_UpdateParameter1 = std::max(0.1, std::min(value, 10.0));
		m_Slider1->setValue(std::floor((std::log10(m_UpdateParameter1) + 1) * 50.0 + 0.5));
		m_LineEdit1->setText(QString("%1").arg(m_UpdateParameter1, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	m_LineEdit1->blockSignals(false);
	m_Slider1->blockSignals(false);
}

void TransformWidget::LineEdit2Edited()
{
	m_LineEdit2->blockSignals(true);
	m_Slider2->blockSignals(true);

	double value = m_LineEdit2->text().toDouble();
	if (m_TranslateRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-height, height]
		m_UpdateParameter2 = std::max((double)m_Slider2->minimum(), std::min(value, (double)m_Slider2->maximum()));
		m_Slider2->setValue(std::floor(m_UpdateParameter2 + 0.5));
		m_LineEdit2->setText(QString("%1").arg(m_UpdateParameter2, 0, 'f', 2));
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		m_UpdateParameter2 = std::max(0.1, std::min(value, 10.0));
		m_Slider2->setValue(std::floor((std::log10(m_UpdateParameter2) + 1) * 50.0 + 0.5));
		m_LineEdit2->setText(QString("%1").arg(m_UpdateParameter2, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	m_LineEdit2->blockSignals(false);
	m_Slider2->blockSignals(false);
}

void TransformWidget::UpdatePreview()
{
	if (m_DisableUpdatePreview)
	{
		return;
	}

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->isChecked();
	data_selection.work = m_TransformTargetCheckBox->isChecked();
	data_selection.tissues = m_TransformTissuesCheckBox->isChecked();
	emit BeginDatachange(data_selection, this, false);

	if (m_TranslateRadioButton->isChecked())
	{
		// Translate
		m_SliceTransform->Translate(m_UpdateParameter1 - m_TransformParameters.m_TranslationOffset[0], m_UpdateParameter2 - m_TransformParameters.m_TranslationOffset[1]);
		m_TransformParameters.m_TranslationOffset[0] = m_UpdateParameter1;
		m_TransformParameters.m_TranslationOffset[1] = m_UpdateParameter2;
	}
	else if (m_RotateRadioButton->isChecked())
	{
		// Rotate
		m_SliceTransform->Rotate(m_UpdateParameter1 -
																 m_TransformParameters.m_RotationAngle,
				m_TransformParameters.m_TransformCenter[0], m_TransformParameters.m_TransformCenter[1]);
		m_TransformParameters.m_RotationAngle = m_UpdateParameter1;
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale
		m_SliceTransform->Scale(m_UpdateParameter1 / m_TransformParameters.m_ScalingFactor[0], m_UpdateParameter2 / m_TransformParameters.m_ScalingFactor[1], m_TransformParameters.m_TransformCenter[0], m_TransformParameters.m_TransformCenter[1]);
		m_TransformParameters.m_ScalingFactor[0] = m_UpdateParameter1;
		m_TransformParameters.m_ScalingFactor[1] = m_UpdateParameter2;
	}
	else if (m_ShearRadioButton->isChecked())
	{
		// Shear
		if (m_XAxisRadioButton->isChecked())
		{
			m_SliceTransform->Shear(m_UpdateParameter1 -
																	m_TransformParameters.m_ShearingAngle,
					true, m_TransformParameters.m_TransformCenter[0]);
		}
		else
		{
			m_SliceTransform->Shear(m_UpdateParameter1 - m_TransformParameters.m_ShearingAngle, false, m_TransformParameters.m_TransformCenter[1]);
		}
		m_TransformParameters.m_ShearingAngle = m_UpdateParameter1;
	}
	else if (m_FlipRadioButton->isChecked())
	{
		// Update flipping
		if (m_XAxisRadioButton->isChecked())
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

void TransformWidget::TransformChanged(int index)
{
	// Reset the tab widgets
	ResetWidgets();

	// Show the corresponding widgets
	if (m_TranslateRadioButton->isChecked())
	{
		// Translate
		m_Slider1Label->setText("Offset x ");
		m_Slider2Label->setText("Offset y ");
		m_HBoxSlider1->show();
		m_HBoxSlider2->show();
		m_HBoxFlip->hide();
		m_HBoxAxisSelection->hide();
		m_HBoxCenter->hide();
	}
	else if (m_RotateRadioButton->isChecked())
	{
		// Rotate
		m_Slider1Label->setText("Angle ");
		m_HBoxSlider1->show();
		m_HBoxSlider2->hide();
		m_HBoxFlip->hide();
		m_HBoxAxisSelection->hide();
		m_HBoxCenter->show();
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale
		m_Slider1Label->setText("Factor x ");
		m_Slider2Label->setText("Factor y ");
		m_HBoxSlider1->show();
		m_HBoxSlider2->show();
		m_HBoxFlip->hide();
		m_HBoxAxisSelection->hide();
		m_HBoxCenter->show();
	}
	else if (m_ShearRadioButton->isChecked())
	{
		// Shear
		m_Slider1Label->setText("Angle ");
		m_HBoxSlider1->show();
		m_HBoxSlider2->hide();
		m_HBoxFlip->hide();
		m_HBoxAxisSelection->show();
		m_HBoxCenter->show();
		m_XAxisRadioButton->setChecked(true);
	}
	else if (m_FlipRadioButton->isChecked())
	{
		// Flip
		m_HBoxSlider1->hide();
		m_HBoxSlider2->hide();
		m_HBoxFlip->show();
		m_HBoxAxisSelection->show();
		m_HBoxCenter->show();
		m_XAxisRadioButton->setChecked(true);
	} /*else if (matrixRadioButton->isChecked()) {
		// Matrix
		hBoxSlider1->hide();
		hBoxSlider2->hide();
		hBoxFlip->hide();
		hBoxAxisSelection->hide();
		hBoxCenter->hide();
	}*/
}

void TransformWidget::SelectSourceChanged(int state)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = true;
	emit BeginDatachange(data_selection, this, false);

	// Set data to be transformed
	m_SliceTransform->SelectTransformData(m_TransformSourceCheckBox->isChecked(), m_TransformTargetCheckBox->isChecked(), m_TransformTissuesCheckBox->isChecked());

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::SelectTargetChanged(int state)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this, false);

	// Set data to be transformed
	m_SliceTransform->SelectTransformData(m_TransformSourceCheckBox->isChecked(), m_TransformTargetCheckBox->isChecked(), m_TransformTissuesCheckBox->isChecked());

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

void TransformWidget::SelectTissuesChanged(int state)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	// Set data to be transformed
	m_SliceTransform->SelectTransformData(m_TransformSourceCheckBox->isChecked(), m_TransformTargetCheckBox->isChecked(), m_TransformTissuesCheckBox->isChecked());

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

QSize TransformWidget::sizeHint() const { return m_HBoxOverall->sizeHint(); }

void TransformWidget::OnSlicenrChanged()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->isChecked();
	data_selection.work = m_TransformTargetCheckBox->isChecked();
	data_selection.tissues = m_TransformTissuesCheckBox->isChecked();
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
	CancelPushButtonClicked();

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
	if (hideparams)
	{
		m_VBoxParams->hide();
	}
	else
	{
		ResetWidgets();
		SetCenterDefault();
		m_VBoxParams->show();
	}
}

bool TransformWidget::GetIsIdentityTransform()
{
	return m_SliceTransform->GetIsIdentityTransform();
}

void TransformWidget::GetDataSelection(bool& source, bool& target, bool& tissues)
{
	source = m_TransformSourceCheckBox->isChecked();
	target = m_TransformTargetCheckBox->isChecked();
	tissues = m_TransformTissuesCheckBox->isChecked();
}

void TransformWidget::OnMouseClicked(Point p)
{
	if (m_CenterSelectPushButton->isChecked())
	{
		ResetWidgets();
		SetCenter(p.px, p.py);
		m_CenterSelectPushButton->setChecked(false);
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
	m_CenterCoordsLabel->setText(QString("(%1, %2)").arg(x).arg(y));
}

void TransformWidget::SetCenterDefault()
{
	SetCenter(m_Handler3D->Width() / 2, m_Handler3D->Height() / 2);
}

void TransformWidget::ResetWidgets()
{
	m_DisableUpdatePreview = true;

	// Reset transform parameters
	m_TransformParameters.m_TranslationOffset[0] =
			m_TransformParameters.m_TranslationOffset[1] = 0;
	m_TransformParameters.m_RotationAngle = 0.0;
	m_TransformParameters.m_ScalingFactor[0] =
			m_TransformParameters.m_ScalingFactor[1] = 1.0;
	m_TransformParameters.m_ShearingAngle = 0.0;

	// Reset sliders
	if (m_TranslateRadioButton->isChecked())
	{
		// Translate: Range = [-width, width], value = 0
		m_Slider1->setRange(-(int)m_Handler3D->Width(), m_Handler3D->Width());
		m_Slider1->setSteps(1, 10);
		m_Slider1->setValue(0);
		Slider1Changed(m_Slider1->value());
		m_Slider2->setRange(-(int)m_Handler3D->Height(), m_Handler3D->Height());
		m_Slider2->setSteps(1, 10);
		m_Slider2->setValue(0);
		Slider2Changed(m_Slider2->value());
	}
	else if (m_RotateRadioButton->isChecked())
	{
		// Rotate: Range = [-180, 180], value = 0
		m_Slider1->setRange(-180, 180);
		m_Slider1->setSteps(1, 10);
		m_Slider1->setValue(0);
		Slider1Changed(m_Slider1->value());
	}
	else if (m_ScaleRadioButton->isChecked())
	{
		// Scale: Range = [0, 100], value = 50
		m_Slider1->setRange(0, 100);
		m_Slider1->setSteps(1, 10);
		m_Slider1->setValue(50);
		Slider1Changed(m_Slider1->value());
		m_Slider2->setRange(0, 100);
		m_Slider2->setSteps(1, 10);
		m_Slider2->setValue(50);
		Slider2Changed(m_Slider2->value());
	}
	else if (m_ShearRadioButton->isChecked())
	{
		// Shear: Range = [-45, 45], value = 0
		m_Slider1->setRange(-45, 45);
		m_Slider1->setSteps(1, 10);
		m_Slider1->setValue(0);
		Slider1Changed(m_Slider1->value());
	}

	m_DisableUpdatePreview = false;
}

void TransformWidget::FlipPushButtonClicked()
{
	// Update transformation preview
	UpdatePreview();
}

void TransformWidget::ExecutePushButtonClicked()
{
	DataSelection data_selection;
	data_selection.allSlices = m_AllSlicesCheckBox->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->isChecked();
	data_selection.work = m_TransformTargetCheckBox->isChecked();
	data_selection.tissues = m_TransformTissuesCheckBox->isChecked();

	// Undo start point
	m_SliceTransform->SwapDataPointers();
	emit BeginDatachange(data_selection, this);
	m_SliceTransform->SwapDataPointers();

	// Apply the transformation
	m_SliceTransform->Execute(m_AllSlicesCheckBox->isChecked());

	// Reset the widgets
	ResetWidgets();

	// Undo end point
	emit EndDatachange(this);
}

void TransformWidget::CancelPushButtonClicked()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = m_TransformSourceCheckBox->isChecked();
	data_selection.work = m_TransformTargetCheckBox->isChecked();
	data_selection.tissues = m_TransformTissuesCheckBox->isChecked();
	emit BeginDatachange(data_selection, this, false);

	// Cancel the transformation
	m_SliceTransform->Cancel();

	// Reset the widgets
	ResetWidgets();

	// Signal data change
	emit EndDatachange(this, iseg::NoUndo);
}

} // namespace iseg
