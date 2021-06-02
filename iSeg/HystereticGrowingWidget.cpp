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

#include "HystereticGrowingWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/addLine.h"

#include "Core/Pair.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

HystereticGrowingWidget::HystereticGrowingWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Segment a tissue by picking seed points and adding "
										"neighboring pixels with similar intensities."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	Pair p;
	m_P1.px = m_P1.py = 0;
	m_Bmphand->GetBmprange(&p);
	m_LowerLimit = p.low;
	m_UpperLimit = p.high;

	m_Vbox1 = new Q3VBox(this);
	m_Vbox1->setMargin(8);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Hbox2a = new Q3HBox(m_Vbox1);
	m_Vbox2 = new Q3VBox(m_Hbox1);
	m_Vbox3 = new Q3VBox(m_Hbox1);
	m_Autoseed = new QCheckBox("AutoSeed: ", m_Hbox2);
	if (m_Autoseed->isChecked())
		m_Autoseed->toggle();
	m_Vbox4 = new Q3VBox(m_Hbox2);
	m_Vbox5 = new Q3VBox(m_Hbox2);
	m_Vbox6 = new Q3VBox(m_Hbox1);
	m_Vbox7 = new Q3VBox(m_Hbox2);
	m_Allslices = new QCheckBox(QString("Apply to all slices"), m_Hbox2a);
	m_PbSaveborders = new QPushButton("Save...", m_Hbox2a);
	m_PbLoadborders = new QPushButton("Open...", m_Hbox2a);
	m_TxtLower = new QLabel("Lower: ", m_Vbox2);
	m_TxtUpper = new QLabel("Upper: ", m_Vbox2);
	m_TxtLowerhyster = new QLabel("Lower: ", m_Vbox4);
	m_TxtUpperhyster = new QLabel("Upper: ", m_Vbox4);
	m_SlLower = new QSlider(Qt::Horizontal, m_Vbox3);
	m_SlLower->setRange(0, 200);
	m_SlLower->setValue(80);
	m_LeBordervall = new QLineEdit(m_Vbox6);
	m_LeBordervall->setFixedWidth(80);
	m_SlUpper = new QSlider(Qt::Horizontal, m_Vbox3);
	m_SlUpper->setRange(0, 200);
	m_SlUpper->setValue(120);
	m_LeBordervalu = new QLineEdit(m_Vbox6);
	m_LeBordervalu->setFixedWidth(80);
	m_SlLowerhyster = new QSlider(Qt::Horizontal, m_Vbox5);
	m_SlLowerhyster->setRange(0, 200);
	m_SlLowerhyster->setValue(60);
	m_LeBordervallh = new QLineEdit(m_Vbox7);
	m_LeBordervallh->setFixedWidth(80);
	m_SlUpperhyster = new QSlider(Qt::Horizontal, m_Vbox5);
	m_SlUpperhyster->setRange(0, 200);
	m_SlUpperhyster->setValue(140);
	m_LeBordervaluh = new QLineEdit(m_Vbox7);
	m_LeBordervaluh->setFixedWidth(80);
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_Drawlimit = new QPushButton("Draw Limit", m_Hbox3);
	m_Clearlimit = new QPushButton("Clear Limit", m_Hbox3);
	m_Pushexec = new QPushButton("Execute", m_Vbox1);

	m_SlLower->setFixedWidth(400);
	m_SlLowerhyster->setFixedWidth(300);

	m_Vbox2->setFixedSize(m_Vbox2->sizeHint());
	m_Vbox3->setFixedSize(m_Vbox3->sizeHint());
	m_Vbox4->setFixedSize(m_Vbox4->sizeHint());
	m_Vbox5->setFixedSize(m_Vbox5->sizeHint());
	m_Vbox6->setFixedSize(m_Vbox6->sizeHint());
	m_Vbox7->setFixedSize(m_Vbox7->sizeHint());
	m_Hbox1->setFixedSize(m_Hbox1->sizeHint());
	m_Hbox2->setFixedSize(m_Hbox2->sizeHint());
	m_Hbox2a->setFixedSize(m_Hbox2a->sizeHint());
	m_Hbox3->setFixedSize(m_Hbox3->sizeHint());
	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	//	setFixedSize(vbox1->size());

	QObject_connect(m_Pushexec, SIGNAL(clicked()), this, SLOT(Execute()));
	QObject_connect(m_Drawlimit, SIGNAL(clicked()), this, SLOT(Limitpressed()));
	QObject_connect(m_Clearlimit, SIGNAL(clicked()), this, SLOT(Clearpressed()));
	QObject_connect(m_Autoseed, SIGNAL(clicked()), this, SLOT(AutoToggled()));
	QObject_connect(m_SlLower, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlUpper, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlLowerhyster, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlUpperhyster, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	QObject_connect(m_SlLower, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlUpper, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlLowerhyster, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlUpperhyster, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlLower, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SlUpper, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SlLowerhyster, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SlUpperhyster, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));

	QObject_connect(m_LeBordervall, SIGNAL(returnPressed()), this, SLOT(LeBordervallReturnpressed()));
	QObject_connect(m_LeBordervalu, SIGNAL(returnPressed()), this, SLOT(LeBordervaluReturnpressed()));
	QObject_connect(m_LeBordervallh, SIGNAL(returnPressed()), this, SLOT(LeBordervallhReturnpressed()));
	QObject_connect(m_LeBordervaluh, SIGNAL(returnPressed()), this, SLOT(LeBordervaluhReturnpressed()));

	QObject_connect(m_PbSaveborders, SIGNAL(clicked()), this, SLOT(SavebordersExecute()));
	QObject_connect(m_PbLoadborders, SIGNAL(clicked()), this, SLOT(LoadbordersExecute()));

	UpdateVisible();
	Getrange();
	Init1();
}

void HystereticGrowingWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void HystereticGrowingWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;

	Getrange();

	Init1();
}

void HystereticGrowingWidget::OnMouseClicked(Point p)
{
	if (m_Limitdrawing)
	{
		m_LastPt = p;
	}
	else
	{
		if (!m_Autoseed->isChecked())
		{
			m_P1 = p;
			Execute();
		}
	}
}

void HystereticGrowingWidget::UpdateVisible()
{
	if (m_Autoseed->isChecked())
	{
		m_Vbox4->show();
		m_Vbox5->show();
		m_Vbox7->show();
		m_Allslices->setText("Apply to all slices");
		//		p1=nullptr;
	}
	else
	{
		m_Vbox4->hide();
		m_Vbox5->hide();
		m_Vbox7->hide();
		m_Allslices->setText("3D");
		//		if(p1!=nullptr)
	}
}

void HystereticGrowingWidget::AutoToggled()
{
	UpdateVisible();

	if (m_Autoseed->isChecked())
	{
		Execute();
	}
}

void HystereticGrowingWidget::Execute()
{
	DataSelection data_selection;
	data_selection.work = data_selection.limits = true;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	emit BeginDatachange(data_selection, this);

	Execute1();

	emit EndDatachange(this);
}

void HystereticGrowingWidget::Execute1()
{
	if (m_Autoseed->isChecked())
	{
		float ll = m_LowerLimit +
							 (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
		float uu = m_LowerLimit +
							 (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
		float ul = ll + (uu - ll) * 0.005f * m_SlLowerhyster->value();
		float lu = ll + (uu - ll) * 0.005f * m_SlUpperhyster->value();
		if (m_Allslices->isChecked())
			m_Handler3D->DoubleHystereticAllslices(ll, ul, lu, uu, false, 255.0f);
		else
			m_Bmphand->DoubleHysteretic(ll, ul, lu, uu, false, 255.0f);

		m_LeBordervall->setText(QString::number(ll, 'g', 3));
		m_LeBordervalu->setText(QString::number(uu, 'g', 3));
		m_LeBordervallh->setText(QString::number(int(0.4f + (ul - ll) / (uu - ll) * 100), 'g', 3));
		m_LeBordervaluh->setText(QString::number(int(0.4f + (lu - ll) / (uu - ll) * 100), 'g', 3));
	}
	else
	{
		float ll = m_LowerLimit +
							 (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
		float uu = m_LowerLimit +
							 (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
		//		bmphand->thresholded_growing(p1,ll,uu,false,255.f);
		if (m_Allslices->isChecked())
		{
			m_Handler3D->ThresholdedGrowing(m_Handler3D->ActiveSlice(), m_P1, ll, uu, 255.f);
		}
		else
		{
			m_Bmphand->ThresholdedGrowinglimit(m_P1, ll, uu, false, 255.f);
		}

		m_LeBordervall->setText(QString::number(ll, 'g', 3));
		m_LeBordervalu->setText(QString::number(uu, 'g', 3));
	}
}

void HystereticGrowingWidget::Getrange()
{
	float ll =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
	float uu =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
	float ul = ll + (uu - ll) * 0.005f * m_SlLowerhyster->value();
	float lu = ll + (uu - ll) * 0.005f * m_SlUpperhyster->value();

	Pair p;
	m_Bmphand->GetBmprange(&p);
	m_LowerLimit = p.low;
	m_UpperLimit = p.high;

	GetrangeSub(ll, uu, ul, lu);
}

void HystereticGrowingWidget::GetrangeSub(float ll, float uu, float ul, float lu)
{
	if (ll < m_LowerLimit)
	{
		ll = m_LowerLimit;
		m_SlLower->setValue(0);
	}
	else if (ll > m_UpperLimit)
	{
		ll = m_UpperLimit;
		m_SlLower->setValue(200);
	}
	else
	{
		if (m_UpperLimit == m_LowerLimit)
			m_SlLower->setValue(100);
		else
			m_SlLower->setValue(int(0.5f + (ll - m_LowerLimit) / (m_UpperLimit - m_LowerLimit) * 200));
	}

	if (uu < m_LowerLimit)
	{
		uu = m_LowerLimit;
		m_SlUpper->setValue(0);
	}
	else if (uu > m_UpperLimit)
	{
		uu = m_UpperLimit;
		m_SlUpper->setValue(200);
	}
	else
	{
		if (m_UpperLimit == m_LowerLimit)
			m_SlUpper->setValue(100);
		else
			m_SlUpper->setValue(int(0.5f + (uu - m_LowerLimit) / (m_UpperLimit - m_LowerLimit) * 200));
	}

	if (ul < ll)
	{
		ul = ll;
		m_SlLowerhyster->setValue(0);
	}
	else if (ul > uu)
	{
		ul = uu;
		m_SlLowerhyster->setValue(200);
	}
	else
	{
		if (uu == ll)
			m_SlLowerhyster->setValue(100);
		else
			m_SlLowerhyster->setValue(int(0.5f + (ul - ll) / (uu - ll) * 200));
	}

	if (lu < ll)
	{
		lu = ll;
		m_SlUpperhyster->setValue(0);
	}
	else if (lu > uu)
	{
		lu = uu;
		m_SlUpperhyster->setValue(200);
	}
	else
	{
		if (uu == ll)
			m_SlUpperhyster->setValue(100);
		else
			m_SlUpperhyster->setValue(int(0.5f + (lu - ll) / (uu - ll) * 200));
	}

	m_LeBordervall->setText(QString::number(ll, 'g', 3));
	m_LeBordervalu->setText(QString::number(uu, 'g', 3));
	m_LeBordervallh->setText(QString::number(int(0.4f + (ul - ll) / (uu - ll) * 100), 'g', 3));
	m_LeBordervaluh->setText(QString::number(int(0.4f + (lu - ll) / (uu - ll) * 100), 'g', 3));
}

void HystereticGrowingWidget::SliderChanged() { Execute1(); }

void HystereticGrowingWidget::BmpChanged()
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	Getrange();
}

QSize HystereticGrowingWidget::sizeHint() const { return m_Vbox1->sizeHint(); }

void HystereticGrowingWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		m_Bmphand = m_Handler3D->GetActivebmphandler();
		Getrange();
	}
	Init1();
	HideParamsChanged();
}

void HystereticGrowingWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	Getrange();

	std::vector<std::vector<Point>>* vvp = m_Bmphand->ReturnLimits();
	m_Vp1.clear();
	for (auto it = vvp->begin(); it != vvp->end(); it++)
	{
		m_Vp1.insert(m_Vp1.end(), it->begin(), it->end());
	}
}

void HystereticGrowingWidget::Init1()
{
	m_Limitdrawing = false;
	m_Drawlimit->setDown(false);

	std::vector<std::vector<Point>>* vvp = m_Bmphand->ReturnLimits();
	m_Vp1.clear();
	for (auto it = vvp->begin(); it != vvp->end(); it++)
	{
		m_Vp1.insert(m_Vp1.end(), it->begin(), it->end());
	}

	emit Vp1Changed(&m_Vp1);
}

void HystereticGrowingWidget::Cleanup()
{
	m_Limitdrawing = false;
	m_Drawlimit->setDown(false);
	m_Vpdyn.clear();
	emit Vp1dynChanged(&m_Vpdyn, &m_Vpdyn);
}

void HystereticGrowingWidget::OnMouseMoved(Point p)
{
	if (m_Limitdrawing)
	{
		addLine(&m_Vpdyn, m_LastPt, p);
		m_LastPt = p;
		emit VpdynChanged(&m_Vpdyn);
	}
}

void HystereticGrowingWidget::OnMouseReleased(Point p)
{
	if (m_Limitdrawing)
	{
		addLine(&m_Vpdyn, m_LastPt, p);
		m_Limitdrawing = false;
		m_Vp1.insert(m_Vp1.end(), m_Vpdyn.begin(), m_Vpdyn.end());

		DataSelection data_selection;
		data_selection.limits = true;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		emit BeginDatachange(data_selection, this);

		m_Bmphand->AddLimit(&m_Vpdyn);

		emit EndDatachange(this);

		m_Vpdyn.clear();
		m_Drawlimit->setDown(false);
		emit Vp1dynChanged(&m_Vp1, &m_Vpdyn);
	}
}

void HystereticGrowingWidget::Limitpressed()
{
	m_Drawlimit->setDown(true);
	m_Limitdrawing = true;
	m_Vpdyn.clear();
	emit VpdynChanged(&m_Vpdyn);
}

void HystereticGrowingWidget::Clearpressed()
{
	m_Limitdrawing = false;
	m_Drawlimit->setDown(false);

	DataSelection data_selection;
	data_selection.limits = true;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	emit BeginDatachange(data_selection, this);

	m_Bmphand->ClearLimits();

	emit EndDatachange(this);

	m_Vpdyn.clear();
	m_Vp1.clear();
	emit Vp1dynChanged(&m_Vp1, &m_Vpdyn);
}

void HystereticGrowingWidget::SliderPressed()
{
	DataSelection data_selection;
	data_selection.work = data_selection.limits = true;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	emit BeginDatachange(data_selection, this);
}

void HystereticGrowingWidget::SliderReleased() { emit EndDatachange(this); }

void HystereticGrowingWidget::SavebordersExecute()
{
	QString savefilename = QFileDialog::getSaveFileName(QString::null, "Thresholds (*.txt)\n", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		float ll = m_LowerLimit +
							 (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
		float uu = m_LowerLimit +
							 (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
		float ul = ll + (uu - ll) * 0.005f * m_SlLowerhyster->value();
		float lu = ll + (uu - ll) * 0.005f * m_SlUpperhyster->value();
		FILE* fp = fopen(savefilename.ascii(), "w");
		fprintf(fp, "%f %f %f %f \n", ll, ul, lu, uu);
		fclose(fp);
	}
}

void HystereticGrowingWidget::LoadbordersExecute()
{
	QString loadfilename = QFileDialog::getOpenFileName(QString::null, "Borders (*.txt)\n"
																																		 "All (*.*)",
			this);

	if (!loadfilename.isEmpty())
	{
		FILE* fp = fopen(loadfilename.ascii(), "r");
		if (fp != nullptr)
		{
			float ll, uu, lu, ul;
			if (fscanf(fp, "%f %f %f %f", &ll, &ul, &lu, &uu) == 4)
			{
				GetrangeSub(ll, uu, ul, lu);
			}
		}
		fclose(fp);
	}
}

void HystereticGrowingWidget::LeBordervallReturnpressed()
{
	bool b1;
	float val = m_LeBordervall->text().toFloat(&b1);
	float ll =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
	if (b1)
	{
		if (val < m_LowerLimit)
		{
			ll = m_LowerLimit;
			m_SlLower->setValue(0);
			m_LeBordervall->setText(QString::number(ll, 'g', 3));
		}
		else if (val > m_UpperLimit)
		{
			ll = m_UpperLimit;
			m_SlLower->setValue(200);
			m_LeBordervall->setText(QString::number(ll, 'g', 3));
		}
		else
		{
			if (m_UpperLimit == m_LowerLimit)
			{
				ll = m_UpperLimit;
				m_SlLower->setValue(100);
				m_LeBordervall->setText(QString::number(ll, 'g', 3));
			}
			else
				m_SlLower->setValue(int(0.5f + (val - m_LowerLimit) /
																					 (m_UpperLimit - m_LowerLimit) *
																					 200));
		}

		Execute();
	}
	else
	{
		QApplication::beep();
		m_LeBordervall->setText(QString::number(ll, 'g', 3));
	}
}

void HystereticGrowingWidget::LeBordervaluReturnpressed()
{
	bool b1;
	float val = m_LeBordervalu->text().toFloat(&b1);
	float uu =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
	if (b1)
	{
		if (val < m_LowerLimit)
		{
			uu = m_LowerLimit;
			m_SlUpper->setValue(0);
			m_LeBordervalu->setText(QString::number(uu, 'g', 3));
		}
		else if (val > m_UpperLimit)
		{
			uu = m_UpperLimit;
			m_SlUpper->setValue(200);
			m_LeBordervalu->setText(QString::number(uu, 'g', 3));
		}
		else
		{
			if (m_UpperLimit == m_LowerLimit)
			{
				uu = m_UpperLimit;
				m_SlUpper->setValue(100);
				m_LeBordervalu->setText(QString::number(uu, 'g', 3));
			}
			else
				m_SlUpper->setValue(int(0.5f + (val - m_LowerLimit) /
																					 (m_UpperLimit - m_LowerLimit) *
																					 200));
		}

		Execute();
	}
	else
	{
		QApplication::beep();
		m_LeBordervall->setText(QString::number(uu, 'g', 3));
	}
}

void HystereticGrowingWidget::LeBordervallhReturnpressed()
{
	bool b1;
	float val = m_LeBordervallh->text().toFloat(&b1);
	float ll =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
	float uu =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
	float ul = ll + (uu - ll) * 0.005f * m_SlLowerhyster->value();
	if (b1)
	{
		if (val < 0)
		{
			ul = ll;
			m_SlLowerhyster->setValue(0);
			m_LeBordervallh->setText(QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
		}
		else if (val > 100)
		{
			ul = uu;
			m_SlLowerhyster->setValue(200);
			m_LeBordervallh->setText(QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
		}
		else
		{
			if (uu == ll)
			{
				ul = uu;
				m_SlLowerhyster->setValue(100);
				m_LeBordervallh->setText(QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
			}
			else
				m_SlLowerhyster->setValue(int(0.5f + val / 100 * 200));
		}

		Execute();
	}
	else
	{
		QApplication::beep();
		m_LeBordervallh->setText(QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
	}
}

void HystereticGrowingWidget::LeBordervaluhReturnpressed()
{
	bool b1;
	float val = m_LeBordervaluh->text().toFloat(&b1);
	float ll =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlLower->value();
	float uu =
			m_LowerLimit + (m_UpperLimit - m_LowerLimit) * 0.005f * m_SlUpper->value();
	float lu = ll + (uu - ll) * 0.005f * m_SlUpperhyster->value();
	if (b1)
	{
		if (val < 0)
		{
			lu = ll;
			m_SlUpperhyster->setValue(0);
			m_LeBordervaluh->setText(QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
		}
		else if (val > 100)
		{
			lu = uu;
			m_SlUpperhyster->setValue(200);
			m_LeBordervaluh->setText(QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
		}
		else
		{
			if (uu == ll)
			{
				lu = uu;
				m_SlUpperhyster->setValue(100);
				m_LeBordervaluh->setText(QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
			}
			else
				m_SlUpperhyster->setValue(int(0.5f + val / 100 * 200));
		}

		Execute();
	}
	else
	{
		QApplication::beep();
		m_LeBordervaluh->setText(QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
	}
}

FILE* HystereticGrowingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlLower->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlUpper->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlLowerhyster->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlUpperhyster->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Autoseed->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Allslices->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&m_UpperLimit, 1, sizeof(float), fp);
		fwrite(&m_LowerLimit, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* HystereticGrowingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_Autoseed, SIGNAL(clicked()), this, SLOT(AutoToggled()));
		QObject_disconnect(m_SlLower, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
		QObject_disconnect(m_SlUpper, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
		QObject_disconnect(m_SlLowerhyster, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
		QObject_disconnect(m_SlUpperhyster, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SlLower->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlUpper->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlLowerhyster->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlUpperhyster->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Autoseed->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Allslices->setChecked(dummy > 0);

		UpdateVisible();

		//		AutoToggled();

		fread(&m_UpperLimit, sizeof(float), 1, fp);
		fread(&m_LowerLimit, sizeof(float), 1, fp);

		QObject_connect(m_Autoseed, SIGNAL(clicked()), this, SLOT(AutoToggled()));
		QObject_connect(m_SlLower, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
		QObject_connect(m_SlUpper, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
		QObject_connect(m_SlLowerhyster, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
		QObject_connect(m_SlUpperhyster, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged()));
	}
	return fp;
}

void HystereticGrowingWidget::HideParamsChanged()
{
	if (hideparams)
	{
		m_PbSaveborders->hide();
		m_PbLoadborders->hide();
	}
	else
	{
		m_PbSaveborders->show();
		m_PbLoadborders->show();
	}
}

} // namespace iseg
