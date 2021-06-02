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
#include "WatershedWidget.h"
#include "bmp_read_1.h"

#include <QFormLayout>
#include <QHBoxLayout>

namespace iseg {

WatershedWidget::WatershedWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Segment a tissue (2D) by selecting points in the current slice "
										"based on the Watershed method."
										"<br>"
										"The method: the gradient of the slightly smoothed image is "
										"calculated. High values are interpreted as mountains and "
										"low values as valleys. Subsequently, a flooding with water is "
										"simulated resulting in thousands of basins. "
										"Higher water causes adjacent basins to merge. "));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Usp = nullptr;

	m_SlH = new QSlider(Qt::Horizontal, nullptr);
	m_SlH->setRange(0, 200);
	m_SlH->setValue(160);

	m_SbH = new QSpinBox(10, 100, 10, nullptr);
	m_SbH->setValue(40);
	m_SbhOld = m_SbH->value();

	m_BtnExec = new QPushButton("Execute");

	// layout
	auto height_hbox = new QHBoxLayout;
	height_hbox->addWidget(m_SlH);
	height_hbox->addWidget(m_SbH);

	auto layout = new QFormLayout;
	layout->addRow(tr("Flooding height (h)"), height_hbox);
	layout->addRow(m_BtnExec);

	setLayout(layout);

	// connections
	QObject_connect(m_SlH, SIGNAL(valueChanged(int)), this, SLOT(HslChanged()));
	QObject_connect(m_SlH, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlH, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SbH, SIGNAL(valueChanged(int)), this, SLOT(HsbChanged(int)));
	QObject_connect(m_BtnExec, SIGNAL(clicked()), this, SLOT(Execute()));
}

void WatershedWidget::HslChanged()
{
	Recalc1();
	emit EndDatachange(this, iseg::NoUndo);
}

void WatershedWidget::HsbChanged(int value)
{
	int oldval100 = m_SbhOld * m_SlH->value();
	int sbh_new = value;
	if (oldval100 < (sbh_new * 200))
	{
		m_SlH->setValue((int)(oldval100 / sbh_new));
	}
	else
	{
		Recalc();
	}
	m_SbhOld = sbh_new;
}

void WatershedWidget::Execute()
{
	if (m_Usp != nullptr)
	{
		free(m_Usp);
		m_Usp = nullptr;
	}

	m_Usp = m_Bmphand->WatershedSobel(false);

	Recalc();
}

void WatershedWidget::Recalc()
{
	if (m_Usp != nullptr)
	{
		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);

		Recalc1();

		emit EndDatachange(this);
	}
}

void WatershedWidget::MarksChanged()
{
	if (m_Usp != nullptr)
	{
		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);

		Recalc1();

		emit EndDatachange(this, iseg::MergeUndo);
	}
}

void WatershedWidget::Recalc1()
{
	if (m_Usp != nullptr)
	{
		m_Bmphand->ConstructRegions((unsigned int)(m_SbH->value() * m_SlH->value() * 0.005f), m_Usp);
	}
}

WatershedWidget::~WatershedWidget()
{
	free(m_Usp);
}

void WatershedWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void WatershedWidget::BmphandChanged(Bmphandler* bmph)
{
	if (m_Usp != nullptr)
	{
		free(m_Usp);
		m_Usp = nullptr;
	}

	m_Bmphand = bmph;
}

void WatershedWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void WatershedWidget::NewLoaded()
{
	if (m_Usp != nullptr)
	{
		free(m_Usp);
		m_Usp = nullptr;
	}

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void WatershedWidget::SliderPressed()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);
}

void WatershedWidget::SliderReleased() { emit EndDatachange(this); }

FILE* WatershedWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SbH->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlH->value();
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&m_SbhOld, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* WatershedWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_SlH, SIGNAL(valueChanged(int)), this, SLOT(HslChanged()));
		QObject_disconnect(m_SbH, SIGNAL(valueChanged(int)), this, SLOT(HsbChanged(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SbH->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlH->setValue(dummy);

		fread(&m_SbhOld, sizeof(float), 1, fp);

		QObject_connect(m_SlH, SIGNAL(valueChanged(int)), this, SLOT(HslChanged()));
		QObject_connect(m_SbH, SIGNAL(valueChanged(int)), this, SLOT(HsbChanged(int)));
	}
	return fp;
}

} // namespace iseg
