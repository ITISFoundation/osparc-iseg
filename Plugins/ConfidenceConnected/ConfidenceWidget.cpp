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

#include <algorithm>
#include <sstream>

ConfidenceWidget::ConfidenceWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();

	usp = NULL;

	vbox1 = new Q3VBox(this);
	bias_header = new QLabel("ConfidenceConnected Algorithm:(Pick with OLC "
													 "Foreground 1 pixel to start) ",
			vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	hbox5 = new Q3HBox(vbox1);
	txt_h2 = new QLabel("Iterations: ", hbox2);
	sl_h2 = new QSpinBox(0, 50, 1, hbox2);
	sl_h2->setValue(1);
	txt_h3 = new QLabel("Multiplier(*10-1) ", hbox3);
	sl_h3 = new QSpinBox(0, 100, 0, hbox3); // BL: todo this is wrong
	sl_h3->setValue(25);
	txt_h4 = new QLabel("Neighborhoodradius: ", hbox4);
	sl_h4 = new QSpinBox(0, 10, 1, hbox4);
	sl_h4->setValue(2);
	/*txt_h5=new QLabel("Seed: ",hbox5);
	sl_h5=new QSpinBox(0,1000,1,hbox5);
	sl_h5->setValue(1);
	sl_h6=new QSpinBox(0,1000,1,hbox5);
	sl_h6->setValue(1);
	sl_h7=new QSpinBox(0,1000,1,hbox5);
	sl_h7->setValue(1);*/
	bias_exec = new QPushButton("ConfidenceConnection Execute", vbox1);

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(bias_exec, SIGNAL(clicked()), this, SLOT(do_work()));

	return;
}

void ConfidenceWidget::do_work()
{
	typedef itk::Image<float, 3> TInput;
	typedef itk::Image<float, 3> TMask;
	typedef TMask TOutput;

	typedef itk::CurvatureFlowImageFilter<TInput, TInput>
			CurvatureFlowImageFilterType;
	CurvatureFlowImageFilterType::Pointer smoothing;
	smoothing = CurvatureFlowImageFilterType::New();
	typedef itk::ConfidenceConnectedImageFilter<TInput, TOutput>
			ConnectedFilterType;
	ConnectedFilterType::Pointer confidenceConnected;
	confidenceConnected = ConnectedFilterType::New();

	TInput::Pointer input = TInput::New();
	handler3D->GetITKImage(input);
	input->Update();
	smoothing->SetInput(input);
	confidenceConnected->SetInput(smoothing->GetOutput());
	TOutput* output;
	output = confidenceConnected->GetOutput();

	smoothing->SetNumberOfIterations(2);
	smoothing->SetTimeStep(0.05);
	confidenceConnected->SetMultiplier(sl_h3->value() / 10);
	confidenceConnected->SetNumberOfIterations(sl_h2->value());
	confidenceConnected->SetInitialNeighborhoodRadius(sl_h4->value());
	confidenceConnected->SetReplaceValue(255);

	TInput::IndexType index0;
	handler3D->GetSeed(&index0);
	confidenceConnected->AddSeed(index0);

	output->Update();
	handler3D->ModifyWorkFloat(output);
}

QSize ConfidenceWidget::sizeHint() const { return vbox1->sizeHint(); }

ConfidenceWidget::~ConfidenceWidget()
{
	delete vbox1;

	free(usp);
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
	if (usp != NULL)
	{
		free(usp);
		usp = NULL;
	}

	activeslice = handler3D->active_slice();
}
