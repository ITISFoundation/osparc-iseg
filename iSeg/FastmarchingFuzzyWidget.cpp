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

#include "FastmarchingFuzzyWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Core/ImageForestingTransform.h"

#include "Interface/LayoutTools.h"

#include <QFormLayout>
#include <QStackedLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>

namespace iseg {

namespace {
QWidget* add_to_widget(QLayout* layout)
{
	auto widget = new QWidget;
	widget->setLayout(layout);
	return widget;
}
} // namespace

FastmarchingFuzzyWidget::FastmarchingFuzzyWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("The Fuzzy tab actually provides access to two different "
										"segmentation techniques that have a very different "
										"background but a similar user and interaction interface : "
										"1) Fuzzy connectedness and 2) a Fast Marching "
										"implementation of a levelset technique. Both methods "
										"require a seed point to be specified."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Area = 0;

	m_SlSigma = new QSlider(Qt::Horizontal, nullptr);
	m_SlSigma->setRange(0, 100);
	m_SlSigma->setValue(int(m_Sigma / m_Sigmamax * 100));
	m_SlThresh = new QSlider(Qt::Horizontal, nullptr);
	m_SlThresh->setRange(0, 100);

	m_SlM1 = new QSlider(Qt::Horizontal, nullptr);
	m_SlM1->setRange(0, 100);
	m_SlM1->setToolTip(Format("The average gray value of the region to be segmented."));

	m_SlS1 = new QSlider(Qt::Horizontal, nullptr);
	m_SlS1->setRange(0, 100);
	m_SlS1->setToolTip(Format("A measure of how much the gray values are expected "
														"to deviate from m1 (standard deviation)."));

	m_SlS2 = new QSlider(Qt::Horizontal, nullptr);
	m_SlS2->setRange(0, 100);
	m_SlS2->setToolTip(Format("Sudden changes larger than s2 are considered to "
														"indicate boundaries."));

	m_RbFastmarch = new QRadioButton(tr("Fast Marching"));
	m_RbFastmarch->setToolTip(Format("Fuzzy connectedness computes for each image "
																	 "point the likelihood of its belonging to the same region as the "
																	 "starting "
																	 "point. It achieves this by looking at each possible connecting path "
																	 "between the two points and assigning the image point probability as "
																	 "identical to the probability of this path lying entirely in the same "
																	 "tissue "
																	 "as the start point."));
	m_RbFuzzy = new QRadioButton(tr("Fuzzy Connect."));
	m_RbFuzzy->setToolTip(Format("This tool simulates the evolution of a line (boundary) on a 2D "
															 "image in time. "
															 "The boundary is continuously expanding. The Sigma parameter "
															 "(Gaussian smoothing) controls the impact of noise."));

	auto bg_method = make_button_group(this, {m_RbFastmarch, m_RbFuzzy});
	m_RbFastmarch->setChecked(true);

	m_RbDrag = new QRadioButton(tr("Drag"));
	m_RbDrag->setToolTip(Format("Drag allows the user to drag the mouse (after clicking on "
															"the start point and keeping the mouse button pressed down) "
															"specifying the "
															"extent of the region on the image."));
	m_RbDrag->setChecked(true);
	m_RbDrag->show();
	m_RbSlider = new QRadioButton(tr("Slider"));
	m_RbSlider->setToolTip(Format("Increase or decrease the region size."));

	auto bg_interact = new QButtonGroup(this);
	bg_interact->insert(m_RbDrag);
	bg_interact->insert(m_RbSlider);

	m_SlExtend = new QSlider(Qt::Horizontal, nullptr);
	m_SlExtend->setRange(0, 200);
	m_SlExtend->setValue(0);

	m_SbThresh = new QSpinBox(10, 100, 10, nullptr);
	m_SbThresh->setValue(30);
	m_SbM1 = new QSpinBox(50, 1000, 50, nullptr);
	m_SbM1->setValue(200);
	m_SbS1 = new QSpinBox(50, 500, 50, nullptr);
	m_SbS1->setValue(100);
	m_SbS2 = new QSpinBox(10, 200, 10, nullptr);
	m_SbS2->setValue(100);

	// layout
	auto method_layout = new QVBoxLayout;
	method_layout->addWidget(m_RbFastmarch);
	method_layout->addWidget(m_RbFuzzy);

	auto method_area = new QFrame;
	method_area->setLayout(method_layout);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	auto fm_params = new QFormLayout;
	fm_params->addRow(tr("Sigma"), m_SlSigma);
	fm_params->addRow(tr("Thresh"), make_hbox({m_SlThresh, m_SbThresh}));

	auto fuzzy_params = new QFormLayout;
	fuzzy_params->addRow(tr("m1"), make_hbox({m_SlM1, m_SbM1}));
	fuzzy_params->addRow(tr("s1"), make_hbox({m_SlS1, m_SbS1}));
	fuzzy_params->addRow(tr("s2"), make_hbox({m_SlS2, m_SbS2}));

	m_ParamsStackLayout = new QStackedLayout;
	m_ParamsStackLayout->addWidget(add_to_widget(fm_params));
	m_ParamsStackLayout->addWidget(add_to_widget(fuzzy_params));

	auto interact_layout = new QVBoxLayout;
	interact_layout->addWidget(m_RbDrag);
	interact_layout->addLayout(make_hbox({m_RbSlider, m_SlExtend}));

	auto params_layout = new QVBoxLayout;
	params_layout->addLayout(m_ParamsStackLayout);
	params_layout->addLayout(interact_layout);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addLayout(params_layout);

	setLayout(top_layout);

	// connections
	QObject_connect(m_SlExtend, SIGNAL(valueChanged(int)), this, SLOT(SlextendChanged(int)));
	QObject_connect(m_SlExtend, SIGNAL(sliderPressed()), this, SLOT(SlextendPressed()));
	QObject_connect(m_SlExtend, SIGNAL(sliderReleased()), this, SLOT(SlextendReleased()));
	QObject_connect(m_SlSigma, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlThresh, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlM1, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlS1, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlS2, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));

	QObject_connect(bg_method, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));
	QObject_connect(bg_interact, SIGNAL(buttonClicked(int)), this, SLOT(InteractChanged()));

	QObject_connect(m_SbThresh, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
	QObject_connect(m_SbM1, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
	QObject_connect(m_SbS1, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
	QObject_connect(m_SbS2, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));

	m_IfTmarch = nullptr;
	m_IfTfuzzy = nullptr;
	m_Sigma = 1.0f;
	m_Sigmamax = 5.0f;
	m_Thresh = 100.0f;
	m_M1 = 110.0f;
	m_S1 = 20.0f;
	m_S2 = 50.0f;
	m_Extend = 0.5f;

	MethodChanged();
	SpinboxChanged();
	m_SlSigma->setValue(int(m_Sigma * 100 / m_Sigmamax));
}

FastmarchingFuzzyWidget::~FastmarchingFuzzyWidget()
{
	if (m_IfTmarch != nullptr)
		delete m_IfTmarch;
	if (m_IfTfuzzy != nullptr)
		delete m_IfTfuzzy;
}

void FastmarchingFuzzyWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void FastmarchingFuzzyWidget::BmphandChanged(Bmphandler* bmph)
{
	if (m_IfTmarch != nullptr)
		delete m_IfTmarch;
	if (m_IfTfuzzy != nullptr)
		delete m_IfTfuzzy;
	m_IfTmarch = nullptr;
	m_IfTfuzzy = nullptr;

	m_Bmphand = bmph;

	m_SlExtend->setEnabled(false);
}

void FastmarchingFuzzyWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		m_Bmphand = m_Handler3D->GetActivebmphandler();
	}

	m_Area = m_Bmphand->ReturnHeight() * (unsigned)m_Bmphand->ReturnWidth();

	if (m_IfTmarch != nullptr)
		delete m_IfTmarch;
	if (m_IfTfuzzy != nullptr)
		delete m_IfTfuzzy;
	m_IfTmarch = nullptr;
	m_IfTfuzzy = nullptr;

	m_SlExtend->setEnabled(false);

	HideParamsChanged();
}

void FastmarchingFuzzyWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void FastmarchingFuzzyWidget::Cleanup()
{
	if (m_IfTmarch != nullptr)
		delete m_IfTmarch;
	if (m_IfTfuzzy != nullptr)
		delete m_IfTfuzzy;
	m_IfTmarch = nullptr;
	m_IfTfuzzy = nullptr;
	m_SlExtend->setEnabled(false);
}

void FastmarchingFuzzyWidget::Getrange()
{
	m_Extendmax = 0;
	for (unsigned i = 0; i < m_Area; i++)
		if (m_Extendmax < m_Map[i])
			m_Extendmax = m_Map[i];
}

void FastmarchingFuzzyWidget::OnMouseClicked(Point p)
{
	if (m_RbFastmarch->isChecked())
	{
		if (m_IfTmarch != nullptr)
			delete m_IfTmarch;

		m_IfTmarch = m_Bmphand->FastmarchingInit(p, m_Sigma, m_Thresh);

		//		if(map!=nullptr) free(map);
		m_Map = m_IfTmarch->ReturnPf();
	}
	else
	{
		if (m_IfTfuzzy == nullptr)
		{
			m_IfTfuzzy = new ImageForestingTransformAdaptFuzzy;
			m_IfTfuzzy->FuzzyInit(m_Bmphand->ReturnWidth(), m_Bmphand->ReturnHeight(), m_Bmphand->ReturnBmp(), p, m_M1, m_S1, m_S2);
		}
		else
		{
			m_IfTfuzzy->ChangeParam(m_M1, m_S1, m_S2);
			m_IfTfuzzy->ChangePt(p);
		}

		//		if(map!=nullptr) free(map);
		m_Map = m_IfTfuzzy->ReturnPf();
	}

	if (m_RbSlider->isChecked())
	{
		Getrange();
		if (m_Extend > m_Extendmax)
		{
			m_Extend = m_Extendmax;
		}

		m_SlExtend->setValue(int(m_Extend / m_Extendmax * 200));
		m_SlExtend->setEnabled(true);

		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);
		Execute();
		emit EndDatachange(this);
	}
}

void FastmarchingFuzzyWidget::OnMouseReleased(Point p)
{
	if (m_RbDrag->isChecked())
	{
		m_VpdynArg.clear();
		emit VpdynChanged(&m_VpdynArg);
		m_Extend = m_Map[unsigned(m_Bmphand->ReturnWidth()) * p.py + p.px];

		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);
		Execute();
		emit EndDatachange(this);
		/*		float *workbits=bmphand->return_work();
		for(unsigned i=0;i<area;i++) {
			if(map[i]<extend) workbits=255.0f;
			else workbits=0.0f;
		}*/
	}
}

void FastmarchingFuzzyWidget::OnMouseMoved(Point p)
{
	if (m_RbDrag->isChecked())
	{
		m_VpdynArg.clear();
		unsigned short width = m_Bmphand->ReturnWidth();
		unsigned short height = m_Bmphand->ReturnHeight();
		m_Extend = m_Map[(unsigned)width * p.py + p.px];

		Point p;

		unsigned pos = 0;

		if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
				(m_Map[pos] <= m_Extend && m_Map[pos + width] > m_Extend))
		{
			p.px = 0;
			p.py = 0;
			m_VpdynArg.push_back(p);
		}
		pos++;
		for (unsigned short j = 1; j + 1 < width; j++)
		{
			if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos - 1] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos + width] > m_Extend))
			{
				p.px = j;
				p.py = 0;
				m_VpdynArg.push_back(p);
			}
			pos++;
		}
		if ((m_Map[pos] <= m_Extend && m_Map[pos - 1] > m_Extend) ||
				(m_Map[pos] <= m_Extend && m_Map[pos + width] > m_Extend))
		{
			p.px = width - 1;
			p.py = 0;
			m_VpdynArg.push_back(p);
		}
		pos++;

		for (unsigned short i = 1; i + 1 < height; i++)
		{
			if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos + width] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos - width] > m_Extend))
			{
				p.px = 0;
				p.py = i;
				m_VpdynArg.push_back(p);
			}
			pos++;
			for (unsigned short j = 1; j + 1 < width; j++)
			{
				if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
						(m_Map[pos] <= m_Extend && m_Map[pos - 1] > m_Extend) ||
						(m_Map[pos] <= m_Extend && m_Map[pos + width] > m_Extend) ||
						(m_Map[pos] <= m_Extend && m_Map[pos - width] > m_Extend))
				{
					p.px = j;
					p.py = i;
					m_VpdynArg.push_back(p);
				}
				pos++;
			}
			if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos + width] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos - width] > m_Extend))
			{
				p.px = width - 1;
				p.py = i;
				m_VpdynArg.push_back(p);
			}
			pos++;
		}
		if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
				(m_Map[pos] <= m_Extend && m_Map[pos - width] > m_Extend))
		{
			p.px = 0;
			p.py = height - 1;
			m_VpdynArg.push_back(p);
		}
		pos++;
		for (unsigned short j = 1; j + 1 < width; j++)
		{
			if ((m_Map[pos] <= m_Extend && m_Map[pos + 1] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos - 1] > m_Extend) ||
					(m_Map[pos] <= m_Extend && m_Map[pos - width] > m_Extend))
			{
				p.px = j;
				p.py = height - 1;
				m_VpdynArg.push_back(p);
			}
			pos++;
		}
		if ((m_Map[pos] <= m_Extend && m_Map[pos - 1] > m_Extend) ||
				(m_Map[pos] <= m_Extend && m_Map[pos - width] > m_Extend))
		{
			p.px = width - 1;
			p.py = height - 1;
			m_VpdynArg.push_back(p);
		}

		emit VpdynChanged(&m_VpdynArg);
		//		Execute();
		/*		float *workbits=bmphand->return_work();
		for(unsigned i=0;i<area;i++) {
			if(map[i]<extend) workbits=255.0f;
			else workbits=0.0f;
		}*/
	}
}

void FastmarchingFuzzyWidget::Execute()
{
	float* workbits = m_Bmphand->ReturnWork();
	for (unsigned i = 0; i < m_Area; i++)
	{
		//		workbits[i]=map[i];
		if (m_Map[i] <= m_Extend)
			workbits[i] = 255.0f;
		else
			workbits[i] = 0.0f;
	}
	m_Bmphand->SetMode(2, false);
}

void FastmarchingFuzzyWidget::SlextendChanged(int val)
{
	m_Extend = val * 0.005f * m_Extendmax;
	if (m_RbSlider->isChecked())
		Execute();
}

void FastmarchingFuzzyWidget::BmpChanged()
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	//	sl_extend->setEnabled(false);
	Init();
}

void FastmarchingFuzzyWidget::MethodChanged()
{
	if (m_RbFastmarch->isChecked())
	{
		m_ParamsStackLayout->setCurrentIndex(0);
	}
	else
	{
		m_ParamsStackLayout->setCurrentIndex(1);
	}
}

void FastmarchingFuzzyWidget::InteractChanged()
{
	if (m_RbDrag->isChecked())
	{
		m_SlExtend->setVisible(false);
	}
	else
	{
		m_SlExtend->setVisible(true);
	}
}

void FastmarchingFuzzyWidget::SpinboxChanged()
{
	if (m_Thresh > m_SbThresh->value())
	{
		m_Thresh = m_SbThresh->value();
		m_SlThresh->setValue(100);
		if (m_IfTmarch != nullptr)
		{
			delete m_IfTmarch;
			m_IfTmarch = nullptr;
		}
		m_SlExtend->setEnabled(false);
	}
	else
	{
		m_SlThresh->setValue(int(m_Thresh * 100 / m_SbThresh->value()));
	}

	if (m_M1 > m_SbM1->value())
	{
		m_M1 = m_SbM1->value();
		m_SlM1->setValue(100);
		m_SlExtend->setEnabled(false);
	}
	else
	{
		m_SlM1->setValue(int(m_M1 * 100 / m_SbM1->value()));
	}

	if (m_S1 > m_SbS1->value())
	{
		m_S1 = m_SbS1->value();
		m_SlS1->setValue(100);
		m_SlExtend->setEnabled(false);
	}
	else
	{
		m_SlS1->setValue(int(m_S1 * 100 / m_SbS1->value()));
	}

	if (m_S2 > m_SbS2->value())
	{
		m_S2 = m_SbS2->value();
		m_SlS2->setValue(100);
		m_SlExtend->setEnabled(false);
	}
	else
	{
		m_SlS2->setValue(int(m_S2 * 100 / m_SbS2->value()));
	}

	}

void FastmarchingFuzzyWidget::SliderChanged()
{
	m_Sigma = m_SlSigma->value() * 0.01f * m_Sigmamax;
	m_Thresh = m_SlThresh->value() * 0.01f * m_SbThresh->value();
	m_M1 = m_SlM1->value() * 0.01f * m_SbM1->value();
	m_S1 = m_SlS1->value() * 0.01f * m_SbS1->value();
	m_S2 = m_SlS2->value() * 0.01f * m_SbS2->value();

	if (m_RbFastmarch->isChecked() && m_IfTmarch != nullptr)
	{
		delete m_IfTmarch;
		m_IfTmarch = nullptr;
	}

	m_SlExtend->setEnabled(false);
}

void FastmarchingFuzzyWidget::SlextendPressed()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);
}

void FastmarchingFuzzyWidget::SlextendReleased() { emit EndDatachange(this); }

FILE* FastmarchingFuzzyWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlSigma->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlThresh->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlM1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlS1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlS2->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlExtend->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbThresh->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbM1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbS1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbS2->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbFastmarch->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbFuzzy->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbDrag->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbSlider->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&m_Sigma, 1, sizeof(float), fp);
		fwrite(&m_Thresh, 1, sizeof(float), fp);
		fwrite(&m_M1, 1, sizeof(float), fp);
		fwrite(&m_S1, 1, sizeof(float), fp);
		fwrite(&m_S2, 1, sizeof(float), fp);
		fwrite(&m_Sigmamax, 1, sizeof(float), fp);
		fwrite(&m_Extend, 1, sizeof(float), fp);
		fwrite(&m_Extendmax, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* FastmarchingFuzzyWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_SlExtend, SIGNAL(valueChanged(int)), this, SLOT(SlextendChanged(int)));
		QObject_disconnect(m_SbThresh, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
		QObject_disconnect(m_SbM1, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
		QObject_disconnect(m_SbS1, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
		QObject_disconnect(m_SbS2, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SlSigma->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlThresh->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlM1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlS1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlS2->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlExtend->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbThresh->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbM1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbS1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbS2->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbFastmarch->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbFuzzy->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbDrag->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbSlider->setChecked(dummy > 0);

		fread(&m_Sigma, sizeof(float), 1, fp);
		fread(&m_Thresh, sizeof(float), 1, fp);
		fread(&m_M1, sizeof(float), 1, fp);
		fread(&m_S1, sizeof(float), 1, fp);
		fread(&m_S2, sizeof(float), 1, fp);
		fread(&m_Sigmamax, sizeof(float), 1, fp);
		fread(&m_Extend, sizeof(float), 1, fp);
		fread(&m_Extendmax, sizeof(float), 1, fp);

		MethodChanged();
		InteractChanged();

		QObject_connect(m_SlExtend, SIGNAL(valueChanged(int)), this, SLOT(SlextendChanged(int)));
		QObject_connect(m_SbThresh, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
		QObject_connect(m_SbM1, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
		QObject_connect(m_SbS1, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
		QObject_connect(m_SbS2, SIGNAL(valueChanged(int)), this, SLOT(SpinboxChanged()));
	}
	return fp;
}

void FastmarchingFuzzyWidget::HideParamsChanged()
{
	MethodChanged();
	InteractChanged();
}

} // namespace iseg
