/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "ConfidenceWidget.h"

#include "Data/ItkUtils.h"
#include "Data/SlicesHandlerITKInterface.h"

#include <itkConfidenceConnectedImageFilter.h>
#include <itkCurvatureFlowImageFilter.h>
#include <itkImage.h>

#include <QFormLayout>

#include <algorithm>
#include <sstream>

ConfidenceWidget::ConfidenceWidget(iseg::SlicesHandlerInterface* hand3D)
		: m_Handler3D(hand3D)
{
	m_Activeslice = m_Handler3D->ActiveSlice();

	setToolTip(Format("Segment pixels with similar statistics using region growing. At least one seed point is needed."));

	m_AllSlices = new QCheckBox;

	m_Iterations = new QSpinBox;
	m_Iterations->setValue(1);
	m_Iterations->setMinimum(0);
	m_Iterations->setMaximum(50);
	m_Iterations->setToolTip(Format("Number of growing iterations (each time image statistics of current region are recomputed)."));

	m_Multiplier = new QLineEdit(QString::number(2.5));
	m_Multiplier->setValidator(new QDoubleValidator(1.0, 1e6, 2));
	m_Multiplier->setToolTip(Format("The confidence interval is the mean plus or minus the 'Multiplier' times the standard deviation."));

	m_Radius = new QSpinBox;
	m_Radius->setValue(2);
	m_Radius->setMinimum(1);
	m_Radius->setMaximum(10);
	m_Radius->setToolTip(Format("The neighborhood side of the initial seed points used to compute the image statistics."));

	m_ClearSeeds = new QPushButton("Clear seeds");
	m_ExecuteButton = new QPushButton("Execute");

	auto layout = new QFormLayout;
	layout->addRow(QString("Apply to all slices"), m_AllSlices);
	layout->addRow(QString("Iterations"), m_Iterations);
	layout->addRow(QString("Multiplier"), m_Multiplier);
	layout->addRow(QString("Neighborhood radius"), m_Radius);
	layout->addRow(m_ClearSeeds);
	layout->addRow(m_ExecuteButton);
	setLayout(layout);

	QObject_connect(m_ClearSeeds, SIGNAL(clicked()), this, SLOT(Clearmarks()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(DoWork()));
}

void ConfidenceWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();

	std::vector<iseg::Point> vp = m_Vpdyn[m_Activeslice];
	emit VpdynChanged(&vp);
}

void ConfidenceWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void ConfidenceWidget::NewLoaded()
{
	Clearmarks();
	OnSlicenrChanged();
}

void ConfidenceWidget::Cleanup()
{
	Clearmarks();
}

void ConfidenceWidget::Clearmarks()
{
	m_Vpdyn.clear();

	std::vector<iseg::Point> empty;
	emit VpdynChanged(&empty);
}

void ConfidenceWidget::OnMouseClicked(iseg::Point p)
{
	m_Vpdyn[m_Activeslice].push_back(p);

	if (!m_AllSlices->isChecked())
	{
		DoWork();
	}
}

void ConfidenceWidget::GetSeeds(std::vector<itk::Index<2>>& seeds)
{
	auto vp = m_Vpdyn[m_Activeslice];
	for (auto p : vp)
	{
		itk::Index<2> idx;
		idx[0] = p.px;
		idx[1] = p.py;
		seeds.push_back(idx);
	}
}

void ConfidenceWidget::GetSeeds(std::vector<itk::Index<3>>& seeds)
{
	auto start_slice = m_Handler3D->StartSlice();
	for (const auto& slice : m_Vpdyn)
	{
		for (auto p : slice.second)
		{
			itk::Index<3> idx;
			idx[0] = p.px;
			idx[1] = p.py;
			idx[2] = slice.first - start_slice;
			seeds.push_back(idx);
		}
	}
}

void ConfidenceWidget::DoWork()
{
	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	if (m_AllSlices->isChecked())
	{
		using input_type = itk::SliceContiguousImage<float>;
		auto source = itk_handler.GetSource(true);
		auto target = itk_handler.GetTarget(true);
		DoWorkNd<input_type>(source, target);
	}
	else
	{
		using input_type = itk::Image<float, 2>;
		auto source = itk_handler.GetSourceSlice();
		auto target = itk_handler.GetTargetSlice();
		DoWorkNd<input_type>(source, target);
	}
}

template<typename TInput>
void ConfidenceWidget::DoWorkNd(TInput* source, TInput* target)
{
	itkStaticConstMacro(ImageDimension, unsigned int, TInput::ImageDimension);
	using input_type = TInput;
	using real_type = itk::Image<float, ImageDimension>;
	using mask_type = itk::Image<unsigned char, ImageDimension>;

	auto smoothing = itk::CurvatureFlowImageFilter<input_type, real_type>::New();
	auto confidence_connected = itk::ConfidenceConnectedImageFilter<real_type, mask_type>::New();

	// connect pipeline
	smoothing->SetInput(source);
	confidence_connected->SetInput(smoothing->GetOutput());

	// set parameters
	smoothing->SetNumberOfIterations(2);
	smoothing->SetTimeStep(0.05);
	confidence_connected->SetMultiplier(m_Multiplier->text().toDouble());
	confidence_connected->SetNumberOfIterations(m_Iterations->value());
	confidence_connected->SetInitialNeighborhoodRadius(m_Radius->value());
	confidence_connected->SetReplaceValue(255);

	std::vector<typename input_type::IndexType> seeds;
	GetSeeds(seeds);
	for (auto idx : seeds)
	{
		confidence_connected->AddSeed(idx);
	}

	try
	{
		confidence_connected->Update();
	}
	catch (itk::ExceptionObject)
	{
		return;
	}

	iseg::DataSelection data_selection;
	data_selection.allSlices = m_AllSlices->isChecked();
	data_selection.sliceNr = m_Activeslice;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	iseg::Paste<mask_type, input_type>(confidence_connected->GetOutput(), target);

	emit EndDatachange(this);
}
