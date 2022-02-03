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

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include "Data/Property.h"

#include <algorithm>
#include <array>

namespace iseg {

class HystereticGrowingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	HystereticGrowingWidget(SlicesHandler* hand3D);
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	std::string GetName() override { return std::string("Growing"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("growing.png")); }

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(Point p) override;
	void OnMouseMoved(Point p) override;
	void OnMouseReleased(Point p) override;
	void BmpChanged() override;

	void Init1();
	void Getrange();
	void GetrangeSub(float ll, float uu, float ul, float lu);
	void Execute1();

	Point m_P1;
	Point m_LastPt;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	float m_UpperLimit;
	float m_LowerLimit;
	bool m_Limitdrawing;
	std::vector<Point> m_Vp1;
	std::vector<Point> m_Vpdyn;
	bool m_Dontundo;

	enum eSlider {
		kLower = 0,
		kUpper,
		kLowerHyst,
		kUpperHyst
	};
	std::array<int, 4> m_Thresholds;

	PropertySlider_ptr m_SlLower;
	PropertySlider_ptr m_SlUpper;
	PropertySlider_ptr m_SlLowerhyster;
	PropertySlider_ptr m_SlUpperhyster;

	PropertyButton_ptr m_Drawlimit;
	PropertyButton_ptr m_Clearlimit;
	PropertyButton_ptr m_Pushexec;

	PropertyBool_ptr m_Autoseed;
	PropertyBool_ptr m_Allslices;

	PropertyReal_ptr m_LeBordervall;
	PropertyReal_ptr m_LeBordervalu;
	PropertyReal_ptr m_LeBordervallh;
	PropertyReal_ptr m_LeBordervaluh;

signals:
	void Vp1Changed(std::vector<Point>* vp1_arg);
	void Vp1dynChanged(std::vector<Point>* vp1_arg, std::vector<Point>* vpdyn_arg);

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void AutoToggled();
	void UpdateVisible();
	void Execute();
	void Limitpressed();
	void Clearpressed();
	void SliderChanged(eSlider which, int new_value);
	void SliderPressed();
	void SliderReleased();
	void LeBordervallReturnpressed();
	void LeBordervaluReturnpressed();
	void LeBordervallhReturnpressed();
	void LeBordervaluhReturnpressed();
};

} // namespace iseg
