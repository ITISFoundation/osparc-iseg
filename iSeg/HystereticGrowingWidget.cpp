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

#include "HystereticGrowingWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/PropertyWidget.h"
#include "Interface/RecentPlaces.h"

#include "Core/Pair.h"

#include "Data/addLine.h"

#include <QBoxLayout>

namespace iseg {

HystereticGrowingWidget::HystereticGrowingWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format(
		"Segment a tissue by picking seed points and adding "
		"neighboring pixels with similar intensities."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	Pair p;
	m_P1.px = m_P1.py = 0;
	m_Bmphand->GetBmprange(&p);
	m_LowerLimit = p.low;
	m_UpperLimit = p.high;

	auto group = PropertyGroup::Create("Settings");

	m_SlLower = group->Add("Lower", PropertySlider::Create(80, 0, 200));
	m_SlUpper = group->Add("Upper", PropertySlider::Create(120, 0, 200));

	// TODO BL
	m_LeBordervall = group->Add("Lower MAX", PropertyReal::Create(200));
	m_LeBordervalu = group->Add("Upper MAX", PropertyReal::Create(200));

	m_Autoseed = group->Add("Auto-Seed", PropertyBool::Create(false));
	m_Allslices = group->Add("All Slices", PropertyBool::Create(false));

	m_SlLowerhyster = group->Add("Lower", PropertySlider::Create(60, 0, 200));
	m_SlUpperhyster = group->Add("Upper", PropertySlider::Create(140, 0, 200));

	// TODO BL
	m_LeBordervallh = group->Add("Lower MAX", PropertyReal::Create(200));
	m_LeBordervaluh = group->Add("Upper MAX", PropertyReal::Create(200));

	m_Drawlimit = group->Add("Draw Limit", PropertyButton::Create([this]() { Limitpressed(); }));
	m_Clearlimit = group->Add("Clear Limit", PropertyButton::Create([this]() { Clearpressed(); }));
	m_Pushexec = group->Add("Execute", PropertyButton::Create([this]() { Execute(); }));

	// connections
	m_Autoseed->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			AutoToggled();
	});

	const auto on_slider_pressed = [this](int) {
		SliderPressed();
	};
	const auto on_slider_moved = [this](int v, eSlider which) {
		SliderChanged(which, v);
	};
	const auto on_slider_released = [this](int) {
		SliderReleased();
	};

	m_SlLower->onPressed.connect(on_slider_pressed);
	m_SlUpper->onPressed.connect(on_slider_pressed);
	m_SlLowerhyster->onPressed.connect(on_slider_pressed);
	m_SlUpperhyster->onPressed.connect(on_slider_pressed);

	m_SlLower->onMoved.connect(std::bind(on_slider_moved, std::placeholders::_1, kLower));
	m_SlUpper->onMoved.connect(std::bind(on_slider_moved, std::placeholders::_1, kUpper));
	m_SlLowerhyster->onMoved.connect(std::bind(on_slider_moved, std::placeholders::_1, kLowerHyst));
	m_SlUpperhyster->onMoved.connect(std::bind(on_slider_moved, std::placeholders::_1, kUpperHyst));

	m_SlLower->onReleased.connect(on_slider_released);
	m_SlUpper->onReleased.connect(on_slider_released);
	m_SlLowerhyster->onReleased.connect(on_slider_released);
	m_SlUpperhyster->onReleased.connect(on_slider_released);

	m_LeBordervall->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			LeBordervallReturnpressed();
	});
	m_LeBordervalu->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			LeBordervaluReturnpressed();
	});
	m_LeBordervallh->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			LeBordervallhReturnpressed();
	});
	m_LeBordervaluh->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			LeBordervaluhReturnpressed();
	});

	// add widget and layout
	auto property_view = new PropertyWidget(group);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view, 2);
	layout->addStretch(1);
	setLayout(layout);

	// initalize
	UpdateVisible();
	Getrange();
	Init1();
}

void HystereticGrowingWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void HystereticGrowingWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;

	Getrange();

	Init1();
}

void HystereticGrowingWidget::OnMouseClicked(Point p)
{
	if (m_Limitdrawing)
	{
		m_LastPt = p;
	}
	else
	{
		if (!m_Autoseed->Value())
		{
			m_P1 = p;
			Execute();
		}
	}
}

void HystereticGrowingWidget::UpdateVisible()
{
	m_SlLowerhyster->SetVisible(m_Autoseed->Value());
	m_SlUpperhyster->SetVisible(m_Autoseed->Value());
	m_LeBordervallh->SetVisible(m_Autoseed->Value());
	m_LeBordervaluh->SetVisible(m_Autoseed->Value());

	if (m_Autoseed->Value())
	{
		m_Allslices->SetDescription("Apply to all slices");
	}
	else
	{
		m_Allslices->SetDescription("3D");
	}
}

void HystereticGrowingWidget::AutoToggled()
{
	UpdateVisible();

	if (m_Autoseed->Value())
	{
		Execute();
	}
}

void HystereticGrowingWidget::Execute()
{
	DataSelection data_selection;
	data_selection.work = data_selection.limits = true;
	data_selection.allSlices = m_Allslices->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	emit BeginDatachange(data_selection, this);

	// Execute1 gets the parameters from m_Thresholds, make sure they are updated now
	m_Thresholds[eSlider::kLower] = m_SlLower->Value();
	m_Thresholds[eSlider::kUpper] = m_SlUpper->Value();
	m_Thresholds[eSlider::kLowerHyst] = m_SlLowerhyster->Value();
	m_Thresholds[eSlider::kUpperHyst] = m_SlUpperhyster->Value();

	Execute1();

	emit EndDatachange(this);
}

void HystereticGrowingWidget::Execute1()
{
	if (m_Autoseed->Value())
	{
		float ll = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_Thresholds[eSlider::kLower];
		float uu = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_Thresholds[eSlider::kUpper];
		float ul = ll + (uu - ll) * 0.005f * m_Thresholds[eSlider::kLowerHyst];
		float lu = ll + (uu - ll) * 0.005f * m_Thresholds[eSlider::kUpperHyst];

		if (m_Allslices->Value())
			m_Handler3D->DoubleHystereticAllslices(ll, ul, lu, uu, false, 255.0f);
		else
			m_Bmphand->DoubleHysteretic(ll, ul, lu, uu, false, 255.0f);

		m_LeBordervall->SetValue(ll);
		m_LeBordervalu->SetValue(uu);
		m_LeBordervallh->SetValue(int(0.4f + (ul - ll) / (uu - ll) * 100));
		m_LeBordervaluh->SetValue(int(0.4f + (lu - ll) / (uu - ll) * 100));
	}
	else
	{
		float ll = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_Thresholds[eSlider::kLower];
		float uu = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_Thresholds[eSlider::kUpper];

		if (m_Allslices->Value())
		{
			m_Handler3D->ThresholdedGrowing(m_Handler3D->ActiveSlice(), m_P1, ll, uu, 255.f);
		}
		else
		{
			m_Bmphand->ThresholdedGrowinglimit(m_P1, ll, uu, false, 255.f);
		}

		m_LeBordervall->SetValue(ll);
		m_LeBordervalu->SetValue(uu);
	}
}

void HystereticGrowingWidget::Getrange()
{
	float ll = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->Value();
	float uu = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->Value();
	float ul = ll + (uu - ll) * 0.005f * m_SlLowerhyster->Value();
	float lu = ll + (uu - ll) * 0.005f * m_SlUpperhyster->Value();

	Pair p;
	m_Bmphand->GetBmprange(&p);
	m_LowerLimit = p.low;
	m_UpperLimit = p.high;

	GetrangeSub(ll, uu, ul, lu);
}

void HystereticGrowingWidget::GetrangeSub(float ll, float uu, float ul, float lu)
{
	if (ll < m_LowerLimit)
	{
		ll = m_LowerLimit;
		m_SlLower->SetValue(0);
	}
	else if (ll > m_UpperLimit)
	{
		ll = m_UpperLimit;
		m_SlLower->SetValue(200);
	}
	else
	{
		if (m_UpperLimit == m_LowerLimit)
			m_SlLower->SetValue(100);
		else
			m_SlLower->SetValue(int(0.5f + (ll - m_LowerLimit) / (m_UpperLimit - m_LowerLimit) * 200));
	}

	if (uu < m_LowerLimit)
	{
		uu = m_LowerLimit;
		m_SlUpper->SetValue(0);
	}
	else if (uu > m_UpperLimit)
	{
		uu = m_UpperLimit;
		m_SlUpper->SetValue(200);
	}
	else
	{
		if (m_UpperLimit == m_LowerLimit)
			m_SlUpper->SetValue(100);
		else
			m_SlUpper->SetValue(int(0.5f + (uu - m_LowerLimit) / (m_UpperLimit - m_LowerLimit) * 200));
	}

	if (ul < ll)
	{
		ul = ll;
		m_SlLowerhyster->SetValue(0);
	}
	else if (ul > uu)
	{
		ul = uu;
		m_SlLowerhyster->SetValue(200);
	}
	else
	{
		if (uu == ll)
			m_SlLowerhyster->SetValue(100);
		else
			m_SlLowerhyster->SetValue(int(0.5f + (ul - ll) / (uu - ll) * 200));
	}

	if (lu < ll)
	{
		lu = ll;
		m_SlUpperhyster->SetValue(0);
	}
	else if (lu > uu)
	{
		lu = uu;
		m_SlUpperhyster->SetValue(200);
	}
	else
	{
		if (uu == ll)
			m_SlUpperhyster->SetValue(100);
		else
			m_SlUpperhyster->SetValue(int(0.5f + (lu - ll) / (uu - ll) * 200));
	}

	m_LeBordervall->SetValue(ll);
	m_LeBordervalu->SetValue(uu);
	m_LeBordervallh->SetValue(int(0.4f + (ul - ll) / (uu - ll) * 100));
	m_LeBordervaluh->SetValue(int(0.4f + (lu - ll) / (uu - ll) * 100));
}

void HystereticGrowingWidget::BmpChanged()
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	Getrange();
}

void HystereticGrowingWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		m_Bmphand = m_Handler3D->GetActivebmphandler();
		Getrange();
	}
	Init1();
	HideParamsChanged();

	m_Thresholds[eSlider::kLower] = m_SlLower->Value();
	m_Thresholds[eSlider::kUpper] = m_SlUpper->Value();
	m_Thresholds[eSlider::kLowerHyst] = m_SlLowerhyster->Value();
	m_Thresholds[eSlider::kUpperHyst] = m_SlUpperhyster->Value();
}

void HystereticGrowingWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	Getrange();

	std::vector<std::vector<Point>>* vvp = m_Bmphand->ReturnLimits();
	m_Vp1.clear();
	for (auto it = vvp->begin(); it != vvp->end(); it++)
	{
		m_Vp1.insert(m_Vp1.end(), it->begin(), it->end());
	}
}

void HystereticGrowingWidget::Init1()
{
	m_Limitdrawing = false;
	m_Drawlimit->SetChecked(false);

	std::vector<std::vector<Point>>* vvp = m_Bmphand->ReturnLimits();
	m_Vp1.clear();
	for (auto it = vvp->begin(); it != vvp->end(); it++)
	{
		m_Vp1.insert(m_Vp1.end(), it->begin(), it->end());
	}

	emit Vp1Changed(&m_Vp1);
}

void HystereticGrowingWidget::Cleanup()
{
	m_Limitdrawing = false;
	m_Drawlimit->SetChecked(false);
	m_Vpdyn.clear();
	emit Vp1dynChanged(&m_Vpdyn, &m_Vpdyn);
}

void HystereticGrowingWidget::OnMouseMoved(Point p)
{
	if (m_Limitdrawing)
	{
		addLine(&m_Vpdyn, m_LastPt, p);
		m_LastPt = p;
		emit VpdynChanged(&m_Vpdyn);
	}
}

void HystereticGrowingWidget::OnMouseReleased(Point p)
{
	if (m_Limitdrawing)
	{
		addLine(&m_Vpdyn, m_LastPt, p);
		m_Limitdrawing = false;
		m_Vp1.insert(m_Vp1.end(), m_Vpdyn.begin(), m_Vpdyn.end());

		DataSelection data_selection;
		data_selection.limits = true;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		emit BeginDatachange(data_selection, this);

		m_Bmphand->AddLimit(&m_Vpdyn);

		emit EndDatachange(this);

		m_Vpdyn.clear();
		m_Drawlimit->SetChecked(false);
		emit Vp1dynChanged(&m_Vp1, &m_Vpdyn);
	}
}

void HystereticGrowingWidget::Limitpressed()
{
	m_Drawlimit->SetChecked(true);
	m_Limitdrawing = true;
	m_Vpdyn.clear();
	emit VpdynChanged(&m_Vpdyn);
}

void HystereticGrowingWidget::Clearpressed()
{
	m_Limitdrawing = false;
	m_Drawlimit->SetChecked(false);

	DataSelection data_selection;
	data_selection.limits = true;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	emit BeginDatachange(data_selection, this);

	m_Bmphand->ClearLimits();

	emit EndDatachange(this);

	m_Vpdyn.clear();
	m_Vp1.clear();
	emit Vp1dynChanged(&m_Vp1, &m_Vpdyn);
}

void HystereticGrowingWidget::SliderPressed()
{
	DataSelection data_selection;
	data_selection.work = data_selection.limits = true;
	data_selection.allSlices = m_Allslices->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	emit BeginDatachange(data_selection, this);
}

void HystereticGrowingWidget::SliderChanged(eSlider which, int new_value)
{
	// Execute1 gets the parameters from m_Thresholds. Note, the properties may not
	// be updated yet, since PropertySlider value is set when 'QSlider::sliderReleased' is triggered)
	m_Thresholds[which] = new_value;

	Execute1();
}

void HystereticGrowingWidget::SliderReleased()
{ 
	emit EndDatachange(this);
}

void HystereticGrowingWidget::LeBordervallReturnpressed()
{
	float val = m_LeBordervall->Value();
	if (val < m_LowerLimit)
	{
		m_SlLower->SetValue(0);
		m_LeBordervall->SetValue(m_LowerLimit);
	}
	else if (val > m_UpperLimit)
	{
		m_SlLower->SetValue(200);
		m_LeBordervall->SetValue(m_UpperLimit);
	}
	else
	{
		if (m_UpperLimit == m_LowerLimit)
		{
			m_SlLower->SetValue(100);
			m_LeBordervall->SetValue(m_UpperLimit);
		}
		else
			m_SlLower->SetValue(int(0.5f + (val - m_LowerLimit) / (m_UpperLimit - m_LowerLimit) * 200));
	}

	Execute();
}

void HystereticGrowingWidget::LeBordervaluReturnpressed()
{
	float val = m_LeBordervalu->Value();
	if (val < m_LowerLimit)
	{
		m_SlUpper->SetValue(0);
		m_LeBordervalu->SetValue(m_LowerLimit);
	}
	else if (val > m_UpperLimit)
	{
		m_SlUpper->SetValue(200);
		m_LeBordervalu->SetValue(m_UpperLimit);
	}
	else
	{
		if (m_UpperLimit == m_LowerLimit)
		{
			m_SlUpper->SetValue(100);
			m_LeBordervalu->SetValue(m_UpperLimit);
		}
		else
			m_SlUpper->SetValue(int(0.5f + (val - m_LowerLimit) /(m_UpperLimit - m_LowerLimit) * 200));
	}

	Execute();
}

void HystereticGrowingWidget::LeBordervallhReturnpressed()
{
	float val = m_LeBordervallh->Value();
	float ll = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->Value();
	float uu = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->Value();
	float ul = ll + (uu - ll) * 0.005f * m_SlLowerhyster->Value();

	if (val < 0)
	{
		ul = ll;
		m_SlLowerhyster->SetValue(0);
		m_LeBordervallh->SetValue(int((ul - ll) / (uu - ll) * 100));
	}
	else if (val > 100)
	{
		ul = uu;
		m_SlLowerhyster->SetValue(200);
		m_LeBordervallh->SetValue(int((ul - ll) / (uu - ll) * 100));
	}
	else
	{
		if (uu == ll)
		{
			ul = uu;
			m_SlLowerhyster->SetValue(100);
			m_LeBordervallh->SetValue(int((ul - ll) / (uu - ll) * 100));
		}
		else
			m_SlLowerhyster->SetValue(int(0.5f + val / 100 * 200));
	}

	Execute();
}

void HystereticGrowingWidget::LeBordervaluhReturnpressed()
{
	float val = m_LeBordervaluh->Value();
	float ll = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->Value();
	float uu = m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->Value();
	float lu = ll + (uu - ll) * 0.005f * m_SlUpperhyster->Value();

	if (val < 0)
	{
		lu = ll;
		m_SlUpperhyster->SetValue(0);
		m_LeBordervaluh->SetValue(int((lu - ll) / (uu - ll) * 100));
	}
	else if (val > 100)
	{
		lu = uu;
		m_SlUpperhyster->SetValue(200);
		m_LeBordervaluh->SetValue(int((lu - ll) / (uu - ll) * 100));
	}
	else
	{
		if (uu == ll)
		{
			lu = uu;
			m_SlUpperhyster->SetValue(100);
			m_LeBordervaluh->SetValue(int((lu - ll) / (uu - ll) * 100));
		}
		else
			m_SlUpperhyster->SetValue(int(0.5f + val / 100 * 200));
	}

	Execute();
}

FILE* HystereticGrowingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlLower->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlUpper->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlLowerhyster->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlUpperhyster->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Autoseed->Value());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Allslices->Value());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&m_UpperLimit, 1, sizeof(float), fp);
		fwrite(&m_LowerLimit, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* HystereticGrowingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		BlockPropertySignal autoseed_block(m_Autoseed);
		BlockPropertySignal lower_block(m_SlLower);
		BlockPropertySignal upper_block(m_SlUpper);
		BlockPropertySignal lowerhyst_block(m_SlLowerhyster);
		BlockPropertySignal upperhyst_block(m_SlUpperhyster);

		int lower, upper, lowerhyst, upperhyst, autoseed, all_slices;
		fread(&lower, sizeof(int), 1, fp);
		fread(&upper, sizeof(int), 1, fp);
		fread(&lowerhyst, sizeof(int), 1, fp);
		fread(&upperhyst, sizeof(int), 1, fp);
		fread(&autoseed, sizeof(int), 1, fp);
		fread(&all_slices, sizeof(int), 1, fp);
		fread(&m_UpperLimit, sizeof(float), 1, fp);
		fread(&m_LowerLimit, sizeof(float), 1, fp);

		m_SlLower->SetValue(lower);
		m_SlUpper->SetValue(upper);
		m_SlLowerhyster->SetValue(lowerhyst);
		m_SlUpperhyster->SetValue(upperhyst);
		m_Autoseed->SetValue(autoseed != 0);
		m_Allslices->SetValue(all_slices != 0);

		UpdateVisible();
	}
	return fp;
}

} // namespace iseg
