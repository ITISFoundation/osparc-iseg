/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "GraphCutTissueSeparator.h"

#include <algorithm>
#include <sstream>

#include "ImageGraphCut3DFilter.h"

#include <itkConnectedComponentImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkImage.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itksys/SystemTools.hxx>

CGraphCutTissueSeparator::CGraphCutTissueSeparator(
		iseg::CSliceHandlerInterface *hand3D, QWidget *parent, const char *name,
		Qt::WindowFlags wFlags)
		: QWidget1(parent, name, wFlags), m_Handler3D(hand3D)
{
	m_CurrentSlice = m_Handler3D->get_activeslice();

	m_VerticalGrid = new Q3VBox(this);

	m_Horizontal1 = new Q3HBox(m_VerticalGrid);
	m_LabelForeground = new QLabel("Foreground: ", m_Horizontal1);
	m_ForegroundValue = new QSpinBox(0, 500, 10, m_Horizontal1);
	m_ForegroundValue->setValue(400);

	m_Horizontal2 = new Q3HBox(m_VerticalGrid);
	m_LabelBackground = new QLabel("Background: ", m_Horizontal2);
	m_BackgroundValue = new QSpinBox(-500, 0, 10, m_Horizontal2);
	m_BackgroundValue->setValue(-60);

	USE_FB = new QCheckBox(QString("USE For&Background"), m_VerticalGrid);
	m_UseIntensity = new QCheckBox(QString("USE Intensity"), m_VerticalGrid);

	m_Horizontal3 = new Q3HBox(m_VerticalGrid);
	m_LabelMaxFlowAlgorithm = new QLabel("Max-Flow Algorithm: ", m_Horizontal3);
	m_MaxFlowAlgorithm = new QComboBox(m_Horizontal3);
	m_MaxFlowAlgorithm->insertItem(QString("Kohli"));
	m_MaxFlowAlgorithm->insertItem(QString("PushLabel-Fifo"));
	m_MaxFlowAlgorithm->insertItem(QString("PushLabel-H_PRF"));
	m_MaxFlowAlgorithm->setCurrentItem(1);

	m_6Connectivity = new QCheckBox(QString("6-Connectivity"), m_VerticalGrid);

	// TODO: this should re-use active-slices
	m_UseSliceRange = new QCheckBox(QString("Slice Range:"), m_VerticalGrid);

	m_Horizontal4 = new Q3HBox(m_VerticalGrid);
	m_LabelStart = new QLabel("Start-slice: ", m_Horizontal4);
	m_Start = new QSpinBox(1, 100000, 1, m_Horizontal4);
	m_Start->setValue(1);
	m_Start->setEnabled(false);

	m_Horizontal5 = new Q3HBox(m_VerticalGrid);
	m_LabelEnd = new QLabel("End-slice: ", m_Horizontal5);
	m_End = new QSpinBox(1, 100000, 1, m_Horizontal5);
	m_End->setValue(1);
	m_End->setEnabled(false);

	m_Execute = new QPushButton("Execute", m_VerticalGrid);

	// ?
	m_VerticalGrid->setFixedHeight(m_VerticalGrid->sizeHint().height());

	QObject::connect(m_Execute, SIGNAL(clicked()), this, SLOT(do_work()));
	QObject::connect(m_UseSliceRange, SIGNAL(clicked()), this,
									 SLOT(showsliders()));
	/*
	activeslice = handler3D->get_activeslice();

	usp = NULL;

	vbox1 = new Q3VBox(this);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);

	gc_header = new QLabel("Graph-Cut Algorithm: ", vbox1);
	txt_h2 = new QLabel("Foreground: ", hbox2);
	sl_h2 = new QSpinBox(0, 500, 10, hbox2);
	sl_h2->setValue(400);

	txt_h3 = new QLabel("Background: ", hbox3);
	sl_h3 = new QSpinBox(-500, 0, 10, hbox3);
	sl_h3->setValue(-60);

	txt_h4 = new QLabel("Graph-Algorithm: ", hbox4);
	select = new QSpinBox(1, 3, 1, hbox4);
	select->setValue(1);

	gc_exec2 = new QPushButton("GC Execute", vbox1);

	USE_FB = new QCheckBox(QString("USE For&Background"), vbox1);
	USE_In = new QCheckBox(QString("USE Intensity"), vbox1);
	LowCon = new QCheckBox(QString("Only_6_Connectivity"), vbox1);

	gc_divide = new QPushButton("GC Divide", vbox1);

	slice_range = new QCheckBox(QString("USE Range:"), vbox1);
	hbox5 = new Q3HBox(vbox1);
	hbox6 = new Q3HBox(vbox1);

	txt_h5 = new QLabel("Startslice: ", hbox5);
	startslice = new QSpinBox(1, 100000, 1, hbox5);
	startslice->setValue(1);

	txt_h6 = new QLabel("Endslice: ", hbox6);
	endslice = new QSpinBox(1, 100000, 1, hbox6);
	endslice->setValue(1);

	startslice->setEnabled(false);
	endslice->setEnabled(false);

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(gc_divide, SIGNAL(clicked()), this, SLOT(do_work()));
	QObject::connect(slice_range, SIGNAL(clicked()), this, SLOT(showsliders()));
	*/
}

void CGraphCutTissueSeparator::showsliders()
{
	if (m_UseSliceRange->isChecked() == true) {
		m_Start->setMaximum(m_Handler3D->return_endslice());
		m_Start->setEnabled(true);
		m_End->setMaximum(m_Handler3D->return_endslice());
		m_End->setValue(m_Handler3D->return_endslice());
		m_End->setEnabled(true);
	}
	else {
		m_Start->setEnabled(false);
		m_End->setEnabled(false);
	}
}

void CGraphCutTissueSeparator::do_work() //Code for special GC for division
{
	typedef itk::Image<float, 3> TInput;
	typedef itk::Image<float, 3> TMask;
	typedef TInput TForeground;
	typedef TInput TBackground;
	typedef TMask TOutput;
	typedef itk::Image<int, 3> TIntImage;
	typedef itk::ImageGraphCut3DFilter<TInput, TForeground, TBackground, TOutput>
			GraphCutFilterType;
	auto graphCutFilter = GraphCutFilterType::New();

	auto input = TInput::New();
	if (m_UseSliceRange->isChecked() == true)
		m_Handler3D->GetITKImage(input, m_Start->value() - 1, m_End->value());
	else
		m_Handler3D->GetITKImageGM(input);
	input->Update();
	TInput *foreground;
	TInput *background;
	foreground = input;
	background = input;
	if (USE_FB) // ?
	{
		if (m_UseSliceRange->isChecked() == true) {
			m_Handler3D->GetITKImageFB(foreground, m_Start->value() - 1,
																 m_End->value());
			m_Handler3D->GetITKImageFB(background, m_Start->value() - 1,
																 m_End->value());
		}
		else {
			m_Handler3D->GetITKImageFB(foreground);
			m_Handler3D->GetITKImageFB(background);
		}
	}

	graphCutFilter->SetInputImage(input);
	graphCutFilter->SetForegroundImage(foreground);
	graphCutFilter->SetBackgroundImage(background);
	// set parameters
	graphCutFilter->SetMaxFlowAlgorithm(
			static_cast<GraphCutFilterType::eMaxFlowAlgorithm>(
					m_MaxFlowAlgorithm->currentItem()));
	graphCutFilter->SetForegroundPixelValue(255);
	graphCutFilter->SetBackgroundPixelValue(-255);
	graphCutFilter->SetSigma(0.2);
	graphCutFilter->SetForeground(m_ForegroundValue->value());
	graphCutFilter->SetBackground(m_BackgroundValue->value());
	graphCutFilter->SetGM(true);
	graphCutFilter->SetFB(true);
	if (m_UseIntensity->isChecked())
		graphCutFilter->SetIntensity(true);
	if (m_6Connectivity->isChecked())
		graphCutFilter->SetConnectivity(true);

	if (input->GetLargestPossibleRegion().GetSize(2) > 1) {
		graphCutFilter->Update();
		TOutput *output;
		output = graphCutFilter->GetOutput();
		output->Update();
		itk::Size<3> rad;
		rad.Fill(1);

		typedef itk::ShapedNeighborhoodIterator<TOutput> IteratorType;
		IteratorType::OffsetType center = {{0, 0, 0}};
		typedef itk::ShapedNeighborhoodIterator<TInput> IteratorType2;
		IteratorType2::OffsetType center2 = {{0, 0, 0}};

		IteratorType iterator(rad, output, output->GetLargestPossibleRegion());
		IteratorType2 iterator2(rad, foreground,
														foreground->GetLargestPossibleRegion());
		iterator.ClearActiveList();
		iterator.ActivateOffset(center);
		iterator2.ClearActiveList();
		iterator2.ActivateOffset(center2);

		iterator2.GoToBegin();
		for (iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator) {
			bool pixelIsValid;
			TInput::PixelType centerPixel = iterator.GetPixel(center, pixelIsValid);
			if (!pixelIsValid) {
				continue;
			}
			TOutput::PixelType centerPixeloutput = iterator2.GetPixel(center2);
			if (centerPixeloutput == 0) {
				iterator.SetCenterPixel(0);
			}
			++iterator2;
		}

		output->Update();
		if (m_UseSliceRange->isChecked() == true)
			m_Handler3D->ModifyWorkFloat(output, m_Start->value() - 1,
																	 m_End->value());
		else
			m_Handler3D->ModifyWorkFloat(output);
	}
}

QSize CGraphCutTissueSeparator::sizeHint() const
{
	return m_VerticalGrid->sizeHint();
}

CGraphCutTissueSeparator::~CGraphCutTissueSeparator() { delete m_VerticalGrid; }

void CGraphCutTissueSeparator::slicenr_changed()
{
	m_CurrentSlice = m_Handler3D->get_activeslice();
}

void CGraphCutTissueSeparator::init()
{
	slicenr_changed();
	hideparams_changed();
}

void CGraphCutTissueSeparator::newloaded()
{
	m_CurrentSlice = m_Handler3D->get_activeslice();
}
