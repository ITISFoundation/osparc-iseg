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

#include "Data/Property.h"

namespace iseg {

class TransformWidget : public WidgetInterface
{
	Q_OBJECT
public:
	TransformWidget(SlicesHandler* hand3D);
	~TransformWidget() override;

	void Init() override;
	void Cleanup() override;
	void NewLoaded() override;
	void HideParamsChanged() override;

	std::string GetName() override { return std::string("Transform"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("transform.png"))); }

	void GetDataSelection(bool& source, bool& target, bool& tissues);

	void CancelTransform();

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(Point p) override;
	void BmpChanged() override;
	void WorkChanged() override;
	void TissuesChanged() override;

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

	void Execute();
	void TransformChanged();
	void SelectSourceChanged();
	void SelectTargetChanged();
	void SelectTissuesChanged();
	void FlipPushButtonClicked();

	// Image data
	SlicesHandler* m_Handler3D;

	// Slice transform
	SliceTransform* m_SliceTransform;

	// Widgets
	PropertyBool_ptr m_TransformSourceCheckBox;
	PropertyBool_ptr m_TransformTargetCheckBox;
	PropertyBool_ptr m_TransformTissuesCheckBox;
	PropertyBool_ptr m_AllSlicesCheckBox;

	enum eTransformType {
		kTranslate,
		kRotate,
		kScale,
		kShear,
		kFlip
	};
	PropertyEnum_ptr m_TransformType;

	enum eAxisDirection {
		kX,
		kY
	};
	PropertyEnum_ptr m_AxisSelection;
	PropertyBool_ptr m_FlipCheckBox;

	PropertyButton_ptr m_CenterSelectPushButton;
	PropertyString_ptr m_CenterCoordsLabel;

	PropertyInt_ptr m_TranslateX, m_TranslateY;
	PropertyReal_ptr m_ScaleX, m_ScaleY;
	PropertyReal_ptr m_RotationAngle;
	PropertyReal_ptr m_ShearingAngle;

	PropertyButton_ptr m_ExecutePushButton;
	PropertyButton_ptr m_CancelPushButton;

	// Transform parameters
	bool m_DisableUpdatePreview;
	TransformParametersStruct m_TransformParameters;
};

} // namespace iseg
