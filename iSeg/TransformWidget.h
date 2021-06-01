/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "SliceTransform.h"

#include "Interface/WidgetInterface.h"

#include <q3vbox.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <qlayout.h>
#include <QLineEdit>
#include <qpixmap.h>
#include <QPushButton>
#include <QRadioButton>
#include <qsize.h>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

class TransformWidget : public WidgetInterface
{
	Q_OBJECT

public:
	TransformWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~TransformWidget() override;

	void Init() override;
	void Cleanup() override;
	void NewLoaded() override;
	void HideParamsChanged() override;

	QSize sizeHint() const override;
	std::string GetName() override { return std::string("Transform"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("transform.png"))); }

	void GetDataSelection(bool& source, bool& target, bool& tissues);

private:
	struct TransformParametersStruct
	{
		int m_TranslationOffset[2];
		double m_RotationAngle;
		double m_ScalingFactor[2];
		double m_ShearingAngle;
		int m_TransformCenter[2];
	};

	bool GetIsIdentityTransform();

	void UpdatePreview();
	void SetCenter(int x, int y);
	void SetCenterDefault();
	void ResetWidgets();
	void BitsChanged();

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(Point p) override;
	void BmpChanged() override;
	void WorkChanged() override;
	void TissuesChanged() override;

	// Image data
	SlicesHandler* m_Handler3D;

	// Slice transform
	SliceTransform* m_SliceTransform;

	// Widgets
	Q3HBox* m_HBoxOverall;
	Q3VBox* m_VBoxTransforms;
	Q3VBox* m_VBoxParams;
	Q3HBox* m_HBoxSelectData;
	Q3HBox* m_HBoxSlider1;
	Q3HBox* m_HBoxSlider2;
	Q3HBox* m_HBoxFlip;
	Q3HBox* m_HBoxAxisSelection;
	Q3HBox* m_HBoxCenter;
	Q3HBox* m_HBoxExecute;

	QCheckBox* m_TransformSourceCheckBox;
	QCheckBox* m_TransformTargetCheckBox;
	QCheckBox* m_TransformTissuesCheckBox;

	QCheckBox* m_AllSlicesCheckBox;
	QPushButton* m_ExecutePushButton;
	QPushButton* m_CancelPushButton;

	QRadioButton* m_TranslateRadioButton;
	QRadioButton* m_RotateRadioButton;
	QRadioButton* m_ScaleRadioButton;
	QRadioButton* m_ShearRadioButton;
	QRadioButton* m_FlipRadioButton;
	//QRadioButton *matrixRadioButton;
	QButtonGroup* m_TransformButtonGroup;

	QLabel* m_Slider1Label;
	QLabel* m_Slider2Label;
	QSlider* m_Slider1;
	QSlider* m_Slider2;
	QLineEdit* m_LineEdit1;
	QLineEdit* m_LineEdit2;

	QRadioButton* m_XAxisRadioButton;
	QRadioButton* m_YAxisRadioButton;
	QButtonGroup* m_AxisButtonGroup;

	QPushButton* m_FlipPushButton;

	QLabel* m_CenterLabel;
	QLabel* m_CenterCoordsLabel;
	QPushButton* m_CenterSelectPushButton;

	// Transform parameters
	bool m_DisableUpdatePreview;
	double m_UpdateParameter1;
	double m_UpdateParameter2;
	TransformParametersStruct m_TransformParameters;

public slots:
	void CancelPushButtonClicked();
	void ExecutePushButtonClicked();

private slots:
	void Slider1Changed(int value);
	void Slider2Changed(int value);
	void LineEdit1Edited();
	void LineEdit2Edited();
	void TransformChanged(int index);
	void SelectSourceChanged(int state);
	void SelectTargetChanged(int state);
	void SelectTissuesChanged(int state);
	void FlipPushButtonClicked();
};

} // namespace iseg
