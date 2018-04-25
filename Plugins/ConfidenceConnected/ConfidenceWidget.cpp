/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "ConfidenceWidget.h"

#include "Data/ItkUtils.h"
#include "Data/SliceHandlerItkWrapper.h"

#include <itkConfidenceConnectedImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkCurvatureFlowImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkImage.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itkStatisticsImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itksys/SystemTools.hxx>

#include <QFormLayout>

#include <algorithm>
#include <sstream>

ConfidenceWidget::ConfidenceWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();

	header = new QLabel("Confidence Connected Algorithm: Pick at least one seed point");

	iterations = new QSpinBox(0, 50, 1, nullptr);
	iterations->setValue(1);

	multiplier = new QLineEdit(QString::number(2.5));
	multiplier->setValidator(new QDoubleValidator(1.0, 1e6, 2));

	radius = new QSpinBox(1, 10, 1, nullptr);
	radius->setValue(2);

	clear_seeds = new QPushButton("Clear seeds");
	execute_button = new QPushButton("Execute");

	auto layout = new QFormLayout;
	layout->addRow(header);
	layout->addRow(QString("Iterations"), iterations);
	layout->addRow(QString("Multiplier"), multiplier);
	layout->addRow(QString("Neighborhood radius"), radius);
	layout->addRow(clear_seeds);
	layout->addRow(execute_button);
	setLayout(layout);

	QObject::connect(clear_seeds, SIGNAL(clicked()), this, SLOT(clearmarks()));
	QObject::connect(execute_button, SIGNAL(clicked()), this, SLOT(do_work()));
}

void ConfidenceWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
}

void ConfidenceWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void ConfidenceWidget::newloaded()
{
	activeslice = handler3D->active_slice();
}

void ConfidenceWidget::clearmarks()
{
	vpdyn.clear();
}

void ConfidenceWidget::on_mouse_clicked(iseg::Point p)
{
	vpdyn.push_back(p);
}

void ConfidenceWidget::get_seeds(std::vector<itk::Index<2>>& seeds)
{
	for (auto p: vpdyn)
	{
		itk::Index<2> idx;
		idx[0] = p.px;
		idx[1] = p.py;
		seeds.push_back(idx);
	}
}

void ConfidenceWidget::get_seeds(std::vector<itk::Index<3>>& seeds)
{
	auto slice_idx = handler3D->active_slice();
	for (auto p: vpdyn)
	{
		itk::Index<3> idx;
		idx[0] = p.px;
		idx[1] = p.py;
		idx[2] = slice_idx;
		seeds.push_back(idx);
	}
}

void ConfidenceWidget::do_work()
{
	using input_type = itk::SliceContiguousImage<float>;
	using real_type = itk::Image<float,3>;
	using mask_type = itk::Image<unsigned char,3>;

	iseg::SliceHandlerItkWrapper itk_handler(handler3D);
	auto source = itk_handler.GetImage(iseg::SliceHandlerItkWrapper::kSource, true);
	auto smoothing = itk::CurvatureFlowImageFilter<input_type, real_type>::New();
	auto confidenceConnected = itk::ConfidenceConnectedImageFilter<real_type, mask_type>::New();

	// connect pipeline
	smoothing->SetInput(source);
	confidenceConnected->SetInput(smoothing->GetOutput());

	// set parameters
	smoothing->SetNumberOfIterations(2);
	smoothing->SetTimeStep(0.05);
	confidenceConnected->SetMultiplier(multiplier->text().toDouble());
	confidenceConnected->SetNumberOfIterations(iterations->value());
	confidenceConnected->SetInitialNeighborhoodRadius(radius->value());
	confidenceConnected->SetReplaceValue(255);

	std::vector<typename input_type::IndexType> seeds;
	get_seeds(seeds);
	for (auto idx: seeds)
	{
		confidenceConnected->AddSeed(idx);
	}

	try
	{
		confidenceConnected->Update();
	}
	catch(itk::ExceptionObject)
	{
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	auto target = itk_handler.GetImage(iseg::SliceHandlerItkWrapper::kTarget, true);
	iseg::Paste<mask_type, input_type>(confidenceConnected->GetOutput(), target);

	emit end_datachange(this);
}
