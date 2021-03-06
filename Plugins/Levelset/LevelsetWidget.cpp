/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "LevelsetWidget.h"

#include "Data/ItkUtils.h"
#include "Data/Logger.h"
#include "Data/SlicesHandlerITKInterface.h"

#include <itkApproximateSignedDistanceMapImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConstNeighborhoodIterator.h>
#include <itkFastMarchingImageFilter.h>
#include <itkImage.h>
#include <itkThresholdSegmentationLevelSetImageFilter.h>

#include <QFormLayout>

#include <accumulators/percentile.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include <algorithm>
#include <sstream>

namespace acc = boost::accumulators;

LevelsetWidget::LevelsetWidget(iseg::SlicesHandlerInterface* hand3D)
		: m_Handler3D(hand3D)
{
	m_Activeslice = m_Handler3D->ActiveSlice();

	setToolTip(Format("LevelSetSegmentation: (Pick with OLC Foreground 1 pixel to start)"));

	m_AllSlices = new QCheckBox;

	m_InitFromTarget = new QCheckBox;

	m_CurvatureScaling = new QLineEdit(QString::number(1.0));
	m_CurvatureScaling->setValidator(new QDoubleValidator);

	m_LowerThreshold = new QLineEdit(QString::number(0.0));
	m_LowerThreshold->setValidator(new QDoubleValidator);

	m_UpperThreshold = new QLineEdit(QString::number(1.0));
	m_UpperThreshold->setValidator(new QDoubleValidator);

	m_EdgeWeight = new QLineEdit(QString::number(0.0));
	m_EdgeWeight->setValidator(new QDoubleValidator);

	m_Multiplier = new QLineEdit(QString::number(2.5));
	m_Multiplier->setValidator(new QDoubleValidator);
	m_Multiplier->setToolTip(Format(
			"Used to estimate thresholds."
			"The confidence interval is the mean plus or minus the 'Multiplier' times the standard deviation."));

	m_GuessThreshold = new QPushButton("Estimate thresholds");

	m_ClearSeeds = new QPushButton("Clear seeds");

	m_ExecuteButton = new QPushButton("Execute");

	auto layout = new QFormLayout;
	layout->addRow(QString("Apply to all slices"), m_AllSlices);

	layout->addRow(QString("Initialize from target"), m_InitFromTarget);
	layout->addRow(QString("Curvature scaling"), m_CurvatureScaling);
	layout->addRow(QString("Lower threshold"), m_LowerThreshold);
	layout->addRow(QString("Upper threshold"), m_UpperThreshold);
	layout->addRow(QString("Edge weight"), m_EdgeWeight);
	layout->addRow(QString("Multiplier"), m_Multiplier);
	layout->addRow(m_GuessThreshold, m_ClearSeeds);
	layout->addRow(m_ExecuteButton);
	setLayout(layout);

	QObject_connect(m_GuessThreshold, SIGNAL(clicked()), this, SLOT(GuessThresholds()));
	QObject_connect(m_ClearSeeds, SIGNAL(clicked()), this, SLOT(Clearmarks()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(DoWork()));
}

void LevelsetWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void LevelsetWidget::NewLoaded()
{
	Clearmarks();
	OnSlicenrChanged();
}

void LevelsetWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
}

void LevelsetWidget::Cleanup()
{
	Clearmarks();
}

void LevelsetWidget::Clearmarks()
{
	m_Vpdyn.clear();

	std::vector<iseg::Point> empty;
	emit VpdynChanged(&empty);
}

void LevelsetWidget::OnMouseClicked(iseg::Point p)
{
	m_Vpdyn[m_Activeslice].push_back(p);

	if (!m_AllSlices->isChecked())
	{
		//DoWork();
	}
}

void LevelsetWidget::GetSeeds(std::vector<itk::Index<2>>& seeds)
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

void LevelsetWidget::GetSeeds(std::vector<itk::Index<3>>& seeds)
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

void LevelsetWidget::GuessThresholds()
{
	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	if (m_AllSlices->isChecked())
	{
		using input_type = itk::SliceContiguousImage<float>;
		auto source = itk_handler.GetSource(true);
		GuessThresholdsNd<input_type>(source);
	}
	else
	{
		using input_type = itk::Image<float, 2>;
		auto source = itk_handler.GetSourceSlice();
		GuessThresholdsNd<input_type>(source);
	}
}

template<class TInput>
void LevelsetWidget::GuessThresholdsNd(TInput* source)
{
	using input_type = TInput;

	std::vector<typename input_type::IndexType> indices;
	GetSeeds(indices);

	typename input_type::SizeType radius;
	radius.Fill(2);

	acc::accumulator_set<double, acc::features<acc::tag::mean, acc::tag::variance>> stats;

	itk::ConstNeighborhoodIterator<input_type> it(radius, source, source->GetLargestPossibleRegion());
	size_t const n = it.Size();
	for (auto idx : indices)
	{
		it.SetLocation(idx);

		for (size_t i = 0; i < n; i++)
		{
			bool in_bounds;
			double v = it.GetPixel(i, in_bounds);
			if (in_bounds)
			{
				stats(v);
			}
		}
	}

	auto mean = acc::mean(stats);
	auto stddev = std::sqrt(acc::variance(stats));
	auto s = m_Multiplier->text().toDouble();
	m_LowerThreshold->setText(QString::number(mean - s * stddev));
	m_UpperThreshold->setText(QString::number(mean + s * stddev));
}

void LevelsetWidget::DoWork()
{
	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	if (m_AllSlices->isChecked())
	{
		using input_type = itk::SliceContiguousImage<float>;
		auto source = itk_handler.GetSource(true); // active_slices -> correct seed z-position
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
void LevelsetWidget::DoWorkNd(TInput* input, TInput* target)
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using input_type = TInput;
	using real_type = itk::Image<float, ImageDimension>;
	using mask_type = itk::Image<unsigned char, ImageDimension>;

	using fast_marching_type = itk::FastMarchingImageFilter<real_type, real_type>;
	using node_container_type = typename fast_marching_type::NodeContainer;
	using node_type = typename fast_marching_type::NodeType;

	// create filters
	auto fast_marching = fast_marching_type::New();
	auto threshold_levelset = itk::ThresholdSegmentationLevelSetImageFilter<real_type, input_type>::New();
	auto threshold = itk::BinaryThresholdImageFilter<real_type, mask_type>::New();

	// initialize levelset
	typename real_type::Pointer initial_levelset;
	if (m_InitFromTarget->isChecked())
	{
		// threshold target -> mask
		auto threshold_target = itk::BinaryThresholdImageFilter<input_type, real_type>::New();
		threshold_target->SetInput(target);
		threshold_target->SetLowerThreshold(0.001f);
		threshold_target->SetInsideValue(-0.5f); // level set filter uses iso-value 0.0
		threshold_target->SetOutsideValue(0.5f);
		threshold_target->Update();
		initial_levelset = threshold_target->GetOutput();
	}
	else
	{
		// setup seeds
		std::vector<typename input_type::IndexType> indices;
		GetSeeds(indices);

		const double initial_distance = 2.0;				 // \todo BL
		const double seed_value = -initial_distance; // \todo BL
		auto seeds = node_container_type::New();
		seeds->Initialize();
		for (size_t i = 0; i < indices.size(); ++i)
		{
			node_type node;
			node.SetValue(seed_value);
			node.SetIndex(indices[i]);
			seeds->InsertElement(i, node);
		}

		fast_marching->SetTrialPoints(seeds);
		fast_marching->SetSpeedConstant(1.0);
		fast_marching->SetOutputRegion(input->GetBufferedRegion());
		fast_marching->SetOutputSpacing(input->GetSpacing());
		fast_marching->SetOutputOrigin(input->GetOrigin());
		fast_marching->SetOutputDirection(input->GetDirection());
		initial_levelset = fast_marching->GetOutput();
	}

	// set parameters
	threshold_levelset->SetInput(initial_levelset);
	threshold_levelset->SetFeatureImage(input);
	threshold_levelset->SetPropagationScaling(1.0);
	threshold_levelset->SetCurvatureScaling(m_CurvatureScaling->text().toDouble());
	threshold_levelset->SetEdgeWeight(m_EdgeWeight->text().toDouble());
	threshold_levelset->SetLowerThreshold(m_LowerThreshold->text().toDouble());
	threshold_levelset->SetUpperThreshold(m_UpperThreshold->text().toDouble());
	threshold_levelset->SetMaximumRMSError(0.02);
	threshold_levelset->SetNumberOfIterations(1200);
	threshold_levelset->SetIsoSurfaceValue(0.0);

	threshold->SetInput(threshold_levelset->GetOutput());
	threshold->SetLowerThreshold(std::numeric_limits<float>::lowest());
	threshold->SetUpperThreshold(0);
	threshold->SetOutsideValue(0);
	threshold->SetInsideValue(255);

	try
	{
		threshold->Update();
		std::cerr << "Finished levelset segmentation\n";
	}
	catch (itk::ExceptionObject e)
	{
		ISEG_ERROR_MSG(e.what());
		return;
	}

	iseg::DataSelection data_selection;
	data_selection.allSlices = m_AllSlices->isChecked();
	data_selection.sliceNr = m_Activeslice;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (!iseg::Paste<mask_type, input_type>(threshold->GetOutput(), target))
	{
		ISEG_ERROR_MSG("could not set output because image regions don't match.");
	}

	emit EndDatachange(this);
}
