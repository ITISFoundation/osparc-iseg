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
#include "TransformWidget.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

using namespace std;
using namespace iseg;

TransformWidget::TransformWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Shift/rotate/scale the Source, Target or Tissue."));

	sliceTransform = new SliceTransform(hand3D);
	sliceTransform->SelectTransformData(true, true, true);
	updateParameter1 = updateParameter2 = 0.0;
	disableUpdatePreview = true;

	// Boxes
	hBoxOverall = new Q3HBox(this);
	hBoxOverall->setMargin(8);
	vBoxTransforms = new Q3VBox(hBoxOverall);
	vBoxParams = new Q3VBox(hBoxOverall);
	hBoxSelectData = new Q3HBox(vBoxParams);
	hBoxSlider1 = new Q3HBox(vBoxParams);
	hBoxSlider2 = new Q3HBox(vBoxParams);
	hBoxFlip = new Q3HBox(vBoxParams);
	hBoxAxisSelection = new Q3HBox(vBoxParams);
	hBoxCenter = new Q3HBox(vBoxParams);

	// Transformation selection radio buttons
	translateRadioButton =
			new QRadioButton(QString("Translate"), vBoxTransforms);
	rotateRadioButton = new QRadioButton(QString("Rotate"), vBoxTransforms);
	scaleRadioButton = new QRadioButton(QString("Scale"), vBoxTransforms);
	shearRadioButton = new QRadioButton(QString("Shear"), vBoxTransforms);
	flipRadioButton = new QRadioButton(QString("Flip"), vBoxTransforms);
	//matrixRadioButton = new QRadioButton(QString("Matrix"), vBoxTransforms);
	transformButtonGroup = new QButtonGroup(vBoxTransforms);
	transformButtonGroup->insert(translateRadioButton);
	transformButtonGroup->insert(rotateRadioButton);
	transformButtonGroup->insert(scaleRadioButton);
	transformButtonGroup->insert(shearRadioButton);
	transformButtonGroup->insert(flipRadioButton);
	//transformButtonGroup->insert(matrixRadioButton);
	translateRadioButton->setChecked(TRUE);

	// Data selection check boxes
	transformSourceCheckBox = new QCheckBox(QString("Source"), hBoxSelectData);
	transformTargetCheckBox = new QCheckBox(QString("Target"), hBoxSelectData);
	transformTissuesCheckBox =
			new QCheckBox(QString("Tissues"), hBoxSelectData);
	transformSourceCheckBox->setChecked(TRUE);
	transformTargetCheckBox->setChecked(TRUE);
	transformTissuesCheckBox->setChecked(TRUE);

	// Axis selection radio buttons
	xAxisRadioButton = new QRadioButton(QString("x axis "), hBoxAxisSelection);
	yAxisRadioButton = new QRadioButton(QString("y axis "), hBoxAxisSelection);
	axisButtonGroup = new QButtonGroup(hBoxAxisSelection);
	axisButtonGroup->insert(xAxisRadioButton);
	axisButtonGroup->insert(yAxisRadioButton);
	xAxisRadioButton->setChecked(TRUE);

	// Sliders
	slider1Label = new QLabel("Offset x ", hBoxSlider1);
	slider1 = new QSlider(Qt::Horizontal, hBoxSlider1);
	lineEdit1 = new QLineEdit("-00000", hBoxSlider1);
	lineEdit1->setAlignment(Qt::AlignRight);
	slider2Label = new QLabel("Offset y ", hBoxSlider2);
	slider2 = new QSlider(Qt::Horizontal, hBoxSlider2);
	lineEdit2 = new QLineEdit("-00000", hBoxSlider2);
	lineEdit2->setAlignment(Qt::AlignRight);

	// Flip button
	flipPushButton = new QPushButton("Flip", hBoxFlip);

	// Center
	centerLabel = new QLabel("Center coordinates: ", hBoxCenter);
	centerCoordsLabel = new QLabel("(0000, 0000)", hBoxCenter);
	centerSelectPushButton = new QPushButton("Select", hBoxCenter);
	centerSelectPushButton->setCheckable(true);
	SetCenterDefault();

	// Common widgets
	hBoxExecute = new Q3HBox(vBoxParams);
	executePushButton = new QPushButton("Execute", hBoxExecute);
	allSlicesCheckBox =
			new QCheckBox(QString("Apply to all slices"), hBoxExecute);
	allSlicesCheckBox->setLayoutDirection(Qt::RightToLeft);
	cancelPushButton = new QPushButton("Cancel", vBoxParams);

	// Layout
	vBoxParams->setMargin(5);
	vBoxTransforms->setMargin(5);
	vBoxTransforms->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vBoxTransforms->setLineWidth(1);

	slider1->setFixedWidth(300);
	slider2->setFixedWidth(300);

	executePushButton->setFixedWidth(100);
	cancelPushButton->setFixedWidth(100);

	vBoxTransforms->setFixedSize(vBoxTransforms->sizeHint());
	vBoxParams->setFixedSize(vBoxParams->sizeHint());
	hBoxSelectData->setFixedSize(hBoxSelectData->sizeHint());
	hBoxSlider1->setFixedSize(hBoxSlider1->sizeHint());
	hBoxSlider2->setFixedSize(hBoxSlider2->sizeHint());
	hBoxFlip->setFixedSize(hBoxFlip->sizeHint());
	hBoxAxisSelection->setFixedSize(hBoxAxisSelection->sizeHint());
	hBoxCenter->setFixedSize(hBoxCenter->sizeHint());
	hBoxExecute->setFixedSize(hBoxExecute->sizeHint());

	centerCoordsLabel->setFixedSize(centerCoordsLabel->sizeHint());
	lineEdit1->setFixedSize(lineEdit1->sizeHint());
	lineEdit2->setFixedSize(lineEdit2->sizeHint());

	hBoxSelectData->show();
	hBoxSlider1->show();
	hBoxSlider2->show();
	hBoxFlip->hide();
	hBoxAxisSelection->hide();
	hBoxCenter->hide();

	// Signals
	QObject::connect(transformButtonGroup, SIGNAL(buttonClicked(int)), this,
			SLOT(TransformChanged(int)));

	QObject::connect(transformSourceCheckBox, SIGNAL(stateChanged(int)), this,
			SLOT(SelectSourceChanged(int)));
	QObject::connect(transformTargetCheckBox, SIGNAL(stateChanged(int)), this,
			SLOT(SelectTargetChanged(int)));
	QObject::connect(transformTissuesCheckBox, SIGNAL(stateChanged(int)), this,
			SLOT(SelectTissuesChanged(int)));

	QObject::connect(slider1, SIGNAL(valueChanged(int)), this,
			SLOT(Slider1Changed(int)));
	QObject::connect(slider2, SIGNAL(valueChanged(int)), this,
			SLOT(Slider2Changed(int)));

	QObject::connect(lineEdit1, SIGNAL(editingFinished()), this,
			SLOT(LineEdit1Edited()));
	QObject::connect(lineEdit2, SIGNAL(editingFinished()), this,
			SLOT(LineEdit2Edited()));

	QObject::connect(flipPushButton, SIGNAL(clicked()), this,
			SLOT(FlipPushButtonClicked()));
	QObject::connect(executePushButton, SIGNAL(clicked()), this,
			SLOT(ExecutePushButtonClicked()));
	QObject::connect(cancelPushButton, SIGNAL(clicked()), this,
			SLOT(CancelPushButtonClicked()));
}

TransformWidget::~TransformWidget() { delete sliceTransform; }

void TransformWidget::Slider1Changed(int value)
{
	lineEdit1->blockSignals(true);
	slider1->blockSignals(true);

	// Update parameter and label values
	if (translateRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-width, width]
		updateParameter1 = value;
		lineEdit1->setText(QString("%1").arg(updateParameter1, 0, 'f', 2));
	}
	else if (rotateRadioButton->isChecked())
	{
		// Rotate: Linear scale: y = x, x in [-180, 180]
		updateParameter1 = value;
		lineEdit1->setText(QString("%1").arg(updateParameter1, 0, 'f', 2));
	}
	else if (scaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		updateParameter1 = std::pow(10, value / 50.0 - 1.0);
		updateParameter1 = std::floor(updateParameter1 * 100) /
											 100.0; // Round to two fractional digits
		lineEdit1->setText(QString("%1").arg(updateParameter1, 0, 'f', 2));
	}
	else if (shearRadioButton->isChecked())
	{
		// Shear: Linear scale: y = x, x in [-45, 45]
		updateParameter1 = value;
		lineEdit1->setText(QString("%1").arg(updateParameter1, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	lineEdit1->blockSignals(false);
	slider1->blockSignals(false);
}

void TransformWidget::Slider2Changed(int value)
{
	lineEdit2->blockSignals(true);
	slider2->blockSignals(true);

	if (translateRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-height, height]
		updateParameter2 = value;
		lineEdit2->setText(QString("%1").arg(updateParameter2, 0, 'f', 2));
	}
	else if (scaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		updateParameter2 = std::pow(10, value / 50.0 - 1.0);
		updateParameter2 = std::floor(updateParameter2 * 100) /
											 100.0; // Round to two fractional digits
		lineEdit2->setText(QString("%1").arg(updateParameter2, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	lineEdit2->blockSignals(false);
	slider2->blockSignals(false);
}

void TransformWidget::LineEdit1Edited()
{
	lineEdit1->blockSignals(true);
	slider1->blockSignals(true);

	double value = lineEdit1->text().toDouble();
	if (translateRadioButton->isChecked() || rotateRadioButton->isChecked() ||
			shearRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-width, width]
		// Rotate: Linear scale: y = x, x in [-180, 180]
		// Shear: Linear scale: y = x, x in [-45, 45]
		updateParameter1 = max((double)slider1->minimum(),
				min(value, (double)slider1->maximum()));
		slider1->setValue(std::floor(updateParameter1 + 0.5));
		lineEdit1->setText(QString("%1").arg(updateParameter1, 0, 'f', 2));
	}
	else if (scaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		updateParameter1 = max(0.1, min(value, 10.0));
		slider1->setValue(
				std::floor((std::log10(updateParameter1) + 1) * 50.0 + 0.5));
		lineEdit1->setText(QString("%1").arg(updateParameter1, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	lineEdit1->blockSignals(false);
	slider1->blockSignals(false);
}

void TransformWidget::LineEdit2Edited()
{
	lineEdit2->blockSignals(true);
	slider2->blockSignals(true);

	double value = lineEdit2->text().toDouble();
	if (translateRadioButton->isChecked())
	{
		// Translate: Linear scale: y = x, x in [-height, height]
		updateParameter2 = max((double)slider2->minimum(),
				min(value, (double)slider2->maximum()));
		slider2->setValue(std::floor(updateParameter2 + 0.5));
		lineEdit2->setText(QString("%1").arg(updateParameter2, 0, 'f', 2));
	}
	else if (scaleRadioButton->isChecked())
	{
		// Scale: Logarithmic scale: y = 10^(x/50-1), x in [0, 100]
		updateParameter2 = max(0.1, min(value, 10.0));
		slider2->setValue(
				std::floor((std::log10(updateParameter2) + 1) * 50.0 + 0.5));
		lineEdit2->setText(QString("%1").arg(updateParameter2, 0, 'f', 2));
	}

	// Update transformation preview
	UpdatePreview();

	lineEdit2->blockSignals(false);
	slider2->blockSignals(false);
}

void TransformWidget::UpdatePreview()
{
	if (disableUpdatePreview)
	{
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = transformSourceCheckBox->isChecked();
	dataSelection.work = transformTargetCheckBox->isChecked();
	dataSelection.tissues = transformTissuesCheckBox->isChecked();
	emit begin_datachange(dataSelection, this, false);

	if (translateRadioButton->isOn())
	{
		// Translate
		sliceTransform->Translate(
				updateParameter1 - transformParameters.translationOffset[0],
				updateParameter2 - transformParameters.translationOffset[1]);
		transformParameters.translationOffset[0] = updateParameter1;
		transformParameters.translationOffset[1] = updateParameter2;
	}
	else if (rotateRadioButton->isOn())
	{
		// Rotate
		sliceTransform->Rotate(updateParameter1 -
															 transformParameters.rotationAngle,
				transformParameters.transformCenter[0],
				transformParameters.transformCenter[1]);
		transformParameters.rotationAngle = updateParameter1;
	}
	else if (scaleRadioButton->isOn())
	{
		// Scale
		sliceTransform->Scale(
				updateParameter1 / transformParameters.scalingFactor[0],
				updateParameter2 / transformParameters.scalingFactor[1],
				transformParameters.transformCenter[0],
				transformParameters.transformCenter[1]);
		transformParameters.scalingFactor[0] = updateParameter1;
		transformParameters.scalingFactor[1] = updateParameter2;
	}
	else if (shearRadioButton->isOn())
	{
		// Shear
		if (xAxisRadioButton->isChecked())
		{
			sliceTransform->Shear(updateParameter1 -
																transformParameters.shearingAngle,
					true, transformParameters.transformCenter[0]);
		}
		else
		{
			sliceTransform->Shear(
					updateParameter1 - transformParameters.shearingAngle, false,
					transformParameters.transformCenter[1]);
		}
		transformParameters.shearingAngle = updateParameter1;
	}
	else if (flipRadioButton->isChecked())
	{
		// Update flipping
		if (xAxisRadioButton->isChecked())
		{
			sliceTransform->Flip(true, transformParameters.transformCenter[0]);
		}
		else
		{
			sliceTransform->Flip(false, transformParameters.transformCenter[1]);
		}
	}

	// Signal data change
	emit end_datachange(this, iseg::NoUndo);
}

void TransformWidget::TransformChanged(int index)
{
	// Reset the tab widgets
	ResetWidgets();

	// Show the corresponding widgets
	if (translateRadioButton->isChecked())
	{
		// Translate
		slider1Label->setText("Offset x ");
		slider2Label->setText("Offset y ");
		hBoxSlider1->show();
		hBoxSlider2->show();
		hBoxFlip->hide();
		hBoxAxisSelection->hide();
		hBoxCenter->hide();
	}
	else if (rotateRadioButton->isChecked())
	{
		// Rotate
		slider1Label->setText("Angle ");
		hBoxSlider1->show();
		hBoxSlider2->hide();
		hBoxFlip->hide();
		hBoxAxisSelection->hide();
		hBoxCenter->show();
	}
	else if (scaleRadioButton->isChecked())
	{
		// Scale
		slider1Label->setText("Factor x ");
		slider2Label->setText("Factor y ");
		hBoxSlider1->show();
		hBoxSlider2->show();
		hBoxFlip->hide();
		hBoxAxisSelection->hide();
		hBoxCenter->show();
	}
	else if (shearRadioButton->isChecked())
	{
		// Shear
		slider1Label->setText("Angle ");
		hBoxSlider1->show();
		hBoxSlider2->hide();
		hBoxFlip->hide();
		hBoxAxisSelection->show();
		hBoxCenter->show();
		xAxisRadioButton->setChecked(true);
	}
	else if (flipRadioButton->isChecked())
	{
		// Flip
		hBoxSlider1->hide();
		hBoxSlider2->hide();
		hBoxFlip->show();
		hBoxAxisSelection->show();
		hBoxCenter->show();
		xAxisRadioButton->setChecked(true);
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
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = true;
	emit begin_datachange(dataSelection, this, false);

	// Set data to be transformed
	sliceTransform->SelectTransformData(transformSourceCheckBox->isChecked(),
			transformTargetCheckBox->isChecked(),
			transformTissuesCheckBox->isChecked());

	// Signal data change
	emit end_datachange(this, iseg::NoUndo);
}

void TransformWidget::SelectTargetChanged(int state)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this, false);

	// Set data to be transformed
	sliceTransform->SelectTransformData(transformSourceCheckBox->isChecked(),
			transformTargetCheckBox->isChecked(),
			transformTissuesCheckBox->isChecked());

	// Signal data change
	emit end_datachange(this, iseg::NoUndo);
}

void TransformWidget::SelectTissuesChanged(int state)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	// Set data to be transformed
	sliceTransform->SelectTransformData(transformSourceCheckBox->isChecked(),
			transformTargetCheckBox->isChecked(),
			transformTissuesCheckBox->isChecked());

	// Signal data change
	emit end_datachange(this, iseg::NoUndo);
}

QSize TransformWidget::sizeHint() const { return hBoxOverall->sizeHint(); }

void TransformWidget::on_slicenr_changed()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = transformSourceCheckBox->isChecked();
	dataSelection.work = transformTargetCheckBox->isChecked();
	dataSelection.tissues = transformTissuesCheckBox->isChecked();
	emit begin_datachange(dataSelection, this, false);

	// Change active slice in transform object
	sliceTransform->ActiveSliceChanged();

	// Signal data change
	emit end_datachange(this, iseg::NoUndo);
}

void TransformWidget::init()
{
	// Initialize transformation
	sliceTransform->Initialize();

	// Show tab
	hideparams_changed();
}

void TransformWidget::cleanup()
{
	// Cancel transform
	CancelPushButtonClicked();

	// Hide tab
	hideparams_changed();
}

void TransformWidget::newloaded()
{
	// Reload transform data
	sliceTransform->NewDataLoaded();

	// Reset the tab widgets
	ResetWidgets();

	// Reset center to default
	SetCenterDefault();
}

void TransformWidget::hideparams_changed()
{
	if (hideparams)
	{
		vBoxParams->hide();
	}
	else
	{
		ResetWidgets();
		SetCenterDefault();
		vBoxParams->show();
	}
}

bool TransformWidget::GetIsIdentityTransform()
{
	return sliceTransform->GetIsIdentityTransform();
}

void TransformWidget::GetDataSelection(bool& source, bool& target,
		bool& tissues)
{
	source = transformSourceCheckBox->isChecked();
	target = transformTargetCheckBox->isChecked();
	tissues = transformTissuesCheckBox->isChecked();
}

void TransformWidget::on_mouse_clicked(Point p)
{
	if (centerSelectPushButton->isChecked())
	{
		ResetWidgets();
		SetCenter(p.px, p.py);
		centerSelectPushButton->setChecked(false);
	}
}

void TransformWidget::BitsChanged() { newloaded(); }

void TransformWidget::bmp_changed() { BitsChanged(); }

void TransformWidget::work_changed() { BitsChanged(); }

void TransformWidget::tissues_changed() { BitsChanged(); }

void TransformWidget::SetCenter(int x, int y)
{
	transformParameters.transformCenter[0] = x;
	transformParameters.transformCenter[1] = y;

	// Display center coordinates
	centerCoordsLabel->setText(QString("(%1, %2)").arg(x).arg(y));
}

void TransformWidget::SetCenterDefault()
{
	SetCenter(handler3D->width() / 2, handler3D->height() / 2);
}

void TransformWidget::ResetWidgets()
{
	disableUpdatePreview = true;

	// Reset transform parameters
	transformParameters.translationOffset[0] =
			transformParameters.translationOffset[1] = 0;
	transformParameters.rotationAngle = 0.0;
	transformParameters.scalingFactor[0] =
			transformParameters.scalingFactor[1] = 1.0;
	transformParameters.shearingAngle = 0.0;

	// Reset sliders
	if (translateRadioButton->isChecked())
	{
		// Translate: Range = [-width, width], value = 0
		slider1->setRange(-(int)handler3D->width(),
				handler3D->width());
		slider1->setSteps(1, 10);
		slider1->setValue(0);
		Slider1Changed(slider1->value());
		slider2->setRange(-(int)handler3D->height(),
				handler3D->height());
		slider2->setSteps(1, 10);
		slider2->setValue(0);
		Slider2Changed(slider2->value());
	}
	else if (rotateRadioButton->isChecked())
	{
		// Rotate: Range = [-180, 180], value = 0
		slider1->setRange(-180, 180);
		slider1->setSteps(1, 10);
		slider1->setValue(0);
		Slider1Changed(slider1->value());
	}
	else if (scaleRadioButton->isChecked())
	{
		// Scale: Range = [0, 100], value = 50
		slider1->setRange(0, 100);
		slider1->setSteps(1, 10);
		slider1->setValue(50);
		Slider1Changed(slider1->value());
		slider2->setRange(0, 100);
		slider2->setSteps(1, 10);
		slider2->setValue(50);
		Slider2Changed(slider2->value());
	}
	else if (shearRadioButton->isChecked())
	{
		// Shear: Range = [-45, 45], value = 0
		slider1->setRange(-45, 45);
		slider1->setSteps(1, 10);
		slider1->setValue(0);
		Slider1Changed(slider1->value());
	}

	disableUpdatePreview = false;
}

void TransformWidget::FlipPushButtonClicked()
{
	// Update transformation preview
	UpdatePreview();
}

void TransformWidget::ExecutePushButtonClicked()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allSlicesCheckBox->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = transformSourceCheckBox->isChecked();
	dataSelection.work = transformTargetCheckBox->isChecked();
	dataSelection.tissues = transformTissuesCheckBox->isChecked();

	// Undo start point
	sliceTransform->SwapDataPointers();
	emit begin_datachange(dataSelection, this);
	sliceTransform->SwapDataPointers();

	// Apply the transformation
	sliceTransform->Execute(allSlicesCheckBox->isChecked());

	// Reset the widgets
	ResetWidgets();

	// Undo end point
	emit end_datachange(this);
}

void TransformWidget::CancelPushButtonClicked()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = transformSourceCheckBox->isChecked();
	dataSelection.work = transformTargetCheckBox->isChecked();
	dataSelection.tissues = transformTissuesCheckBox->isChecked();
	emit begin_datachange(dataSelection, this, false);

	// Cancel the transformation
	sliceTransform->Cancel();

	// Reset the widgets
	ResetWidgets();

	// Signal data change
	emit end_datachange(this, iseg::NoUndo);
}
