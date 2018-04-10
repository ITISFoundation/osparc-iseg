/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Levelset.h"
#include <algorithm>

#include <itkImage.h>

#include <itkBinaryThresholdImageFilter.h>
#include <itkFastMarchingImageFilter.h>
#include <itkThresholdSegmentationLevelSetImageFilter.h>

#include <sstream>

LevelsetWidget::LevelsetWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->get_activeslice();

	usp = NULL;

	vbox1 = new Q3VBox(this);
	bias_header = new QLabel(
			"LevelSetSegmentation: (Pick with OLC Foreground 1 pixel to start)",
			vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	hbox5 = new Q3HBox(vbox1);
	txt_h2 = new QLabel("Lower Threshold: ", hbox2);
	sl_h2 = new QSpinBox(-5000, 5000, 1, hbox2);
	sl_h2->setValue(0);
	txt_h3 = new QLabel("Upper Threshold ", hbox3);
	sl_h3 = new QSpinBox(-5000, 5000, 1, hbox3);
	sl_h3->setValue(0);
	txt_h4 = new QLabel("Number of Iterations at Maximum: ", hbox4);
	sl_h4 = new QSpinBox(0, 10000, 100, hbox4);
	sl_h4->setValue(1000);
	bias_exec = new QPushButton("LevelSetSegmentation Execute", vbox1);

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(bias_exec, SIGNAL(clicked()), this, SLOT(do_work()));

	return;
}

void LevelsetWidget::do_work()
{
	typedef itk::Image<float, 3> TInput;
	typedef itk::Image<float, 3> TMask;
	typedef TMask TOutput;
	typedef itk::ThresholdSegmentationLevelSetImageFilter<TInput, TInput>
			ThresholdSegmentationLevelSetImageFilterType;
	ThresholdSegmentationLevelSetImageFilterType::Pointer thresholdSegmentation;
	thresholdSegmentation = ThresholdSegmentationLevelSetImageFilterType::New();

	typedef itk::FastMarchingImageFilter<TInput, TInput> FastMarchingFilterType;
	FastMarchingFilterType::Pointer fastMarching;
	fastMarching = FastMarchingFilterType::New();

	typedef FastMarchingFilterType::NodeContainer NodeContainer;
	typedef FastMarchingFilterType::NodeType NodeType;
	NodeContainer::Pointer seeds;
	seeds = NodeContainer::New();

	TInput::IndexType seedPosition;
	handler3D->GetSeed(&seedPosition);

	TInput::Pointer input = TInput::New();
	handler3D->GetITKImage(input);
	input->Update();

	const double initialDistance = 2.0;
	NodeType node;
	const double seedValue = -initialDistance;
	node.SetValue(seedValue);
	node.SetIndex(seedPosition);
	seeds->Initialize();
	seeds->InsertElement(0, node);
	fastMarching->SetTrialPoints(seeds);
	fastMarching->SetSpeedConstant(1.0);
	fastMarching->SetOutputRegion(input->GetBufferedRegion());
	fastMarching->SetOutputSpacing(input->GetSpacing());
	fastMarching->SetOutputOrigin(input->GetOrigin());
	fastMarching->SetOutputDirection(input->GetDirection());

	typedef itk::BinaryThresholdImageFilter<TInput, TOutput>
			ThresholdingFilterType;
	ThresholdingFilterType::Pointer thresholder;
	thresholder = ThresholdingFilterType::New();
	thresholder->SetLowerThreshold(-5000.0);
	thresholder->SetUpperThreshold(0);
	thresholder->SetOutsideValue(0);
	thresholder->SetInsideValue(255);

	thresholdSegmentation->SetPropagationScaling(1.0);
	thresholdSegmentation->SetCurvatureScaling(1.0);
	thresholdSegmentation->SetMaximumRMSError(0.02);
	thresholdSegmentation->SetNumberOfIterations(sl_h4->value());
	thresholdSegmentation->SetUpperThreshold(sl_h3->value());
	thresholdSegmentation->SetLowerThreshold(sl_h2->value());
	thresholdSegmentation->SetIsoSurfaceValue(0.0);

	//Ensure that it is a 3D image for the 3D image filter ! Else it does nothing
	TOutput* output;
	thresholdSegmentation->SetInput(fastMarching->GetOutput());
	thresholdSegmentation->SetFeatureImage(input);
	thresholder->SetInput(thresholdSegmentation->GetOutput());
	output = thresholder->GetOutput();
	output->Update();
	handler3D->ModifyWorkFloat(output);
}

QSize LevelsetWidget::sizeHint() const { return vbox1->sizeHint(); }

LevelsetWidget::~LevelsetWidget()
{
	delete vbox1;

	free(usp);
}

void LevelsetWidget::on_slicenr_changed()
{
	activeslice = handler3D->get_activeslice();
}

void LevelsetWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void LevelsetWidget::newloaded()
{
	if (usp != NULL)
	{
		free(usp);
		usp = NULL;
	}

	activeslice = handler3D->get_activeslice();
}
