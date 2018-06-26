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
#include <itkCurvatureFlowImageFilter.h>
#include <itkImage.h>

#include <QFormLayout>

#include <algorithm>
#include <sstream>

ConfidenceWidget::ConfidenceWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();

	setToolTip(Format(
			"Segment pixels with similar statistics using region growing. At least one seed point is needed."));

	all_slices = new QCheckBox;

	iterations = new QSpinBox(0, 50, 1, nullptr);
	iterations->setValue(1);
	iterations->setToolTip(Format("Number of growing iterations (each time image statistics of current region are recomputed)."));

	multiplier = new QLineEdit(QString::number(2.5));
	multiplier->setValidator(new QDoubleValidator(1.0, 1e6, 2));
	multiplier->setToolTip(Format("The confidence interval is the mean plus or minus the 'Multiplier' times the standard deviation."));

	radius = new QSpinBox(1, 10, 1, nullptr);
	radius->setValue(2);
	radius->setToolTip(Format("The neighborhood side of the initial seed points used to compute the image statistics."));

	clear_seeds = new QPushButton("Clear seeds");
	execute_button = new QPushButton("Execute");

	auto layout = new QFormLayout;
	layout->addRow(QString("Apply to all slices"), all_slices);
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

	std::vector<iseg::Point> vp = vpdyn[activeslice];
	emit vpdyn_changed(&vp);
}

void ConfidenceWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void ConfidenceWidget::newloaded()
{
	clearmarks();
	on_slicenr_changed();
}

void ConfidenceWidget::cleanup()
{
	clearmarks();
}

void ConfidenceWidget::clearmarks()
{
	vpdyn.clear();

	std::vector<iseg::Point> empty;
	emit vpdyn_changed(&empty);
}

void ConfidenceWidget::on_mouse_clicked(iseg::Point p)
{
	vpdyn[activeslice].push_back(p);

	if (!all_slices->isChecked())
	{
		do_work();
	}
}

void ConfidenceWidget::get_seeds(std::vector<itk::Index<2>>& seeds)
{
	auto vp = vpdyn[activeslice];
	for (auto p : vp)
	{
		itk::Index<2> idx;
		idx[0] = p.px;
		idx[1] = p.py;
		seeds.push_back(idx);
	}
}

void ConfidenceWidget::get_seeds(std::vector<itk::Index<3>>& seeds)
{
	auto start_slice = handler3D->start_slice();
	for (auto slice : vpdyn)
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

void ConfidenceWidget::do_work()
{
	iseg::SliceHandlerItkWrapper itk_handler(handler3D);
	if (all_slices->isChecked())
	{
		using input_type = itk::SliceContiguousImage<float>;
		auto source = itk_handler.GetSource(true);
		auto target = itk_handler.GetTarget(true);
		do_work_nd<input_type>(source, target);
	}
	else
	{
		using input_type = itk::Image<float, 2>;
		auto source = itk_handler.GetSourceSlice();
		auto target = itk_handler.GetTargetSlice();
		do_work_nd<input_type>(source, target);
	}
}

template<typename TInput>
void ConfidenceWidget::do_work_nd(TInput* source, TInput* target)
{
	itkStaticConstMacro(ImageDimension, unsigned int, TInput::ImageDimension);
	using input_type = TInput;
	using real_type = itk::Image<float, ImageDimension>;
	using mask_type = itk::Image<unsigned char, ImageDimension>;

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
	for (auto idx : seeds)
	{
		confidenceConnected->AddSeed(idx);
	}

	try
	{
		confidenceConnected->Update();
	}
	catch (itk::ExceptionObject)
	{
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = all_slices->isChecked();
	dataSelection.sliceNr = activeslice;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	iseg::Paste<mask_type, input_type>(confidenceConnected->GetOutput(), target);

	emit end_datachange(this);
}
