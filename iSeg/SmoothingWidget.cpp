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

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

namespace {
float f1(float dI, float k) { return std::exp(-std::pow(dI / k, 2)); }

float f2(float dI, float k) { return 1 / (1 + std::pow(dI / k, 2)); }
} // namespace

SmoothingWidget::SmoothingWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Smoothing and noise removal filters."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Hboxoverall = new Q3HBox(this);
	m_Hboxoverall->setMargin(8);
	m_Vboxmethods = new Q3VBox(m_Hboxoverall);
	m_Vbox1 = new Q3VBox(m_Hboxoverall);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Vbox2 = new Q3VBox(m_Vbox1);
	m_Hbox3 = new Q3HBox(m_Vbox2);
	m_Hbox5 = new Q3HBox(m_Vbox2);
	m_Hbox4 = new Q3HBox(m_Vbox1);
	m_Allslices = new QCheckBox(QString("Apply to all slices"), m_Vbox1);
	m_Pushexec = new QPushButton("Execute", m_Vbox1);
	m_Contdiff = new QPushButton("Cont. Diffusion", m_Vbox1);

	m_TxtN = new QLabel("n: ", m_Hbox1);
	m_SbN = new QSpinBox(1, 11, 2, m_Hbox1);
	m_SbN->setValue(5);
	m_SbN->setToolTip("'n' is the width of the kernel in pixels.");

	m_TxtSigma1 = new QLabel("Sigma: 0 ", m_Hbox2);
	m_SlSigma = new QSlider(Qt::Horizontal, m_Hbox2);
	m_SlSigma->setRange(1, 100);
	m_SlSigma->setValue(20);
	m_SlSigma->setToolTip("Sigma gives the radius of the smoothing filter. Larger "
												"values remove more details.");
	m_TxtSigma2 = new QLabel(" 5", m_Hbox2);

	m_TxtDt = new QLabel("dt: 1    ", m_Hbox3);
	m_TxtIter = new QLabel("Iterations: ", m_Hbox3);
	m_SbIter = new QSpinBox(1, 100, 1, m_Hbox3);
	m_SbIter->setValue(20);

	m_TxtK = new QLabel("Sigma: 0 ", m_Hbox4);
	m_SlK = new QSlider(Qt::Horizontal, m_Hbox4);
	m_SlK->setRange(0, 100);
	m_SlK->setValue(50);
	m_SlK->setToolTip("Together with value on the right, defines the Sigma of the "
										"smoothing filter.");
	m_SbKmax = new QSpinBox(1, 100, 1, m_Hbox4);
	m_SbKmax->setValue(10);
	m_SbKmax->setToolTip("Gives the max. radius of the smoothing filter. This "
											 "value defines the scale used in the slider bar.");

	m_TxtRestrain1 = new QLabel("Restraint: 0 ", m_Hbox5);
	m_SlRestrain = new QSlider(Qt::Horizontal, m_Hbox5);
	m_SlRestrain->setRange(0, 100);
	m_SlRestrain->setValue(0);
	m_TxtRestrain2 = new QLabel(" 1", m_Hbox5);

	m_RbGaussian = new QRadioButton(QString("Gaussian"), m_Vboxmethods);
	m_RbGaussian->setToolTip("Gaussian smoothing blurs the image and can remove "
													 "noise and details (high frequency).");
	m_RbAverage = new QRadioButton(QString("Average"), m_Vboxmethods);
	m_RbAverage->setToolTip("Mean smoothing blurs the image and can remove noise and details.");
	m_RbMedian = new QRadioButton(QString("Median"), m_Vboxmethods);
	m_RbMedian->setToolTip("Median filtering can remove (speckle or salt-and-pepper) noise.");
	m_RbSigmafilter = new QRadioButton(QString("Sigma Filter"), m_Vboxmethods);
	m_RbSigmafilter->setToolTip("Sigma filtering is a mixture between Gaussian and Average filtering. "
															"It "
															"preserves edges better than Average filtering.");
	m_RbAnisodiff =
			new QRadioButton(QString("Anisotropic Diffusion"), m_Vboxmethods);
	m_RbAnisodiff->setToolTip("Anisotropic diffusion can remove noise, while "
														"preserving significant details, such as edges.");

	m_Modegroup = new QButtonGroup(this);
	m_Modegroup->insert(m_RbGaussian);
	m_Modegroup->insert(m_RbAverage);
	m_Modegroup->insert(m_RbMedian);
	m_Modegroup->insert(m_RbSigmafilter);
	m_Modegroup->insert(m_RbAnisodiff);
	m_RbGaussian->setChecked(TRUE);

	m_SlRestrain->setFixedWidth(300);
	m_SlK->setFixedWidth(300);
	m_SlSigma->setFixedWidth(300);

	m_Vboxmethods->setMargin(5);
	m_Vbox1->setMargin(5);
	m_Vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	m_Vboxmethods->setLineWidth(1);

	m_Hbox1->setFixedSize(m_Hbox1->sizeHint());
	m_Hbox2->setFixedSize(m_Hbox2->sizeHint());
	m_Hbox3->setFixedSize(m_Hbox3->sizeHint());
	m_Hbox4->setFixedSize(m_Hbox4->sizeHint());
	m_Hbox5->setFixedSize(m_Hbox5->sizeHint());
	m_Vbox2->setFixedSize(m_Vbox2->sizeHint());
	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	MethodChanged(0);

	QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
	QObject_connect(m_Pushexec, SIGNAL(clicked()), this, SLOT(Execute()));
	QObject_connect(m_Contdiff, SIGNAL(clicked()), this, SLOT(ContinueDiff()));
	QObject_connect(m_SlSigma, SIGNAL(valueChanged(int)), this, SLOT(SigmasliderChanged(int)));
	QObject_connect(m_SlSigma, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlSigma, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SlK, SIGNAL(valueChanged(int)), this, SLOT(KsliderChanged(int)));
	QObject_connect(m_SlK, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlK, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SbN, SIGNAL(valueChanged(int)), this, SLOT(NChanged(int)));
	QObject_connect(m_SbKmax, SIGNAL(valueChanged(int)), this, SLOT(KmaxChanged(int)));
}

SmoothingWidget::~SmoothingWidget()
= default;

void SmoothingWidget::Execute()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		if (m_RbGaussian->isChecked())
		{
			m_Handler3D->Gaussian(m_SlSigma->value() * 0.05f);
		}
		else if (m_RbAverage->isChecked())
		{
			m_Handler3D->Average((short unsigned)m_SbN->value());
		}
		else if (m_RbMedian->isChecked())
		{
			m_Handler3D->MedianInterquartile(true);
		}
		else if (m_RbSigmafilter->isChecked())
		{
			m_Handler3D->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		}
		else
		{
			m_Handler3D->AnisoDiff(1.0f, m_SbIter->value(), f2, m_SlK->value() * 0.01f * m_SbKmax->value(), m_SlRestrain->value() * 0.01f);
		}
	}
	else // current slice
	{
		if (m_RbGaussian->isChecked())
		{
			m_Bmphand->Gaussian(m_SlSigma->value() * 0.05f);
		}
		else if (m_RbAverage->isChecked())
		{
			m_Bmphand->Average((short unsigned)m_SbN->value());
		}
		else if (m_RbMedian->isChecked())
		{
			m_Bmphand->MedianInterquartile(true);
		}
		else if (m_RbSigmafilter->isChecked())
		{
			m_Bmphand->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		}
		else
		{
			m_Bmphand->AnisoDiff(1.0f, m_SbIter->value(), f2, m_SlK->value() * 0.01f * m_SbKmax->value(), m_SlRestrain->value() * 0.01f);
		}
	}

	emit EndDatachange(this);
}

void SmoothingWidget::MethodChanged(int)
{
	if (m_RbGaussian->isChecked())
	{
		if (hideparams)
			m_Hbox2->hide();
		else
			m_Hbox2->show();
		m_Hbox1->hide();
		m_Hbox4->hide();
		m_Vbox2->hide();
		m_Contdiff->hide();
	}
	else if (m_RbAverage->isChecked())
	{
		if (hideparams)
			m_Hbox1->hide();
		else
			m_Hbox1->show();
		m_Hbox2->hide();
		m_Hbox4->hide();
		m_Vbox2->hide();
		m_Contdiff->hide();
	}
	else if (m_RbMedian->isChecked())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox4->hide();
		m_Vbox2->hide();
		m_Contdiff->hide();
	}
	else if (m_RbSigmafilter->isChecked())
	{
		if (hideparams)
			m_Hbox1->hide();
		else
			m_Hbox1->show();
		//		hbox2->show();
		m_Hbox2->hide();
		m_Vbox2->hide();
		if (hideparams)
			m_Hbox4->hide();
		else
			m_Hbox4->show();
		m_Contdiff->hide();
		m_TxtK->setText("Sigma: 0 ");
	}
	else
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_TxtK->setText("k: 0 ");
		if (hideparams)
			m_Hbox4->hide();
		else
			m_Hbox4->show();
		if (hideparams)
			m_Vbox2->hide();
		else
			m_Vbox2->show();
		if (hideparams)
			m_Contdiff->hide();
		else
			m_Contdiff->show();
	}
}

void SmoothingWidget::ContinueDiff()
{
	if (!m_RbAnisodiff->isChecked())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		m_Handler3D->ContAnisodiff(1.0f, m_SbIter->value(), f2, m_SlK->value() * 0.01f * m_SbKmax->value(), m_SlRestrain->value() * 0.01f);
	}
	else
	{
		m_Bmphand->ContAnisodiff(1.0f, m_SbIter->value(), f2, m_SlK->value() * 0.01f * m_SbKmax->value(), m_SlRestrain->value() * 0.01f);
	}

	emit EndDatachange(this);
}

void SmoothingWidget::SigmasliderChanged(int /* newval */)
{
	if (m_RbGaussian->isChecked())
	{
		if (m_Allslices->isChecked())
			m_Handler3D->Gaussian(m_SlSigma->value() * 0.05f);
		else
			m_Bmphand->Gaussian(m_SlSigma->value() * 0.05f);
		emit EndDatachange(this, iseg::NoUndo);
	}
}

void SmoothingWidget::KsliderChanged(int /* newval */)
{
	if (m_RbSigmafilter->isChecked())
	{
		if (m_Allslices->isChecked())
			m_Handler3D->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		else
			m_Bmphand->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		emit EndDatachange(this, iseg::NoUndo);
	}
}

void SmoothingWidget::NChanged(int /* newval */)
{
	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_RbSigmafilter->isChecked())
	{
		if (m_Allslices->isChecked())
		{
			m_Handler3D->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		}
		else
		{
			m_Bmphand->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		}
	}
	else if (m_RbAverage)
	{
		if (m_Allslices->isChecked())
		{
			m_Handler3D->Average((short unsigned)m_SbN->value());
		}
		else
		{
			m_Bmphand->Average((short unsigned)m_SbN->value());
		}
	}
	emit EndDatachange(this);
}

void SmoothingWidget::KmaxChanged(int /* newval */)
{
	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_RbSigmafilter->isChecked())
	{
		if (m_Allslices->isChecked())
		{
			m_Handler3D->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		}
		else
		{
			m_Bmphand->Sigmafilter((m_SlK->value() + 1) * 0.01f * m_SbKmax->value(), (short unsigned)m_SbN->value(), (short unsigned)m_SbN->value());
		}
	}
	emit EndDatachange(this);
}

QSize SmoothingWidget::sizeHint() const { return m_Vbox1->sizeHint(); }

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

void SmoothingWidget::SliderPressed()
{
	if ((m_RbGaussian->isChecked() || m_RbSigmafilter->isChecked()))
	{
		DataSelection data_selection;
		data_selection.allSlices = m_Allslices->isChecked();
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);
	}
}

void SmoothingWidget::SliderReleased()
{
	if (m_RbGaussian->isChecked() || m_RbSigmafilter->isChecked())
	{
		emit EndDatachange(this);
	}
}

FILE* SmoothingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlSigma->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlK->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlRestrain->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbN->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbIter->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SbKmax->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbGaussian->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbAverage->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbMedian->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbSigmafilter->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbAnisodiff->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Allslices->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* SmoothingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
		QObject_disconnect(m_SlSigma, SIGNAL(valueChanged(int)), this, SLOT(SigmasliderChanged(int)));
		QObject_disconnect(m_SlK, SIGNAL(valueChanged(int)), this, SLOT(KsliderChanged(int)));
		QObject_disconnect(m_SbN, SIGNAL(valueChanged(int)), this, SLOT(NChanged(int)));
		QObject_disconnect(m_SbKmax, SIGNAL(valueChanged(int)), this, SLOT(KmaxChanged(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SlSigma->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlK->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlRestrain->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbN->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbIter->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SbKmax->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbGaussian->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbAverage->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbMedian->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbSigmafilter->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbAnisodiff->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Allslices->setChecked(dummy > 0);

		MethodChanged(0);

		QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
		QObject_connect(m_SlSigma, SIGNAL(valueChanged(int)), this, SLOT(SigmasliderChanged(int)));
		QObject_connect(m_SlK, SIGNAL(valueChanged(int)), this, SLOT(KsliderChanged(int)));
		QObject_connect(m_SbN, SIGNAL(valueChanged(int)), this, SLOT(NChanged(int)));
		QObject_connect(m_SbKmax, SIGNAL(valueChanged(int)), this, SLOT(KmaxChanged(int)));
	}
	return fp;
}

void SmoothingWidget::HideParamsChanged() { MethodChanged(0); }

} // namespace iseg
