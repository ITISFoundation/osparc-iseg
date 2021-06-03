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
#include "SmoothingWidget.h"
#include "bmp_read_1.h"

#include "Interface/PropertyWidget.h"

#include <QBoxLayout>

#include <algorithm>
#include <array>

namespace iseg {

namespace {
float f1(float dI, float k) { return std::exp(-std::pow(dI / k, 2)); }

float f2(float dI, float k) { return 1 / (1 + std::pow(dI / k, 2)); }
} // namespace

SmoothingWidget::SmoothingWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Smoothing and noise removal filters."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	// properties
	auto group = PropertyGroup::Create("Smoothing Settings");

	m_Modegroup = group->Add("Method", PropertyEnum::Create({"Gaussian", "Average", "Median", "Sigma filter", "Anisotropic diffusion"}, kGaussian));
	m_Modegroup->SetDescription("Method");
	m_Modegroup->SetToolTip("Select the smoothing method.");

	m_Modegroup->SetItemToolTip(kGaussian, "Gaussian smoothing blurs the image and can remove noise and details (high frequency).");
	m_Modegroup->SetItemToolTip(kAverage, "Mean smoothing blurs the image and can remove noise and details.");
	m_Modegroup->SetItemToolTip(kMedian, "Median filtering can remove (speckle or salt-and-pepper) noise.");
	m_Modegroup->SetItemToolTip(kSigmafilter, "Sigma filtering is a mixture between Gaussian and Average filtering. It preserves edges better than Average filtering.");
	m_Modegroup->SetItemToolTip(kAnisodiff, "Anisotropic diffusion can remove noise, while preserving significant details, such as edges.");

	m_Allslices = group->Add("AllSlices", PropertyBool::Create(false));
	m_Allslices->SetDescription("Apply to all slices");

	m_SlSigma = group->Add("Sigma", PropertySlider::Create(20, 1, 100)); // left=0, right=5
	m_SlSigma->SetToolTip("Sigma gives the radius of the smoothing filter. Larger values remove more details.");

	m_SbN = group->Add("Width", PropertyInt::Create(5, 1, 11));
	m_SbN->SetToolTip("The width of the kernel in pixels.");

	m_SbIter = group->Add("Iterations", PropertyInt::Create(20, 1, 100));

	m_SlK = group->Add("Sigma", PropertySlider::Create(50, 0, 100));
	m_SlK->SetToolTip("Together with value on the right, defines the Sigma of the smoothing filter."); // TODO BL: replace by one meaningful setting

	m_SbKmax = group->Add("Sigma Max.", PropertyInt::Create(10, 1, 100));
	m_SbKmax->SetToolTip("Gives the max. radius of the smoothing filter. This value defines the scale used in the slider bar.");

	m_SlRestrain = group->Add("Restraint", PropertySlider::Create(0, 0, 100)); // 0-1

	m_Pushexec = group->Add("Execute", PropertyButton::Create([this]() { Execute(); }));
	m_Contdiff = group->Add("Cont. Diffusion", PropertyButton::Create([this]() { ContinueDiff(); }));

	// initialize state
	MethodChanged();

	// connections
	m_Modegroup->onModified.connect([this](Property_ptr, Property::eChangeType type) { 
		if (type == Property::kValueChanged)
			MethodChanged(); 
	});

	m_SlSigma->onPressed.connect([this](int) { SliderPressed(); });
	m_SlSigma->onMoved.connect([this](int v) { SigmasliderChanged(v); });

	m_SlK->onPressed.connect([this](int) { SliderPressed(); });
	m_SlK->onMoved.connect([this](int v) { KsliderChanged(v); });

	group->onChildModified.connect([this](Property_ptr prop, Property::eChangeType type) {
		if (type == Property::kValueChanged)
		{
			if (prop == m_SlSigma)
				SliderReleased();
			else if (prop == m_SlK)
				SliderReleased();
			else if (prop == m_SbKmax)
				KmaxChanged();
			else if (prop == m_SbN)
				NChanged();
		}
	});

	// add widget and layout
	auto property_view = new PropertyWidget(group);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view, 2);
	layout->addStretch(1);
	setLayout(layout);
}

SmoothingWidget::~SmoothingWidget() = default;

void SmoothingWidget::Execute()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->Value())
	{
		if (m_Modegroup->Value() == kGaussian)
		{
			m_Handler3D->Gaussian(m_SlSigma->Value() * 0.05f);
		}
		else if (m_Modegroup->Value() == kAverage)
		{
			m_Handler3D->Average((short unsigned)m_SbN->Value());
		}
		else if (m_Modegroup->Value() == kMedian)
		{
			m_Handler3D->MedianInterquartile(true);
		}
		else if (m_Modegroup->Value() == kSigmafilter)
		{
			m_Handler3D->Sigmafilter((m_SlK->Value() + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		}
		else
		{
			m_Handler3D->AnisoDiff(1.0f, m_SbIter->Value(), f2, m_SlK->Value() * 0.01f * m_SbKmax->Value(), m_SlRestrain->Value() * 0.01f);
		}
	}
	else // current slice
	{
		if (m_Modegroup->Value() == kGaussian)
		{
			m_Bmphand->Gaussian(m_SlSigma->Value() * 0.05f);
		}
		else if (m_Modegroup->Value() == kAverage)
		{
			m_Bmphand->Average((short unsigned)m_SbN->Value());
		}
		else if (m_Modegroup->Value() == kMedian)
		{
			m_Bmphand->MedianInterquartile(true);
		}
		else if (m_Modegroup->Value() == kSigmafilter)
		{
			m_Bmphand->Sigmafilter((m_SlK->Value() + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		}
		else
		{
			m_Bmphand->AnisoDiff(1.0f, m_SbIter->Value(), f2, m_SlK->Value() * 0.01f * m_SbKmax->Value(), m_SlRestrain->Value() * 0.01f);
		}
	}

	emit EndDatachange(this);
}

void SmoothingWidget::MethodChanged()
{
	m_SlSigma->SetVisible(m_Modegroup->Value() == kGaussian);
	m_SbN->SetVisible(m_Modegroup->Value() == kAverage || m_Modegroup->Value() == kSigmafilter);
	m_SlK->SetVisible(m_Modegroup->Value() == kSigmafilter || m_Modegroup->Value() == kAnisodiff);
	m_SbKmax->SetVisible(m_Modegroup->Value() == kSigmafilter || m_Modegroup->Value() == kAnisodiff);

	m_SbIter->SetVisible(m_Modegroup->Value() == kAnisodiff);
	m_SlRestrain->SetVisible(m_Modegroup->Value() == kAnisodiff);
	m_Contdiff->SetVisible(m_Modegroup->Value() == kAnisodiff);
}

void SmoothingWidget::SliderPressed()
{
	if ((m_Modegroup->Value() == kGaussian || m_Modegroup->Value() == kSigmafilter))
	{
		DataSelection data_selection;
		data_selection.allSlices = m_Allslices->Value();
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);
	}
}

void SmoothingWidget::SigmasliderChanged(int v)
{
	if (m_Modegroup->Value() == kGaussian)
	{
		if (m_Allslices->Value())
			m_Handler3D->Gaussian(v * 0.05f);
		else
			m_Bmphand->Gaussian(v * 0.05f);
		emit EndDatachange(this, iseg::NoUndo);
	}
}

void SmoothingWidget::SliderReleased()
{
	if (m_Modegroup->Value() == kGaussian || m_Modegroup->Value() == kSigmafilter)
	{
		emit EndDatachange(this);
	}
}

void SmoothingWidget::KsliderChanged(int v)
{
	if (m_Modegroup->Value() == kSigmafilter)
	{
		if (m_Allslices->Value())
			m_Handler3D->Sigmafilter((v + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		else
			m_Bmphand->Sigmafilter((v + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		emit EndDatachange(this, iseg::NoUndo);
	}
}

void SmoothingWidget::NChanged()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Modegroup->Value() == kSigmafilter)
	{
		if (m_Allslices->Value())
		{
			m_Handler3D->Sigmafilter((m_SlK->Value() + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		}
		else
		{
			m_Bmphand->Sigmafilter((m_SlK->Value() + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		}
	}
	else if (m_Modegroup->Value() == kAverage)
	{
		if (m_Allslices->Value())
		{
			m_Handler3D->Average((short unsigned)m_SbN->Value());
		}
		else
		{
			m_Bmphand->Average((short unsigned)m_SbN->Value());
		}
	}
	emit EndDatachange(this);
}

void SmoothingWidget::KmaxChanged()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Modegroup->Value() == kSigmafilter)
	{
		if (m_Allslices->Value())
		{
			m_Handler3D->Sigmafilter((m_SlK->Value() + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		}
		else
		{
			m_Bmphand->Sigmafilter((m_SlK->Value() + 1) * 0.01f * m_SbKmax->Value(), (short unsigned)m_SbN->Value(), (short unsigned)m_SbN->Value());
		}
	}
	emit EndDatachange(this);
}

void SmoothingWidget::ContinueDiff()
{
	if (m_Modegroup->Value() != kAnisodiff)
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->Value();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->Value())
	{
		m_Handler3D->ContAnisodiff(1.0f, m_SbIter->Value(), f2, m_SlK->Value() * 0.01f * m_SbKmax->Value(), m_SlRestrain->Value() * 0.01f);
	}
	else
	{
		m_Bmphand->ContAnisodiff(1.0f, m_SbIter->Value(), f2, m_SlK->Value() * 0.01f * m_SbKmax->Value(), m_SlRestrain->Value() * 0.01f);
	}

	emit EndDatachange(this);
}

void SmoothingWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void SmoothingWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
}

void SmoothingWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void SmoothingWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

FILE* SmoothingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlSigma->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlK->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlRestrain->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbN->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbIter->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbKmax->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kGaussian);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kAverage);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kMedian);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kSigmafilter);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kAnisodiff);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Allslices->Value());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* SmoothingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		BlockPropertySignal mode_guard(m_Modegroup);
		BlockPropertySignal sigma_guard(m_SlSigma);
		BlockPropertySignal k_guard(m_SlK);
		BlockPropertySignal b_guard(m_SbN);
		BlockPropertySignal kmax_guard(m_SbKmax);

		int sigma, k, restrain, n, iter, kmax, all_slices;
		std::array<int, keModeTypesSize> mode{};
		fread(&sigma, sizeof(int), 1, fp);
		fread(&k, sizeof(int), 1, fp);
		fread(&restrain, sizeof(int), 1, fp);
		fread(&n, sizeof(int), 1, fp);
		fread(&iter, sizeof(int), 1, fp);
		fread(&kmax, sizeof(int), 1, fp);
		fread(&mode[kGaussian], sizeof(int), 1, fp);
		fread(&mode[kAverage], sizeof(int), 1, fp);
		fread(&mode[kMedian], sizeof(int), 1, fp);
		fread(&mode[kSigmafilter], sizeof(int), 1, fp);
		fread(&mode[kAnisodiff], sizeof(int), 1, fp);
		fread(&all_slices, sizeof(int), 1, fp);

		m_SlSigma->SetValue(sigma);
		m_SlK->SetValue(k);
		m_SlRestrain->SetValue(restrain);
		m_SbN->SetValue(n);
		m_SbIter->SetValue(iter);
		m_SbKmax->SetValue(kmax);
		m_Allslices->SetValue(all_slices != 0);
		for (int i = 0; i < keModeTypesSize; ++i)
		{
			if (mode[i] != 0)
			{
				m_Modegroup->SetValue(i);
			}
		}

		MethodChanged();
	}
	return fp;
}

void SmoothingWidget::HideParamsChanged() { MethodChanged(); }

} // namespace iseg
