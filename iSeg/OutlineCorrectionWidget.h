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

#include "Data/Vec3.h"

#include "Interface/WidgetInterface.h"

class QButtonGroup;
class QRadioButton;
class QStackedWidget;

namespace iseg {

class SlicesHandler;
class Bmphandler;
class ParamViewBase;
class OLCorrParamView;
class BrushParamView;
class FillHolesParamView;
class AddSkinParamView;
class FillSkinParamView;
class FillAllParamView;
class SpherizeParamView;
class SmoothTissuesParamView;

/** \brief Class which contains different methods to modify/correct target or tissues
*/
class OutlineCorrectionWidget : public WidgetInterface
{
	Q_OBJECT
public:
	OutlineCorrectionWidget(SlicesHandler* hand3D);
	~OutlineCorrectionWidget() override = default;
	void Cleanup() override;

	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("OLC"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("olc.png"))); }

	void BmphandChanged(Bmphandler* bmph);

private:
	void OnTissuenrChanged(int i) override;
	void OnSlicenrChanged() override;

	void OnMouseClicked(Point p) override;
	void OnMouseReleased(Point p) override;
	void OnMouseMoved(Point p) override;
	void WorkChanged() override;

	void DrawCircle(Point p);

	float GetObjectValue() const;
	void WorkbitsChanged();

	// parameter view
	QButtonGroup* m_Methods;
	QRadioButton* m_Olcorr;
	QRadioButton* m_Brush;
	QRadioButton* m_Holefill;
	QRadioButton* m_Removeislands;
	QRadioButton* m_Gapfill;
	QRadioButton* m_Addskin;
	QRadioButton* m_Fillskin;
	QRadioButton* m_Allfill;
	QRadioButton* m_Spherize;
	QRadioButton* m_SmoothTissues;

	ParamViewBase* m_CurrentParams = nullptr;
	QStackedWidget* m_StackedParams;
	OLCorrParamView* m_OlcParams;
	BrushParamView* m_BrushParams;
	FillHolesParamView* m_FillHolesParams;
	FillHolesParamView* m_RemoveIslandsParams;
	FillHolesParamView* m_FillGapsParams;
	AddSkinParamView* m_AddSkinParams;
	FillSkinParamView* m_FillSkinParams;
	FillAllParamView* m_FillAllParams;
	SpherizeParamView* m_SpherizeParams;
	SmoothTissuesParamView* m_SmoothTissuesParams;

	// member/state variables
	tissues_size_t m_Tissuenr;
	tissues_size_t m_Tissuenrnew;
	bool m_Draw;
	bool m_Selectobj;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	Vec3 m_Spacing;
	Point m_LastPt;

	std::vector<Point> m_Vpdyn;
	bool m_Dontundo;
	bool m_CopyMode = false;

private slots:
	void MethodChanged();
	void ExecutePushed();
	void SelectobjPushed();
	void DrawGuide();
	void CopyGuide(Point* p = nullptr);
	void CopyPickPushed();
	void CarvePushed();
	void SmoothTissuesPushed();
};

} // namespace iseg
