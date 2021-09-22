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

#include "Data/Point.h"
#include "Data/SlicesHandlerInterface.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

class ParamViewBase : public QWidget
{
	Q_OBJECT
public:
	ParamViewBase(QWidget* parent = nullptr) : QWidget(parent) {}

	virtual void Init() {}

	virtual bool Work() const { return m_Work; }
	virtual void SetWork(bool v) { m_Work = v; }

	virtual float ObjectValue() const { return m_ObjectValue; }
	virtual void SetObjectValue(float v) { m_ObjectValue = v; }

private:
	// cache setting
	bool m_Work = true;
	float m_ObjectValue = 0.f;
};

class OLCorrParamView : public ParamViewBase
{
	Q_OBJECT
public:
	OLCorrParamView(QWidget* parent = nullptr);

	bool Work() const override { return m_Target->isOn(); }
	void SetWork(bool v) override
	{
		m_Target->setOn(v);
		m_Tissues->setOn(!v);
	}
	float ObjectValue() const override { return m_ObjectValue->text().toFloat(); }
	void SetObjectValue(float v) override;

	// params
	QRadioButton* m_Target;
	QRadioButton* m_Tissues;

	QPushButton* m_SelectObject;
	QLineEdit* m_ObjectValue;
};

class BrushParamView : public ParamViewBase
{
	Q_OBJECT
public:
	BrushParamView(QWidget* parent = nullptr);

	bool Work() const override { return m_Target->isOn(); }
	void SetWork(bool v) override
	{
		m_Target->setOn(v);
		m_Tissues->setOn(!v);
	}
	float ObjectValue() const override { return m_ObjectValue->text().toFloat(); }
	void SetObjectValue(float v) override;

	std::vector<Point> DrawCircle(Point p, float spacing_x, float spacing_y, int width, int height) const;

	// params
	QRadioButton* m_Tissues;
	QRadioButton* m_Target;

	QPushButton* m_SelectObject;
	QLineEdit* m_ObjectValue;

	QRadioButton* m_Modify;
	QRadioButton* m_Draw;
	QRadioButton* m_Erase;

	QLineEdit* m_Radius;
	QRadioButton* m_UnitMm;
	QRadioButton* m_UnitPixel;

	QCheckBox* m_ShowGuide;
	QSpinBox* m_GuideOffset;
	QPushButton* m_CopyGuide;
	QPushButton* m_CopyPickGuide;

private slots:
	void UnitChanged();
};

// used for fill holes, remove islands, and fill gaps
class FillHolesParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillHolesParamView(QWidget* parent = nullptr);

	bool Work() const override { return m_Target->isOn(); }
	void SetWork(bool v) override
	{
		m_Target->setOn(v);
		m_Tissues->setOn(!v);
	}
	float ObjectValue() const override { return m_ObjectValue->text().toFloat(); }
	void SetObjectValue(float v) override;

	// params
	QCheckBox* m_AllSlices;

	QRadioButton* m_Target;
	QRadioButton* m_Tissues;

	QPushButton* m_SelectObject;
	QLineEdit* m_ObjectValue;

	QSpinBox* m_ObjectSize;
	QLabel* m_ObjectSizeLabel;

	QPushButton* m_Execute;
};

// \todo BL why does this tool not allow to choose object value?
class AddSkinParamView : public ParamViewBase
{
	Q_OBJECT
public:
	AddSkinParamView(QWidget* parent = nullptr);

	bool Work() const override { return m_Target->isOn(); }
	void SetWork(bool v) override
	{
		m_Target->setOn(v);
		m_Tissues->setOn(!v);
	}

	// params
	QCheckBox* m_AllSlices;

	QRadioButton* m_Target;
	QRadioButton* m_Tissues;

	QRadioButton* m_Inside;
	QRadioButton* m_Outside;

	QLineEdit* m_Thickness;
	QRadioButton* m_UnitMm;
	QRadioButton* m_UnitPixel;

	QPushButton* m_Execute;

private slots:
	void UnitChanged();
};

class FillSkinParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillSkinParamView(SlicesHandlerInterface* h, QWidget* parent = nullptr);

	void Init() override;

	void SelectBackground(QString tissueName, tissues_size_t nr);

	void SelectSkin(QString tissueName, tissues_size_t nr);

	// params
	QCheckBox* m_AllSlices;

	QLineEdit* m_Thickness;
	QRadioButton* m_UnitMm;
	QRadioButton* m_UnitPixel;

	QPushButton* m_SelectBackground;
	QLineEdit* m_BackgroundValue;

	QPushButton* m_SelectSkin;
	QLineEdit* m_SkinValue;

	QPushButton* m_Execute;

private slots:
	void UnitChanged();
	void OnSelectBackground();
	void OnSelectSkin();

private:
	// data
	SlicesHandlerInterface* m_Handler;
	tissues_size_t m_SelectedBackgroundId;
	tissues_size_t m_SelectedSkinId;
	bool m_BackgroundSelected = false;
	bool m_SkinSelected = false;
};

class FillAllParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillAllParamView(QWidget* parent = nullptr);

	bool Work() const override { return m_Target->isOn(); }
	void SetWork(bool v) override
	{
		m_Target->setOn(v);
		m_Tissues->setOn(!v);
	}

	// params
	QCheckBox* m_AllSlices;

	QRadioButton* m_Target;
	QRadioButton* m_Tissues;

	QPushButton* m_Execute;
};

class SpherizeParamView : public ParamViewBase
{
	Q_OBJECT
public:
	SpherizeParamView(QWidget* parent = nullptr);

	bool Work() const override { return m_Target->isOn(); }
	void SetWork(bool v) override
	{
		m_Target->setOn(v);
		m_Tissues->setOn(!v);
	}
	float ObjectValue() const override { return m_ObjectValue->text().toFloat(); }
	void SetObjectValue(float v) override;

private slots:
	void UpdateState();

public:
	// params
	QRadioButton* m_Target;
	QRadioButton* m_Tissues;

	QPushButton* m_SelectObject;
	QLineEdit* m_ObjectValue;
	QLineEdit* m_Radius;

	QRadioButton* m_CarveOutside;
	QRadioButton* m_CarveInside;

	QPushButton* m_Execute;
};

class SmoothTissuesParamView : public ParamViewBase
{
	Q_OBJECT
public:
	SmoothTissuesParamView(QWidget* parent = nullptr);

	// params
	QRadioButton* m_ActiveSlice;
	QRadioButton* m_AllSlices;
	QRadioButton* m_3D; // NOLINT
	QLineEdit* m_SplitLimit;
	QLineEdit* m_Sigma;
	QPushButton* m_Execute;
};

} // namespace iseg
