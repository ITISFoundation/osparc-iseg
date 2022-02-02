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

#include "WidgetCollection.h"

#include "LoaderWidgets.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"
#include "TissueTreeWidget.h"
#include "bmp_read_1.h"
#include "config.h"

#include "Interface/FormatTooltip.h"
#include "Interface/RecentPlaces.h"

#include <QApplication>
#include <QButtonGroup>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QListWidget>
#include <QPaintEvent>
#include <QProgressDialog>
#include <QTableWidget>

#include <algorithm>
#include <fstream>

namespace iseg {

ScaleWork::ScaleWork(SlicesHandler* hand3D, QDir picpath, QWidget* parent, Qt::WindowFlags wFlags)
		//  : QWidget( parent, name, wFlags ),handler3D(hand3D)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Vbox1 = new Q3VBox(this);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_LL = new QLabel(QString("Low: "), m_Hbox1);
	m_LL->show();
	m_LimitLow = new QLineEdit(QString::number((int)0), m_Hbox1);
	m_LimitLow->show();
	m_Hbox1->show();
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_LH = new QLabel(QString("High: "), m_Hbox2);
	m_LH->show();
	m_LimitHigh = new QLineEdit(QString::number((int)255), m_Hbox2);
	m_LimitHigh->show();
	m_Hbox2->show();
	m_Allslices = new QCheckBox(QString("3D"), m_Vbox1);
	m_GetRange = new QPushButton("Get Range", m_Vbox1);
	m_GetRange->show();
	m_DoScale = new QPushButton("Scale", m_Vbox1);
	m_DoScale->show();
	m_DoCrop = new QPushButton("Clamp", m_Vbox1);
	m_DoCrop->show();
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_LbBrightness = new QLabel(QString("B: "), m_Hbox3);
	m_LbBrightness->show();
	m_LbBrightness->setPixmap(QIcon(picpath.absoluteFilePath(QString("icon-brightness.png")).ascii())
																.pixmap());
	m_SlBrighness = new QSlider(Qt::Horizontal, m_Hbox3);
	m_SlBrighness->setRange(0, 100);
	m_SlBrighness->setValue(30);
	m_Hbox3->show();
	m_Hbox4 = new Q3HBox(m_Vbox1);
	m_LbContrast = new QLabel(QString("C: "), m_Hbox4);
	m_LbContrast->show();
	m_LbContrast->setPixmap(QIcon(picpath.absoluteFilePath(QString("icon-contrast.png")).ascii())
															.pixmap());
	m_SlContrast = new QSlider(Qt::Horizontal, m_Hbox4);
	m_SlContrast->setRange(0, 99);
	m_SlContrast->setValue(30);
	m_Hbox4->show();

	m_CloseButton = new QPushButton("Close", m_Vbox1);

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	setFixedSize(m_Vbox1->size());

	QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_GetRange, SIGNAL(clicked()), this, SLOT(GetrangePushed()));
	QObject_connect(m_DoScale, SIGNAL(clicked()), this, SLOT(ScalePushed()));
	QObject_connect(m_DoCrop, SIGNAL(clicked()), this, SLOT(CropPushed()));
	QObject_connect(m_SlBrighness, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SlBrighness, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlBrighness, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
	QObject_connect(m_SlContrast, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SlContrast, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlContrast, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));

	GetrangePushed();
}

ScaleWork::~ScaleWork() { delete m_Vbox1; }

void ScaleWork::GetrangePushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	Pair p;
	if (m_Allslices->isChecked())
	{
		m_Handler3D->GetRange(&p);
	}
	else
	{
		m_Bmphand->GetRange(&p);
	}
	m_Minval = m_Minval1 = p.low;
	m_Maxval = m_Maxval1 = p.high;
	m_LimitLow->setText(QString::number((double)p.low, 'f', 6));
	m_LimitHigh->setText(QString::number((double)p.high, 'f', 6));

	QObject_disconnect(m_SlBrighness, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	QObject_disconnect(m_SlContrast, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	int brightnessint = (int)(100 * (m_Maxval - 127.5f) / (m_Maxval - m_Minval));
	if (brightnessint < 0)
	{
		brightnessint = 0;
	}
	if (brightnessint > 100)
	{
		brightnessint = 100;
	}
	m_SlBrighness->setValue(brightnessint);
	int contrastint = 100 - (int)(25500.0f / (m_Maxval - m_Minval));
	if (contrastint < 0)
	{
		contrastint = 0;
	}
	if (contrastint > 99)
	{
		contrastint = 99;
	}
	m_SlContrast->setValue(contrastint);
	QObject_connect(m_SlBrighness, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SlContrast, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));

	if (m_Allslices->isChecked())
	{
		m_Bmphand->GetRange(&p);
		m_Minval1 = p.low;
		m_Maxval1 = p.high;
	}
}

void ScaleWork::ScalePushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	Pair p;
	bool b1, b2;
	p.low = m_LimitLow->text().toFloat(&b1);
	p.high = m_LimitHigh->text().toFloat(&b2);
	if (b1 && b2)
	{
		DataSelection data_selection;
		data_selection.allSlices = m_Allslices->isChecked();
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);

		if (m_Allslices->isChecked())
		{
			m_Handler3D->ScaleColors(p);
		}
		else
		{
			m_Bmphand->ScaleColors(p);
		}

		emit EndDatachange(this);
	}
}

void ScaleWork::CropPushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		m_Handler3D->CropColors();
	}
	else
	{
		m_Bmphand->CropColors();
	}

	emit EndDatachange(this);
}

void ScaleWork::SlicenrChanged()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
	//	}
}

void ScaleWork::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
}

void ScaleWork::SliderChanged(int /* newval */)
{
	if (m_Minval1 < m_Maxval1)
	{
		m_Bmphand = m_Handler3D->GetActivebmphandler();
		Pair p, p1;
		m_Bmphand->GetRange(&p1);
		float subpos1, subpos2;
		subpos1 = ((1.0f - 0.01f * m_SlBrighness->value()) * (m_Maxval - m_Minval) -
									(m_Minval1 - m_Minval)) /
							(m_Maxval1 - m_Minval1);
		subpos2 = (1.0f - 0.01f * m_SlContrast->value()) * (m_Maxval - m_Minval) *
							0.5f / (m_Maxval1 - m_Minval1);
		p.low = p1.low + (p1.high - p1.low) * (subpos1 - subpos2);
		p.high = p1.low + (p1.high - p1.low) * (subpos1 + subpos2);

		//		if(allslices->isChecked()){
		//			handler3D->scale_colors(p);
		//		} else {
		//			bmphand=handler3D->get_activebmphandler();
		m_Bmphand->ScaleColors(p);
		//		}
	}
	emit EndDatachange(this, iseg::NoUndo);
}

void ScaleWork::SliderPressed()
{
	GetrangePushed();

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);
}

void ScaleWork::SliderReleased()
{
	if (m_Allslices->isChecked())
	{
		m_Bmphand = m_Handler3D->GetActivebmphandler();
		Pair p, p1, p2;
		m_Bmphand->GetRange(&p);
		float subpos1, subpos2;
		subpos1 = -m_Minval1 / (m_Maxval1 - m_Minval1);
		subpos2 = (255.0f - m_Minval) / (m_Maxval1 - m_Minval1);
		p1.low = p.low + (p.high - p.low) * subpos1;
		p1.high = p.low + (p.high - p.low) * subpos2;
		m_Bmphand->ScaleColors(p1);
		subpos1 = 1.0f - 0.01f * m_SlBrighness->value();
		subpos2 = 0.5f - 0.005f * m_SlContrast->value();
		p2.low = m_Minval + (m_Maxval - m_Minval) * (subpos1 - subpos2);
		p2.high = m_Minval + (m_Maxval - m_Minval) * (subpos1 + subpos2);
		m_Handler3D->ScaleColors(p2);
	}

	emit EndDatachange(this);
}

HistoWin::HistoWin(unsigned int* histo1, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags)
{
	m_Histo = histo1;
	m_Image.create(258, 258, 8);
	m_Image.setNumColors(256);
	for (int i = 0; i < 256; i++)
	{
		m_Image.setColor(i, qRgb(i, i, i));
	}
	setFixedSize(258, 258);
	update();
	show();
}

void HistoWin::update()
{
	unsigned int maxim = 1;
	//unsigned int maxim1=0;
	for (int i = 0; i < 256; i++)
	{
		maxim = std::max(maxim, m_Histo[i]);
	}

	m_Image.fill(0);

	for (int i = 0; i < 256; i++)
	{
		for (int j = 255; j > (int)(255 - ((m_Histo[i] * 255) / maxim)); j--)
		{
			m_Image.setPixel(i + 1, j + 1, 255);
		}
	}

	repaint();
}

void HistoWin::HistoChanged(unsigned int* histo1)
{
	m_Histo = histo1;
}

void HistoWin::paintEvent(QPaintEvent* e)
{
	if (m_Image.size() != QSize(0, 0)) // is an image loaded?
	{
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.drawImage(0, 0, m_Image);
	}
}

ShowHisto::ShowHisto(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Vbox1 = new Q3VBox(this);
	m_Bmphand->MakeHistogram(true);
	m_Histwindow = new HistoWin(m_Bmphand->ReturnHistogram(), m_Vbox1, wFlags);
	m_Histwindow->setFixedSize(258, 258);

	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Pictselect = new QButtonGroup(this);
	//	pictselect->hide();
	m_Bmppict = new QRadioButton(QString("Source"), m_Hbox1);
	m_Workpict = new QRadioButton(QString("Target"), m_Hbox1);
	m_Pictselect->insert(m_Bmppict);
	m_Pictselect->insert(m_Workpict);
	m_Workpict->setChecked(TRUE);
	m_Workpict->show();
	m_Bmppict->show();
	m_Hbox1->show();

	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Subsect = new QCheckBox(QString("Subsection "), m_Hbox2);
	m_Subsect->setChecked(TRUE);
	m_Subsect->show();
	m_Vbox2 = new Q3VBox(m_Hbox2);
	m_Hbox3 = new Q3HBox(m_Vbox2);
	m_Xoffs = new QLabel(QString("x-Offset: "), m_Hbox3);
	m_Xoffset = new QSpinBox(0, (int)m_Bmphand->ReturnWidth(), 1, m_Hbox3);
	m_Xoffset->setValue(0);
	m_Xoffset->show();
	m_Yoffs = new QLabel(QString("y-Offset: "), m_Hbox3);
	m_Yoffset = new QSpinBox(0, (int)m_Bmphand->ReturnHeight(), 1, m_Hbox3);
	m_Yoffset->show();
	m_Xoffset->setValue(0);
	m_Hbox3->show();
	m_Hbox4 = new Q3HBox(m_Vbox2);
	m_Xl = new QLabel(QString("x-Length: "), m_Hbox4);
	m_Xlength = new QSpinBox(0, (int)m_Bmphand->ReturnWidth(), 1, m_Hbox4);
	m_Xlength->show();
	m_Xlength->setValue((int)m_Bmphand->ReturnWidth());
	m_Yl = new QLabel(QString("y-Length: "), m_Hbox4);
	m_Ylength = new QSpinBox(0, (int)m_Bmphand->ReturnHeight(), 1, m_Hbox4);
	m_Ylength->show();
	m_Ylength->setValue((int)m_Bmphand->ReturnHeight());
	m_Hbox4->show();
	m_UpdateSubsect = new QPushButton("Update Subsection", m_Vbox2);
	m_Vbox2->show();
	m_Hbox2->show();
	m_CloseButton = new QPushButton("Close", m_Vbox1);

	DrawHisto();
	m_Histwindow->show();

	m_Vbox1->show();

	QObject_connect(m_UpdateSubsect, SIGNAL(clicked()), this, SLOT(SubsectUpdate()));
	QObject_connect(m_Workpict, SIGNAL(toggled(bool)), this, SLOT(PictToggled(bool)));
	QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	setFixedSize(m_Vbox1->size());
}

ShowHisto::~ShowHisto()
{
	delete m_Vbox1;
	delete m_Pictselect;
}

void ShowHisto::SubsectToggled()
{
	bool isset = m_Subsect->isChecked();
	if (isset)
	{
		m_Vbox2->show();
	}
	else
	{
		m_Vbox2->hide();
	}
	DrawHisto();
}

void ShowHisto::PictToggled(bool /* on */)
{
	DrawHisto();
}

void ShowHisto::SubsectUpdate()
{
	DrawHisto();
}

void ShowHisto::DrawHisto()
{
	//Point p;
	if (m_Bmppict->isChecked())
	{
		m_Bmphand->SwapBmpwork();
		if (m_Subsect->isChecked())
		{
			Point p;
			p.px = m_Xoffset->value();
			p.py = m_Yoffset->value();
			m_Bmphand->MakeHistogram(p, std::min((int)m_Bmphand->ReturnWidth() - m_Xoffset->value(), m_Xlength->value()), std::min((int)m_Bmphand->ReturnHeight() - m_Yoffset->value(), m_Ylength->value()), true);
		}
		else
		{
			m_Bmphand->MakeHistogram(true);
		}
		m_Bmphand->SwapBmpwork();
	}
	else
	{
		if (m_Subsect->isChecked())
		{
			Point p;
			p.px = m_Xoffset->value();
			p.py = m_Yoffset->value();
			m_Bmphand->MakeHistogram(p, std::min((int)m_Bmphand->ReturnWidth() - m_Xoffset->value(), m_Xlength->value()), std::min((int)m_Bmphand->ReturnHeight() - m_Yoffset->value(), m_Ylength->value()), true);
		}
		else
		{
			m_Bmphand->MakeHistogram(true);
		}
	}

	m_Histwindow->update();
}

void ShowHisto::SlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void ShowHisto::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
	m_Histwindow->HistoChanged(m_Bmphand->ReturnHistogram());
	DrawHisto();
}

void ShowHisto::Newloaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

Colorshower::Colorshower(int lx1, int ly1, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags)
{
	m_Lx = lx1;
	m_Ly = ly1;
	m_Fr = 0;
	m_Fg = 0;
	m_Fb = 0;
	m_Opac = 0.5f;
	setFixedSize(m_Lx, m_Ly);

	repaint();
}

void Colorshower::ColorChanged(float fr1, float fg1, float fb1, float opac1)
{
	m_Fr = fr1;
	m_Fg = fg1;
	m_Fb = fb1;
	m_Opac = opac1;

	repaint();
}

void Colorshower::paintEvent(QPaintEvent* e)
{
	QColor color;
	color.setRgb((int)(m_Opac * m_Fr * 255), (int)(m_Opac * m_Fg * 255), (int)(m_Opac * m_Fb * 255));
	QPainter painter(this);
	painter.setClipRect(e->rect());
	painter.fillRect(0, 0, m_Lx, m_Ly, color);
	color.setRgb((int)((1 - m_Opac + m_Opac * m_Fr) * 255), (int)((1 - m_Opac + m_Opac * m_Fg) * 255), (int)((1 - m_Opac + m_Opac * m_Fb) * 255));
	painter.setClipRect(e->rect());
	painter.fillRect(m_Lx / 4, m_Ly / 4, m_Lx / 2, m_Ly / 2, color);
}

TissueAdder::TissueAdder(bool modifyTissue, TissueTreeWidget* tissueTree, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_TissueTreeWidget(tissueTree)
{
	setModal(true);

	m_Modify = modifyTissue;

	m_Vbox1 = new Q3VBoxLayout(this);
	m_Hbox1 = new Q3HBoxLayout(this);
	m_Vbox1->addLayout(m_Hbox1);
	m_Tissuename = new QLabel(QString("Tissue Name: "), this);
	m_NameField = new QLineEdit(this);
	m_Hbox1->addWidget(m_Tissuename);
	m_Hbox1->addWidget(m_NameField);
	m_Hbox2 = new Q3HBoxLayout(this);
	m_Cs = new Colorshower(50, 50, this);
	m_Hbox2->addWidget(m_Cs);
	m_Vbox2 = new Q3VBoxLayout(this);
	m_Vbox3 = new Q3VBoxLayout(this);
	m_Vbox4 = new Q3VBoxLayout(this);

	m_R = new QSlider(Qt::Horizontal, this);
	m_R->setRange(0, 255);
	m_Red = new QLabel(QString("Red: "), this);

	m_G = new QSlider(Qt::Horizontal, this);
	m_G->setRange(0, 255);
	m_Green = new QLabel(QString("Green: "), this);

	m_B = new QSlider(Qt::Horizontal, this);
	m_B->setRange(0, 255);
	m_Blue = new QLabel(QString("Blue: "), this);
	m_SlTransp = new QSlider(Qt::Horizontal, this);
	m_SlTransp->setRange(0, 100);
	m_Opac = new QLabel(QString("Transp.: "), this);
	m_SbR = new QSpinBox(0, 255, 1, this);
	m_SbG = new QSpinBox(0, 255, 1, this);
	m_SbB = new QSpinBox(0, 255, 1, this);
	m_SbTransp = new QSpinBox(0, 100, 1, this);

	m_Vbox2->addWidget(m_Red);
	m_Vbox2->addWidget(m_Green);
	m_Vbox2->addWidget(m_Blue);
	m_Vbox2->addWidget(m_Opac);
	m_Vbox3->addWidget(m_R);
	m_Vbox3->addWidget(m_G);
	m_Vbox3->addWidget(m_B);
	m_Vbox3->addWidget(m_SlTransp);
	m_Vbox4->addWidget(m_SbR);
	m_Vbox4->addWidget(m_SbG);
	m_Vbox4->addWidget(m_SbB);
	m_Vbox4->addWidget(m_SbTransp);
	m_Hbox2->addLayout(m_Vbox2);
	m_Hbox2->addLayout(m_Vbox3);
	m_Hbox2->addLayout(m_Vbox4);
	m_Vbox1->addLayout(m_Hbox2);
	m_Hbox3 = new Q3HBoxLayout(this);

	m_AddTissue = new QPushButton("", this);
	m_CloseButton = new QPushButton("Close", this);
	m_Hbox3->addWidget(m_AddTissue);
	m_Hbox3->addWidget(m_CloseButton);
	m_Vbox1->addLayout(m_Hbox3);

	if (m_Modify)
	{
		m_AddTissue->setText("Modify Tissue");

		TissueInfo* tissue_info = TissueInfos::GetTissueInfo(m_TissueTreeWidget->GetCurrentType());
		m_NameField->setText(ToQ(tissue_info->m_Name));
		m_R->setValue(int(tissue_info->m_Color[0] * 255));
		m_G->setValue(int(tissue_info->m_Color[1] * 255));
		m_B->setValue(int(tissue_info->m_Color[2] * 255));
		m_SbR->setValue(int(tissue_info->m_Color[0] * 255));
		m_SbG->setValue(int(tissue_info->m_Color[1] * 255));
		m_SbB->setValue(int(tissue_info->m_Color[2] * 255));
		m_SlTransp->setValue(int(100 - tissue_info->m_Opac * 100));
		m_SbTransp->setValue(int(100 - tissue_info->m_Opac * 100));

		m_Fr1 = float(m_R->value()) / 255;
		m_Fg1 = float(m_G->value()) / 255;
		m_Fb1 = float(m_B->value()) / 255;
		m_Transp1 = float(m_SlTransp->value()) / 100;

		QObject_connect(m_R, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorR(int)));
		QObject_connect(m_G, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorG(int)));
		QObject_connect(m_B, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorB(int)));
		QObject_connect(m_SlTransp, SIGNAL(valueChanged(int)), this, SLOT(UpdateOpac(int)));
		QObject_connect(m_SbR, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorRsb(int)));
		QObject_connect(m_SbG, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorGsb(int)));
		QObject_connect(m_SbB, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorBsb(int)));
		QObject_connect(m_SbTransp, SIGNAL(valueChanged(int)), this, SLOT(UpdateOpacsb(int)));
		QObject_connect(m_AddTissue, SIGNAL(clicked()), this, SLOT(AddPressed()));
		QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
		QObject_connect(this, SIGNAL(ColorChanged(float,float,float,float)), m_Cs, SLOT(ColorChanged(float,float,float,float)));

		emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0 - m_Transp1);
	}
	else
	{
		m_AddTissue->setText("Add Tissue");

		m_R->setValue(0);
		m_G->setValue(0);
		m_B->setValue(0);
		m_SbR->setValue(0);
		m_SbG->setValue(0);
		m_SbB->setValue(0);
		m_SlTransp->setValue(50);
		m_SbTransp->setValue(50);

		m_Fr1 = float(m_R->value()) / 255;
		m_Fg1 = float(m_G->value()) / 255;
		m_Fb1 = float(m_B->value()) / 255;
		m_Transp1 = float(m_SlTransp->value()) / 100;

		QObject_connect(m_R, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorR(int)));
		QObject_connect(m_G, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorG(int)));
		QObject_connect(m_B, SIGNAL(valueChanged(int)), this, SLOT(UpdateColorB(int)));
		QObject_connect(m_SlTransp, SIGNAL(valueChanged(int)), this, SLOT(UpdateOpac(int)));
		QObject_connect(m_AddTissue, SIGNAL(clicked()), this, SLOT(AddPressed()));
		QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
		QObject_connect(this, SIGNAL(ColorChanged(float,float,float,float)), m_Cs, SLOT(ColorChanged(float,float,float,float)));
	}

}

TissueAdder::~TissueAdder()
{
	delete m_Cs;
	delete m_Vbox1;
	delete m_CloseButton;
	delete m_AddTissue;
	delete m_Red;
	delete m_Green;
	delete m_Blue;
	delete m_Opac;
	delete m_Tissuename;
	delete m_NameField;
	delete m_R;
	delete m_G;
	delete m_B;
	delete m_SlTransp;
	delete m_SbR;
	delete m_SbG;
	delete m_SbB;
	delete m_SbTransp;
}

class SignalBlock
{
	QObject* m_Obejct;
	bool m_Blocked;

public:
	SignalBlock(QObject* o) : m_Obejct(o)
	{
		m_Blocked = o->blockSignals(true);
	}
	~SignalBlock()
	{
		m_Obejct->blockSignals(m_Blocked);
	}
};

void TissueAdder::UpdateColorR(int v)
{
	SignalBlock guard(m_SbR);
	m_SbR->setValue(v);
	m_Fr1 = float(v) / 255;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateColorG(int v)
{
	SignalBlock guard(m_SbG);
	m_SbG->setValue(v);
	m_Fg1 = float(v) / 255;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateColorB(int v)
{
	SignalBlock guard(m_SbB);
	m_SbB->setValue(v);
	m_Fb1 = float(v) / 255;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateOpac(int v)
{
	SignalBlock guard(m_SbTransp);
	m_SbTransp->setValue(v);
	m_Transp1 = float(v) / 100;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateColorRsb(int v)
{
	SignalBlock guard(m_R);
	m_R->setValue(v);
	m_Fr1 = float(v) / 255;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateColorGsb(int v)
{
	SignalBlock guard(m_G);
	m_G->setValue(v);
	m_Fg1 = float(v) / 255;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateColorBsb(int v)
{
	SignalBlock guard(m_B);
	m_B->setValue(v);
	m_Fb1 = float(v) / 255;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::UpdateOpacsb(int v)
{
	SignalBlock guard(m_SlTransp);
	m_SlTransp->setValue(v);
	m_Transp1 = float(v) / 100;
	emit ColorChanged(m_Fr1, m_Fg1, m_Fb1, 1.0f - m_Transp1);
}

void TissueAdder::AddPressed()
{
	if (m_Modify)
	{
		if (!m_NameField->text().isEmpty())
		{
			tissues_size_t type = m_TissueTreeWidget->GetCurrentType();
			TissueInfo* tissue_info = TissueInfos::GetTissueInfo(type);
			QString old_name = ToQ(tissue_info->m_Name);
			if (old_name.compare(m_NameField->text(), Qt::CaseInsensitive) != 0 &&
					TissueInfos::GetTissueType(ToStd(m_NameField->text())) > 0)
			{
				QMessageBox::information(this, "iSeg", "A tissue with the same name already exists.");
				return;
			}
			TissueInfos::SetTissueName(type, ToStd(m_NameField->text()));
			tissue_info->m_Opac = 1.0f - m_Transp1;
			tissue_info->m_Color[0] = m_Fr1;
			tissue_info->m_Color[1] = m_Fg1;
			tissue_info->m_Color[2] = m_Fb1;
			// Update tissue name and icon in hierarchy
			m_TissueTreeWidget->UpdateTissueName(old_name, ToQ(tissue_info->m_Name));
			m_TissueTreeWidget->UpdateTissueIcons();
			close();
		}
		return;
	}
	else
	{
		if (TissueInfos::GetTissueCount() >= TISSUES_SIZE_MAX)
		{
			close();
		}
		else if (!m_NameField->text().isEmpty())
		{
			if (TissueInfos::GetTissueType(ToStd(m_NameField->text())) > 0)
			{
				QMessageBox::information(this, "iSeg", "A tissue with the same name already exists.");
				return;
			}
			TissueInfo tissue_info;
			tissue_info.m_Name = ToStd(m_NameField->text());
			tissue_info.m_Locked = false;
			tissue_info.m_Opac = 1.0f - m_Transp1;
			tissue_info.m_Color[0] = m_Fr1;
			tissue_info.m_Color[1] = m_Fg1;
			tissue_info.m_Color[2] = m_Fb1;
			TissueInfos::AddTissue(tissue_info);
			// Insert new tissue in hierarchy
			m_TissueTreeWidget->InsertItem(false, ToQ(tissue_info.m_Name));
			close();
		}
	}

	//	fprintf(fp,"%f %f %f",tissuecolor[1][0],tissuecolor[1][1],tissuecolor[1][2]);
	//	fclose(fp);

	}

TissueFolderAdder::TissueFolderAdder(TissueTreeWidget* tissueTree, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_TissueTreeWidget(tissueTree)
{
	setModal(true);

	setFixedWidth(235);
	setFixedHeight(161);

	m_HboxOverall = new Q3HBoxLayout(this);
	m_VboxOverall = new Q3VBoxLayout(this);
	m_HboxFolderName = new Q3HBoxLayout(this);
	m_HboxPushButtons = new Q3HBoxLayout(this);
	m_HboxOverall->addLayout(m_VboxOverall);

	// Folder name line edit
	m_NameLabel = new QLabel(QString("Folder Name: "), this);
	m_NameLineEdit = new QLineEdit(this);
	m_NameLineEdit->setText("New folder");
	m_HboxFolderName->addWidget(m_NameLabel);
	m_HboxFolderName->addWidget(m_NameLineEdit);
	m_VboxOverall->addLayout(m_HboxFolderName);

	// Buttons
	m_AddButton = new QPushButton("Add", this);
	m_CloseButton = new QPushButton("Close", this);
	m_HboxPushButtons->addWidget(m_AddButton);
	m_HboxPushButtons->addWidget(m_CloseButton);
	m_VboxOverall->addLayout(m_HboxPushButtons);

	QObject_connect(m_AddButton, SIGNAL(clicked()), this, SLOT(AddPressed()));
	QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
}

TissueFolderAdder::~TissueFolderAdder() { delete m_HboxOverall; }

void TissueFolderAdder::AddPressed()
{
	m_TissueTreeWidget->InsertItem(true, m_NameLineEdit->text());
	close();
}

TissueHierarchyWidget::TissueHierarchyWidget(TissueTreeWidget* tissueTree, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags), m_TissueTreeWidget(tissueTree)
{
	m_HboxOverall = new Q3HBoxLayout(this);
	m_VboxOverall = new Q3VBoxLayout();
	m_VboxHierarchyButtons = new Q3VBoxLayout();
	m_HboxOverall->addLayout(m_VboxOverall);

	// Hierarchy selection combo box
	m_HierarchyComboBox = new QComboBox(this);
	m_VboxOverall->addWidget(m_HierarchyComboBox);

	// Hierarchy buttons
	m_NewHierarchyButton = new QPushButton("New Hierarchy...", this);
	m_LoadHierarchyButton = new QPushButton("Load Hierarchy...", this);
	m_SaveHierarchyAsButton = new QPushButton("Save Hierarchy As...", this);
	m_RemoveHierarchyButton = new QPushButton("Remove Hierarchy", this);
	m_VboxHierarchyButtons->addWidget(m_NewHierarchyButton);
	m_VboxHierarchyButtons->addWidget(m_LoadHierarchyButton);
	m_VboxHierarchyButtons->addWidget(m_SaveHierarchyAsButton);
	m_VboxHierarchyButtons->addWidget(m_RemoveHierarchyButton);
	m_VboxOverall->addLayout(m_VboxHierarchyButtons);

	QObject_connect(m_HierarchyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(HierarchyChanged(int)));

	QObject_connect(m_NewHierarchyButton, SIGNAL(clicked()), this, SLOT(NewHierarchyPressed()));
	QObject_connect(m_LoadHierarchyButton, SIGNAL(clicked()), this, SLOT(LoadHierarchyPressed()));
	QObject_connect(m_SaveHierarchyAsButton, SIGNAL(clicked()), this, SLOT(SaveHierarchyAsPressed()));
	QObject_connect(m_RemoveHierarchyButton, SIGNAL(clicked()), this, SLOT(RemoveHierarchyPressed()));

	QObject_connect(m_TissueTreeWidget, SIGNAL(HierarchyListChanged()), this, SLOT(UpdateHierarchyComboBox()));

	UpdateHierarchyComboBox();

	setFixedHeight(m_HboxOverall->sizeHint().height());
}

void TissueHierarchyWidget::UpdateHierarchyComboBox()
{
	m_HierarchyComboBox->blockSignals(true);
	m_HierarchyComboBox->clear();
	auto hierarchy_names = m_TissueTreeWidget->GetHierarchyNamesPtr();
	for (const auto& name : *hierarchy_names)
	{
		m_HierarchyComboBox->addItem(name);
	}
	m_HierarchyComboBox->setCurrentIndex(m_TissueTreeWidget->GetSelectedHierarchy());
	m_HierarchyComboBox->blockSignals(false);
}

TissueHierarchyWidget::~TissueHierarchyWidget() { delete m_HboxOverall; }

bool TissueHierarchyWidget::HandleChangedHierarchy()
{
#if 0 // Version: Ask user whether to save xml
	if (tissueTreeWidget->get_hierarchy_modified())
	{
		int ret = QMessageBox::warning(this, "iSeg", QString("Do you want to save changes to hierarchy %1?").arg(tissueTreeWidget->get_current_hierarchy_name()), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes)
		{
			// Save hierarchy as...
			save_hierarchy_as_pressed();
		}
		else if (ret == QMessageBox::Cancel)
		{
			// Update internal representation of current hierarchy
			tissueTreeWidget->update_hierarchy();
			return false;
		}
		tissueTreeWidget->set_hierarchy_modified(false);
	}
	return true;
#else // Version: Commit change, but only save to xml if default hierarchy changed
	if (m_TissueTreeWidget->GetSelectedHierarchy() == 0 &&
			m_TissueTreeWidget->GetHierarchyModified())
	{
		int ret = QMessageBox::warning(this, "iSeg", QString("Do you want to save changes to hierarchy %1?").arg(m_TissueTreeWidget->GetCurrentHierarchyName()), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes)
		{
			// Save hierarchy as...
			if (!SaveHierarchyAsPressed())
			{
				return false;
			}
		}
		else if (ret == QMessageBox::Cancel)
		{
			// Update internal representation of current hierarchy
			m_TissueTreeWidget->UpdateHierarchy();
			return false;
		}
		else
		{
			// Reset default hierarchy
			m_TissueTreeWidget->ResetDefaultHierarchy();
		}
		m_TissueTreeWidget->UpdateTreeWidget();
	}
	else
	{
		// Update internal representation of current hierarchy
		m_TissueTreeWidget->UpdateHierarchy();
		m_TissueTreeWidget->SetHierarchyModified(false);
	}
	return true;
#endif
}

void TissueHierarchyWidget::HierarchyChanged(int index)
{
	if ((int)m_TissueTreeWidget->GetSelectedHierarchy() == index)
	{
		return;
	}

	// Save changes to current hierarchy
	if (!HandleChangedHierarchy())
	{
		// Select default hierarchy
		m_HierarchyComboBox->blockSignals(true);
		m_HierarchyComboBox->setCurrentIndex(0);
		m_HierarchyComboBox->blockSignals(false);
		return;
	}

	// Set selected hierarchy
	m_TissueTreeWidget->SetHierarchy(index);

	// Set selected item in combo box (may have changed during save hierarchy)
	m_HierarchyComboBox->blockSignals(true);
	m_HierarchyComboBox->setCurrentIndex(index);
	m_HierarchyComboBox->blockSignals(false);
}

void TissueHierarchyWidget::NewHierarchyPressed()
{
	// Save changes to current hierarchy
	if (!HandleChangedHierarchy())
	{
		return;
	}

	// Get hierarchy name
	bool ok = false;
	QString new_hierarchy_name = QInputDialog::getText("Hierarchy name", "Enter a name for the hierarchy:", QLineEdit::Normal, "New Hierarchy", &ok, this);
	if (!ok)
	{
		return;
	}

	// Create new hierarchy
	m_TissueTreeWidget->AddNewHierarchy(new_hierarchy_name);
}

void TissueHierarchyWidget::LoadHierarchyPressed()
{
	// Save changes to current hierarchy
	if (!HandleChangedHierarchy())
	{
		return;
	}

	// Get file name
	QString file_name = RecentPlaces::GetOpenFileName(this, "Load File", QString::null, "XML files (*.xml)");
	if (file_name.isNull())
	{
		return;
	}

	// Load hierarchy
	m_TissueTreeWidget->LoadHierarchy(file_name);

	// Set loaded hierarchy selected
	m_TissueTreeWidget->SetHierarchy(m_TissueTreeWidget->GetHierarchyCount() -
																	 1);
	UpdateHierarchyComboBox();
}

bool TissueHierarchyWidget::SaveHierarchyAsPressed()
{
	// Get file name
	QString file_name = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("XML files (*.xml)"));
	if (file_name.isNull())
	{
		return false;
	}

	// Get hierarchy name
	bool ok = false;
	QString new_hierarchy_name = QInputDialog::getText("Hierarchy name", "Enter a name for the hierarchy:", QLineEdit::Normal, m_TissueTreeWidget->GetCurrentHierarchyName(), &ok, this);
	if (!ok)
	{
		return false;
	}

	// Save hierarchy
	return m_TissueTreeWidget->SaveHierarchyAs(new_hierarchy_name, file_name);
}

void TissueHierarchyWidget::RemoveHierarchyPressed()
{
	// Remove current hierarchy
	m_TissueTreeWidget->RemoveCurrentHierarchy();
}

BitsStack::BitsStack(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags), m_Handler3D(hand3D)
{
	m_BitsNames = new QListWidget(this);
	m_Hbox1 = new Q3HBoxLayout(this);
	m_Hbox1->addWidget(m_BitsNames);
	m_Vbox1 = new Q3VBoxLayout(m_Hbox1);
	m_Pushwork = new QPushButton("Copy Target...", this);
	m_Pushbmp = new QPushButton("Copy Source...", this);
	m_Pushtissue = new QPushButton("Copy Tissue...", this);
	m_Popwork = new QPushButton("Paste Target", this);
	m_Popbmp = new QPushButton("Paste Source", this);
	m_Poptissue = new QPushButton("Paste Tissue", this);
	m_Deletebtn = new QPushButton("Delete", this);
	m_Saveitem = new QPushButton("Save Item(s)", this);
	m_Loaditem = new QPushButton("Open Item(s)", this);
	m_Vbox1->addWidget(m_Pushwork);
	m_Vbox1->addWidget(m_Pushbmp);
	m_Vbox1->addWidget(m_Pushtissue);
	m_Vbox1->addWidget(m_Popwork);
	m_Vbox1->addWidget(m_Popbmp);
	m_Vbox1->addWidget(m_Poptissue);
	m_Vbox1->addWidget(m_Deletebtn);
	m_Vbox1->addWidget(m_Saveitem);
	m_Vbox1->addWidget(m_Loaditem);

	m_BitsNames->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_BitsNames->setDragEnabled(true);
	m_BitsNames->setDragDropMode(QAbstractItemView::InternalMove);
	m_BitsNames->viewport()->setAcceptDrops(true);
	m_BitsNames->setDropIndicatorShown(true);

	QObject_connect(m_Pushwork, SIGNAL(clicked()), this, SLOT(PushworkPressed()));
	QObject_connect(m_Pushbmp, SIGNAL(clicked()), this, SLOT(PushbmpPressed()));
	QObject_connect(m_Pushtissue, SIGNAL(clicked()), this, SLOT(PushtissuePressed()));
	QObject_connect(m_Popwork, SIGNAL(clicked()), this, SLOT(PopworkPressed()));
	QObject_connect(m_Popbmp, SIGNAL(clicked()), this, SLOT(PopbmpPressed()));
	QObject_connect(m_Poptissue, SIGNAL(clicked()), this, SLOT(PoptissuePressed()));
	QObject_connect(m_Deletebtn, SIGNAL(clicked()), this, SLOT(DeletePressed()));
	QObject_connect(m_Saveitem, SIGNAL(clicked()), this, SLOT(SaveitemPressed()));
	QObject_connect(m_Loaditem, SIGNAL(clicked()), this, SLOT(LoaditemPressed()));

	m_Oldw = m_Handler3D->Width();
	m_Oldh = m_Handler3D->Height();

	m_Tissuenr = 0;
}

BitsStack::~BitsStack()
{
	delete m_BitsNames;
	delete m_Pushwork;
	delete m_Pushbmp;
	delete m_Pushtissue;
	delete m_Popwork;
	delete m_Popbmp;
	delete m_Poptissue;
	delete m_Deletebtn;
	delete m_Saveitem;
	delete m_Loaditem;
}

void BitsStack::TissuenrChanged(unsigned short i) { m_Tissuenr = i + 1; }

void BitsStack::PushHelper(bool source, bool target, bool tissue)
{
	if ((short)source + (short)target + (short)tissue != 1)
	{
		return;
	}

	QString data_name("");
	if (source)
	{
		data_name = QString("Source");
	}
	else if (target)
	{
		data_name = QString("Target");
	}
	else if (tissue)
	{
		data_name = QString("Tissue");
	}

	DataSelection data_selection;
	data_selection.bmp = source;
	data_selection.work = target;
	data_selection.tissues = tissue;
	emit BeginDataexport(data_selection, this);

	BitsStackPushdialog pushdialog(this);
	if (pushdialog.exec() == QDialog::Rejected)
	{
		emit EndDataexport(this);
		return;
	}

	if (pushdialog.GetPushcurrentslice())
	{
		// Copy current slice
		bool ok;
		QString new_text = QInputDialog::getText("Name", "Enter a name for the picture:", QLineEdit::Normal, "", &ok, this);
		new_text = new_text + QString(" (") +
							 QString::number(m_Handler3D->ActiveSlice() + 1) +
							 QString(")");
		while (ok &&
					 !m_BitsNames->findItems(new_text, Qt::MatchExactly).empty())
		{
			new_text = QInputDialog::getText("Name", "Enter a !new! name for the picture:", QLineEdit::Normal, "", &ok, this);
			new_text = new_text + QString(" (") +
								 QString::number(m_Handler3D->ActiveSlice() + 1) +
								 QString(")");
		}
		if (ok)
		{
			unsigned dummy;
			if (source)
			{
				dummy = m_Handler3D->PushstackBmp();
			}
			else if (target)
			{
				dummy = m_Handler3D->PushstackWork();
			}
			else if (tissue)
			{
				dummy = m_Handler3D->PushstackTissue(m_Tissuenr);
			}
			m_BitsNr[new_text] = dummy;
			m_BitsNames->addItem(new_text);
			emit StackChanged();
		}
	}
	else
	{
		// Copy slice range
		bool ok, matchfound, startok, endok;
		unsigned int startslice = pushdialog.GetStartslice(&startok);
		unsigned int endslice = pushdialog.GetEndslice(&endok);
		while (!startok || !endok || startslice > endslice || startslice < 1 ||
					 endslice > m_Handler3D->NumSlices())
		{
			QMessageBox::information(this, QString("Copy ") + data_name + QString("..."), "Please enter a valid slice range.\n");
			if (pushdialog.exec() == QDialog::Rejected)
			{
				emit EndDataexport(this);
				return;
			}
			startslice = pushdialog.GetStartslice(&startok);
			endslice = pushdialog.GetEndslice(&endok);
		}

		matchfound = true;
		QString new_text = QInputDialog::getText("Name", "Enter a name for the pictures:", QLineEdit::Normal, "", &ok, this);
		while (ok && matchfound)
		{
			matchfound = false;
			for (unsigned int slice = startslice; slice <= endslice; ++slice)
			{
				QString new_text_ext = new_text + QString(" (") +
															 QString::number(slice) + QString(")");
				if (!m_BitsNames->findItems(new_text_ext, Qt::MatchExactly).empty())
				{
					matchfound = true;
					new_text = QInputDialog::getText("Name", "Enter a !new! name for the pictures:", QLineEdit::Normal, "", &ok, this);
					break;
				}
			}
		}
		if (ok)
		{
			for (unsigned int slice = startslice; slice <= endslice; ++slice)
			{
				QString new_text_ext = new_text + QString(" (") +
															 QString::number(slice) + QString(")");
				unsigned dummy;
				if (source)
				{
					dummy = m_Handler3D->PushstackBmp(slice - 1);
				}
				else if (target)
				{
					dummy = m_Handler3D->PushstackWork(slice - 1);
				}
				else if (tissue)
				{
					dummy = m_Handler3D->PushstackTissue(m_Tissuenr, slice - 1);
				}
				m_BitsNr[new_text_ext] = dummy;
				m_BitsNames->addItem(new_text_ext);
			}
			emit StackChanged();
		}
	}

	emit EndDataexport(this);
}

void BitsStack::PushworkPressed() { PushHelper(false, true, false); }

void BitsStack::PushtissuePressed() { PushHelper(false, false, true); }

void BitsStack::PushbmpPressed() { PushHelper(true, false, false); }

void BitsStack::PopHelper(bool source, bool target, bool tissue)
{
	if ((short)source + (short)target + (short)tissue != 1)
	{
		return;
	}

	QString data_name("");
	if (source)
	{
		data_name = QString("Source");
	}
	else if (target)
	{
		data_name = QString("Target");
	}
	else if (tissue)
	{
		data_name = QString("Tissue");
	}

	QList<QListWidgetItem*> selected_items = m_BitsNames->selectedItems();
	if (selected_items.empty())
	{
		return;
	}
	else if (m_Handler3D->ActiveSlice() + selected_items.size() >
					 m_Handler3D->NumSlices())
	{
		QMessageBox::information(this, QString("Paste ") + data_name, "The number of images to be pasted starting at the\ncurrent slice "
																																	"would surpass the end of the data stack.\n");
		return;
	}

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = source;
	data_selection.work = target;
	data_selection.tissues = tissue;
	data_selection.allSlices = selected_items.size() > 1;
	emit BeginDatachange(data_selection, this);

	unsigned int slice = m_Handler3D->ActiveSlice();
	if (source)
	{
		for (QList<QListWidgetItem*>::iterator iter = selected_items.begin();
				 iter != selected_items.end(); ++iter)
		{
			m_Handler3D->GetstackBmp(slice++, m_BitsNr[(*iter)->text()]);
		}
	}
	else if (target)
	{
		for (QList<QListWidgetItem*>::iterator iter = selected_items.begin();
				 iter != selected_items.end(); ++iter)
		{
			m_Handler3D->GetstackWork(slice++, m_BitsNr[(*iter)->text()]);
		}
	}
	else if (tissue)
	{
		for (QList<QListWidgetItem*>::iterator iter = selected_items.begin();
				 iter != selected_items.end(); ++iter)
		{
			m_Handler3D->GetstackTissue(slice++, m_BitsNr[(*iter)->text()], m_Tissuenr, true);
		}
	}

	emit EndDatachange(this);
}

void BitsStack::PopworkPressed() { PopHelper(false, true, false); }

void BitsStack::PopbmpPressed() { PopHelper(true, false, false); }

void BitsStack::PoptissuePressed() { PopHelper(false, false, true); }

void BitsStack::LoaditemPressed()
{
	QStringList selected_files = QFileDialog::getOpenFileNames(
			this, "Select one or more files to open", QString::null, "Stackitems (*.stk)\nAll (*.*)");
	if (selected_files.isEmpty())
	{
		return;
	}

	if (selected_files.size() > 1)
	{
		// Load multiple items
		bool ok;
		bool matchfound = true;
		QString new_text = QInputDialog::getText("Name", "Enter a name for the pictures:", QLineEdit::Normal, "", &ok, this);
		while (ok && matchfound)
		{
			matchfound = false;
			unsigned int suffix = 0;
			for (QStringList::Iterator iter = selected_files.begin();
					 iter != selected_files.end(); ++iter)
			{
				QString new_text_ext = new_text + QString::number(suffix++);
				if (!m_BitsNames->findItems(new_text_ext, Qt::MatchExactly).empty())
				{
					matchfound = true;
					new_text = QInputDialog::getText("Name", "Enter a !new! name for the pictures:", QLineEdit::Normal, "", &ok, this);
					break;
				}
			}
		}

		if (ok)
		{
			unsigned int suffix = 0;
			for (QStringList::Iterator iter = selected_files.begin();
					 iter != selected_files.end(); ++iter)
			{
				QString new_text_ext = new_text + QString::number(suffix++);
				unsigned dummy = m_Handler3D->Loadstack(iter->ascii());
				if (dummy != 123456)
				{
					m_BitsNr[new_text_ext] = dummy;
					m_BitsNames->addItem(new_text_ext);
					emit StackChanged();
				}
			}
		}
	}
	else if (!selected_files.empty())
	{
		// Load single item
		bool ok;
		QString new_text = QInputDialog::getText("Name", "Enter a name for the picture:", QLineEdit::Normal, "", &ok, this);
		while (ok &&
					 !m_BitsNames->findItems(new_text, Qt::MatchExactly).empty())
		{
			new_text = QInputDialog::getText("Name", "Enter a !new! name for the picture:", QLineEdit::Normal, "", &ok, this);
		}

		if (ok)
		{
			unsigned dummy;
			dummy = m_Handler3D->Loadstack(selected_files[0].ascii());
			if (dummy != 123456)
			{
				m_BitsNr[new_text] = dummy;
				m_BitsNames->addItem(new_text);
				emit StackChanged();
			}
		}
	}
}

void BitsStack::SaveitemPressed()
{
	QList<QListWidgetItem*> selected_items = m_BitsNames->selectedItems();
	if (selected_items.empty())
	{
		return;
	}

	QString savefilename = QFileDialog::getSaveFileName(
			this, "Save", "Stackitems (*.stk)\n", QString::null); //, filename);
	if (savefilename.endsWith(QString(".stk")))
	{
		savefilename.remove(savefilename.length() - 4, 4);
	}
	if (savefilename.isEmpty())
	{
		return;
	}

	if (selected_items.size() > 1)
	{
		unsigned int field_width = 0;
		int tmp = selected_items.size();
		while (tmp > 0)
		{
			tmp /= 10;
			field_width++;
		}
		unsigned int suffix = 1;
		for (QList<QListWidgetItem*>::iterator iter = selected_items.begin();
				 iter != selected_items.end(); ++iter)
		{
			QString savefilename_ext =
					savefilename +
					QString("%1").arg(suffix++, field_width, 10, QChar('0')) +
					QString(".stk");
			m_Handler3D->Savestack(m_BitsNr[(*iter)->text()], savefilename_ext.ascii());
		}
	}
	else
	{
		QString savefilename_ext = savefilename + QString(".stk");
		m_Handler3D->Savestack(m_BitsNr[selected_items[0]->text()], savefilename_ext.ascii());
	}
}

void BitsStack::DeletePressed()
{
	QList<QListWidgetItem*> selected_items = m_BitsNames->selectedItems();
	if (selected_items.empty())
	{
		return;
	}

	for (QList<QListWidgetItem*>::iterator iter = selected_items.begin();
			 iter != selected_items.end(); ++iter)
	{
		m_Handler3D->Removestack(m_BitsNr[(*iter)->text()]);
		m_BitsNr.erase((*iter)->text());
		delete m_BitsNames->takeItem(m_BitsNames->row(*iter));
		emit StackChanged();
	}
}

QMap<QString, unsigned int>* BitsStack::ReturnBitsnr() { return &m_BitsNr; }

void BitsStack::Newloaded()
{
	if (m_Oldw != m_Handler3D->Width() || m_Oldh != m_Handler3D->Height())
	{
		m_BitsNames->clear();
		m_BitsNr.clear();
	}
	m_Oldw = m_Handler3D->Width();
	m_Oldh = m_Handler3D->Height();
	emit StackChanged();
}

void BitsStack::ClearStack()
{
	m_Handler3D->ClearStack();
	m_BitsNames->clear();
	m_BitsNr.clear();
	emit StackChanged();
}

FILE* BitsStack::SaveProj(FILE* fp)
{
	int size = int(m_BitsNr.size());
	fwrite(&size, 1, sizeof(int), fp);

	for (int j = 0; j < m_BitsNames->count(); ++j)
	{
		QString item_name = m_BitsNames->item(j)->text();
		int size1 = item_name.length();
		fwrite(&size1, 1, sizeof(int), fp);
		fwrite(item_name.ascii(), 1, sizeof(char) * size1, fp);
		unsigned int stack_idx = m_BitsNr[item_name];
		fwrite(&stack_idx, 1, sizeof(unsigned), fp);
	}

	return fp;
}

FILE* BitsStack::LoadProj(FILE* fp)
{
	m_Oldw = m_Handler3D->Width();
	m_Oldh = m_Handler3D->Height();

	char name[100];
	int size;
	fread(&size, sizeof(int), 1, fp);

	m_BitsNr.clear();
	m_BitsNames->clear();

	for (int j = 0; j < size; j++)
	{
		int size1;
		unsigned dummy;
		fread(&size1, sizeof(int), 1, fp);
		fread(name, sizeof(char) * size1, 1, fp);
		name[size1] = '\0';
		QString s(name);
		m_BitsNames->addItem(s);

		fread(&dummy, sizeof(unsigned), 1, fp);
		m_BitsNr[s] = dummy;
	}
	emit StackChanged();

	return fp;
}

ExtoverlayWidget::ExtoverlayWidget(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags), m_Handler3D(hand3D)
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Alpha = 0.0f;
	m_SliderMax = 1000;

	m_SliderPrecision = 0;
	int tmp = m_SliderMax;
	while (tmp > 1)
	{
		tmp /= 10;
		m_SliderPrecision++;
	}

	m_HboxOverall = new Q3HBoxLayout(this);
	m_VboxOverall = new Q3VBoxLayout();
	m_HboxAlpha = new Q3HBoxLayout();
	m_HboxDisplaySrcTgt = new Q3HBoxLayout();
	m_HboxOverall->addLayout(m_VboxOverall);

	// Dataset selection combo box
	m_DatasetComboBox = new QComboBox(this);
	m_VboxOverall->addWidget(m_DatasetComboBox);

	// Dataset buttons
	m_LoadDatasetButton = new QPushButton("Load Dataset...", this);
	m_VboxOverall->addWidget(m_LoadDatasetButton);

	// Alpha value slider
	m_LbAlpha = new QLabel(QString("Alpha: "));
	m_SlAlpha = new QSlider(Qt::Horizontal);
	m_SlAlpha->setRange(0, m_SliderMax);
	m_SlAlpha->setValue(m_Alpha * m_SliderMax);
	m_LeAlpha = new QLineEdit(QString::number((int)0));
	m_LeAlpha->setAlignment(Qt::AlignRight);
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
	QString text = m_LeAlpha->text();
	QFontMetrics fm = m_LeAlpha->fontMetrics();
	QRect rect = fm.boundingRect(text);
	m_LeAlpha->setFixedSize(rect.width() + 10, rect.height() + 4);
	m_HboxAlpha->addWidget(m_LbAlpha);
	m_HboxAlpha->addWidget(m_SlAlpha);
	m_HboxAlpha->addWidget(m_LeAlpha);
	m_VboxOverall->addLayout(m_HboxAlpha);

	// Display checkboxes
	m_SrcCheckBox = new QCheckBox(QString("Source"));
	m_TgtCheckBox = new QCheckBox(QString("Target"));
	m_HboxDisplaySrcTgt->addWidget(m_SrcCheckBox);
	m_HboxDisplaySrcTgt->addWidget(m_TgtCheckBox);
	m_VboxOverall->addLayout(m_HboxDisplaySrcTgt);

	setFixedHeight(m_HboxOverall->sizeHint().height());

	QObject_connect(m_DatasetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(DatasetChanged(int)));
	QObject_connect(m_LoadDatasetButton, SIGNAL(clicked()), this, SLOT(LoadDatasetPressed()));
	QObject_connect(m_LeAlpha, SIGNAL(editingFinished()), this, SLOT(AlphaChanged()));
	QObject_connect(m_SlAlpha, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SrcCheckBox, SIGNAL(clicked()), this, SLOT(SourceToggled()));
	QObject_connect(m_TgtCheckBox, SIGNAL(clicked()), this, SLOT(TargetToggled()));

	Initialize();
}

void ExtoverlayWidget::Initialize()
{
	m_DatasetComboBox->blockSignals(true);
	m_DatasetComboBox->clear();
	m_DatasetComboBox->blockSignals(false);

	m_DatasetNames.clear();
	m_DatasetFilepaths.clear();
	AddDataset("(None)");
	m_SelectedDataset = 0;

	m_Alpha = 0.0f;
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
	m_SlAlpha->setValue(m_Alpha * m_SliderMax);
	emit OverlayalphaChanged(m_Alpha);
}

void ExtoverlayWidget::Newloaded() { m_Handler3D->NewOverlay(); }

void ExtoverlayWidget::AddDataset(const QString& path)
{
	m_DatasetNames.push_back(QFileInfo(path).fileName());
	m_DatasetFilepaths.push_back(path);

	m_DatasetComboBox->blockSignals(true);
	m_DatasetComboBox->addItem(*m_DatasetNames.rbegin());
	m_DatasetComboBox->blockSignals(false);

	m_DatasetComboBox->setCurrentIndex((m_DatasetNames.size() - 1));
}

void ExtoverlayWidget::RemoveDataset(unsigned short idx)
{
	m_DatasetNames.erase(m_DatasetNames.begin() + idx);
	m_DatasetFilepaths.erase(m_DatasetFilepaths.begin() + idx);

	m_DatasetComboBox->blockSignals(true);
	m_DatasetComboBox->removeItem(idx);
	m_DatasetComboBox->blockSignals(false);

	m_DatasetComboBox->setCurrentIndex(0);
}

void ExtoverlayWidget::ReloadOverlay()
{
	bool ok = true;
	if (m_SelectedDataset == 0 && (m_SrcCheckBox->isChecked() || m_TgtCheckBox->isChecked()))
	{
		m_Handler3D->ClearOverlay();
	}
	else if (m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".raw"), Qt::CaseInsensitive))
	{
		ok = m_Handler3D->ReadRawOverlay(m_DatasetFilepaths[m_SelectedDataset], 8, m_Handler3D->ActiveSlice());
	}
	else if (m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".vtk"), Qt::CaseInsensitive) ||
					 m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".vti"), Qt::CaseInsensitive))
	{
		ok = m_Handler3D->ReadOverlay(m_DatasetFilepaths[m_SelectedDataset], m_Handler3D->ActiveSlice());
	}
	else if (m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".nii"), Qt::CaseInsensitive) ||
					 m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".hdr"), Qt::CaseInsensitive) ||
					 m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".img"), Qt::CaseInsensitive) ||
					 m_DatasetFilepaths[m_SelectedDataset].endsWith(QString(".nii.gz"), Qt::CaseInsensitive))
	{
		ok = m_Handler3D->ReadOverlay(m_DatasetFilepaths[m_SelectedDataset], m_Handler3D->ActiveSlice());
	}

	if (!ok)
	{
		m_Handler3D->ClearOverlay();
	}

	emit OverlayChanged();
}

void ExtoverlayWidget::SlicenrChanged() { ReloadOverlay(); }

void ExtoverlayWidget::DatasetChanged(int index)
{
	if (index < 0 || index >= m_DatasetNames.size())
	{
		return;
	}
	m_SelectedDataset = index;
	ReloadOverlay();
}

ExtoverlayWidget::~ExtoverlayWidget()
{
	m_DatasetNames.clear();
	m_DatasetFilepaths.clear();
	delete m_HboxOverall;
}

void ExtoverlayWidget::LoadDatasetPressed()
{
	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString::null, "VTK (*.vti *.vtk)\n"
																																												 "Raw files (*.raw)\n"
																																												 "NIFTI (*.nii *.hdr *.img *.nii.gz)\n"
																																												 "All (*.*)");
	if (!loadfilename.isEmpty())
	{
		AddDataset(loadfilename);
	}
}

void ExtoverlayWidget::AlphaChanged()
{
	bool b1;
	float value = m_LeAlpha->text().toFloat(&b1);
	if (!b1)
	{
		m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
		m_SlAlpha->setValue(m_Alpha * m_SliderMax);
		QApplication::beep();
		return;
	}

	if (value < 0.0f)
	{
		value = 0.0f;
	}
	else if (value > 1.0f)
	{
		value = 1.0f;
	}
	m_Alpha = value;
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
	m_SlAlpha->setValue(m_Alpha * m_SliderMax);
	emit OverlayalphaChanged(m_Alpha);
}

void ExtoverlayWidget::SliderChanged(int newval)
{
	m_Alpha = newval / (float)m_SliderMax;
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
	emit OverlayalphaChanged(m_Alpha);
}

void ExtoverlayWidget::SourceToggled()
{
	bool isset = m_SrcCheckBox->isChecked();
	emit BmpoverlayvisibleChanged(isset);
}

void ExtoverlayWidget::TargetToggled()
{
	bool isset = m_TgtCheckBox->isChecked();
	emit WorkoverlayvisibleChanged(isset);
}

BitsStackPushdialog::BitsStackPushdialog(QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags)
{
	setModal(false);

	m_Vboxoverall = new Q3VBox(this);
	m_Hboxparams = new Q3HBox(m_Vboxoverall);
	m_Vboxsliceselection = new Q3VBox(m_Hboxparams);
	m_Vboxdelimiter = new Q3VBox(m_Hboxparams);
	m_Hboxslicerange = new Q3HBox(m_Hboxparams);
	m_Vboxslicerangelabels = new Q3VBox(m_Hboxslicerange);
	m_Vboxslicerangelineedits = new Q3VBox(m_Hboxslicerange);
	m_Hboxpushbuttons = new Q3HBox(m_Vboxoverall);

	m_RbCurrentslice = new QRadioButton(QString("Current slice"), m_Vboxsliceselection);
	m_RbMultislices = new QRadioButton(QString("Slice range"), m_Vboxsliceselection);
	m_Slicegroup = new QButtonGroup(this);
	m_Slicegroup->insert(m_RbCurrentslice);
	m_Slicegroup->insert(m_RbMultislices);
	m_RbCurrentslice->setChecked(TRUE);

	m_LbStartslice = new QLabel(QString("Start slice:"), m_Vboxslicerangelabels);
	m_LbEndslice = new QLabel(QString("End slice:"), m_Vboxslicerangelabels);
	m_LeStartslice = new QLineEdit(QString(""), m_Vboxslicerangelineedits);
	m_LeEndslice = new QLineEdit(QString(""), m_Vboxslicerangelineedits);

	m_Pushexec = new QPushButton(QString("OK"), m_Hboxpushbuttons);
	m_Pushcancel = new QPushButton(QString("Cancel"), m_Hboxpushbuttons);

	m_Vboxoverall->setMargin(5);
	m_Hboxslicerange->setMargin(5);
	m_Vboxsliceselection->setMargin(5);
	m_Hboxpushbuttons->setMargin(5);
	m_Vboxdelimiter->setMargin(5);
	m_Vboxsliceselection->layout()->setAlignment(Qt::AlignTop);
	m_Vboxdelimiter->setFixedSize(15, m_Hboxslicerange->height());
	this->setFixedSize(m_Vboxoverall->sizeHint());

	m_Hboxslicerange->hide();

	QObject_connect(m_Pushexec, SIGNAL(clicked()), this, SLOT(accept()));
	QObject_connect(m_Pushcancel, SIGNAL(clicked()), this, SLOT(reject()));
	QObject_connect(m_Slicegroup, SIGNAL(buttonClicked(int)), this, SLOT(SliceselectionChanged()));
}

BitsStackPushdialog::~BitsStackPushdialog()
{
	delete m_Slicegroup;
	delete m_Vboxoverall;
}

bool BitsStackPushdialog::GetPushcurrentslice()
{
	return m_RbCurrentslice->isChecked();
}

unsigned int BitsStackPushdialog::GetStartslice(bool* ok)
{
	return m_LeStartslice->text().toUInt(ok);
}

unsigned int BitsStackPushdialog::GetEndslice(bool* ok)
{
	return m_LeEndslice->text().toUInt(ok);
}

void BitsStackPushdialog::SliceselectionChanged()
{
	if (m_RbCurrentslice->isChecked())
	{
		m_Hboxslicerange->hide();
	}
	else
	{
		m_Hboxslicerange->show();
	}
}

//xxxxxxxxxxxxxxxxxxxxxx histo xxxxxxxxxxxxxxxxxxxxxxxxxxxx

ZoomWidget::ZoomWidget(double zoom1, QDir picpath, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags)
{
	m_Zoom = zoom1;
	m_Vbox1 = new Q3VBoxLayout(this);
	m_Pushzoomin = new QPushButton(QIcon(picpath.absoluteFilePath(QString("zoomin.png"))), "Zoom in", this);
	m_Pushzoomout = new QPushButton(QIcon(picpath.absoluteFilePath(QString("zoomout.png"))), "Zoom out", this);
	m_Pushunzoom = new QPushButton(QIcon(picpath.absoluteFilePath(QString("unzoom.png"))), "Unzoom", this);

	m_ZoomF = new QLabel(QString("x"), this);
	m_LeZoomF = new QLineEdit(QString::number(m_Zoom, 'g', 4), this);
	m_LeZoomF->setFixedWidth(80);

	m_Vbox1->addWidget(m_Pushzoomin);
	m_Vbox1->addWidget(m_Pushzoomout);
	m_Vbox1->addWidget(m_Pushunzoom);

	m_Hbox1 = new Q3HBoxLayout(m_Vbox1);
	m_Hbox1->addWidget(m_ZoomF);
	m_Hbox1->addWidget(m_LeZoomF);

	setFixedHeight(m_Vbox1->sizeHint().height());

	QObject_connect(m_Pushzoomin, SIGNAL(clicked()), this, SLOT(ZoominPushed()));
	QObject_connect(m_Pushzoomout, SIGNAL(clicked()), this, SLOT(ZoomoutPushed()));
	QObject_connect(m_Pushunzoom, SIGNAL(clicked()), this, SLOT(UnzoomPushed()));
	QObject_connect(m_LeZoomF, SIGNAL(editingFinished()), this, SLOT(LeZoomChanged()));
}

ZoomWidget::~ZoomWidget()
{
	delete m_Vbox1;
	delete m_Pushzoomin;
	delete m_Pushzoomout;
	delete m_Pushunzoom;
	delete m_LeZoomF;
	delete m_ZoomF;
}

double ZoomWidget::GetZoom() const { return m_Zoom; }

void ZoomWidget::ZoomChanged(double z)
{
	m_Zoom = z;
	m_ZoomF->setText(QString("x")); //+QString::number(zoom,'g',4));
	m_LeZoomF->setText(QString::number(m_Zoom, 'g', 4));
	emit SetZoom(m_Zoom);
}

void ZoomWidget::ZoominPushed()
{
	m_Zoom = 2 * m_Zoom;
	m_ZoomF->setText(QString("x")); //+QString::number(zoom,'g',4));
	m_LeZoomF->setText(QString::number(m_Zoom, 'g', 4));
	emit SetZoom(m_Zoom);
}

void ZoomWidget::ZoomoutPushed()
{
	m_Zoom = 0.5 * m_Zoom;
	m_ZoomF->setText(QString("x")); //+QString::number(zoom,'g',4));
	m_LeZoomF->setText(QString::number(m_Zoom, 'g', 4));
	emit SetZoom(m_Zoom);
}

void ZoomWidget::UnzoomPushed()
{
	m_Zoom = 1.0;
	m_ZoomF->setText(QString("x")); //+QString::number(zoom,'g',4));
	m_LeZoomF->setText(QString::number(m_Zoom, 'g', 4));
	emit SetZoom(m_Zoom);
}

void ZoomWidget::LeZoomChanged()
{
	bool b1;
	float zoom1 = m_LeZoomF->text().toFloat(&b1);
	if (b1)
	{
		m_Zoom = zoom1;
		m_ZoomF->setText(QString("x")); //+QString::number(zoom,'g',4));
		emit SetZoom(m_Zoom);
	}
	else
	{
		if (m_LeZoomF->text() != QString("."))
		{
			QApplication::beep();
		}
	}
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

/*QHBoxLayout_fixedheight::QHBoxLayout_fixedheight(QWidget *parent,int margin,int spacing,const char *name)
	: QHBoxLayout(parent,margin,spacing,name)
{
}

QSizePolicy::ExpandData QHBoxLayout_fixedheight::expanding() const
{
	return QSizePolicy::Horizontal;
}

QSize QHBoxLayout_fixedheight::maximumSize() const
{
	QSize qs=sizeHint();
	qs.setHeight(15);
	return qs;
}*/

//--------------------------------------------------

ImageMath::ImageMath(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Vbox1 = new Q3VBox(this);

	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Imgorval = new QButtonGroup(this);
	//	imgorval->hide();
	m_RbImg = new QRadioButton(QString("Image"), m_Hbox1);
	m_RbVal = new QRadioButton(QString("Value"), m_Hbox1);
	m_Imgorval->insert(m_RbImg);
	m_Imgorval->insert(m_RbVal);
	m_RbImg->setChecked(TRUE);
	m_RbImg->show();
	m_RbVal->show();
	m_Hbox1->show();

	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_LbVal = new QLabel(QString("Value: "), m_Hbox2);
	m_LbVal->show();
	m_LeVal = new QLineEdit(QString::number((int)0), m_Hbox2);
	m_LeVal->show();
	m_Val = 0;
	m_Hbox2->show();

	m_Allslices = new QCheckBox(QString("3D"), m_Vbox1);

	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_DoAdd = new QPushButton("Add.", m_Hbox3);
	m_DoAdd->show();
	m_DoSub = new QPushButton("Subt.", m_Hbox3);
	m_DoSub->show();
	m_DoMult = new QPushButton("Mult.", m_Hbox3);
	m_DoMult->show();
	m_DoNeg = new QPushButton("Neg.", m_Hbox3);
	m_DoNeg->show();
	m_Hbox3->show();

	m_CloseButton = new QPushButton("Close", m_Vbox1);

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	setFixedSize(m_Vbox1->size());

	ImgorvalChanged(0);

	QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_DoAdd, SIGNAL(clicked()), this, SLOT(AddPushed()));
	QObject_connect(m_DoSub, SIGNAL(clicked()), this, SLOT(SubPushed()));
	QObject_connect(m_DoMult, SIGNAL(clicked()), this, SLOT(MultPushed()));
	QObject_connect(m_DoNeg, SIGNAL(clicked()), this, SLOT(NegPushed()));
	QObject_connect(m_Imgorval, SIGNAL(buttonClicked(int)), this, SLOT(ImgorvalChanged(int)));
	QObject_connect(m_LeVal, SIGNAL(editingFinished()), this, SLOT(ValueChanged()));

	}

ImageMath::~ImageMath() { delete m_Vbox1; }

void ImageMath::ImgorvalChanged(int)
{
	if (m_RbVal->isChecked())
	{
		m_Hbox2->show();
	}
	else
	{
		m_Hbox2->hide();
	}
}

void ImageMath::AddPushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		if (m_RbVal->isChecked())
		{
			m_Handler3D->BmpAdd(m_Val);
		}
		else
		{
			m_Handler3D->BmpSum();
		}
	}
	else
	{
		if (m_RbVal->isChecked())
		{
			m_Bmphand->BmpAdd(m_Val);
		}
		else
		{
			m_Bmphand->BmpSum();
		}
	}

	emit EndDatachange(this);
}

void ImageMath::SubPushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		if (m_RbVal->isChecked())
		{
			m_Handler3D->BmpAdd(-m_Val);
		}
		else
		{
			m_Handler3D->BmpDiff();
		}
	}
	else
	{
		if (m_RbVal->isChecked())
		{
			m_Bmphand->BmpAdd(-m_Val);
		}
		else
		{
			m_Bmphand->BmpDiff();
		}
	}

	emit EndDatachange(this);
}

void ImageMath::MultPushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		if (m_RbVal->isChecked())
		{
			m_Handler3D->BmpMult(m_Val);
		}
		else
		{
			m_Handler3D->BmpMult();
		}
	}
	else
	{
		if (m_RbVal->isChecked())
		{
			m_Bmphand->BmpMult(m_Val);
		}
		else
		{
			m_Bmphand->BmpMult();
		}
	}

	emit EndDatachange(this);
}

void ImageMath::NegPushed()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		m_Handler3D->BmpNeg();
	}
	else
	{
		m_Bmphand->BmpNeg();
	}

	emit EndDatachange(this);
}

void ImageMath::SlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void ImageMath::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
}

void ImageMath::ValueChanged()
{
	bool b1;
	float value = m_LeVal->text().toFloat(&b1);
	if (b1)
	{
		m_Val = value;
	}
	else
	{
		if (m_LeVal->text() != QString("."))
		{
			QApplication::beep();
		}
	}
}

//--------------------------------------------------

ImageOverlay::ImageOverlay(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_BkpWork = (float*)malloc(sizeof(float) * m_Bmphand->ReturnArea());
	m_Bmphand->Copyfromwork(m_BkpWork);

	m_Alpha = 0.0f;
	m_SliderMax = 1000;

	m_SliderPrecision = 0;
	int tmp = m_SliderMax;
	while (tmp > 1)
	{
		tmp /= 10;
		m_SliderPrecision++;
	}

	m_Vbox1 = new Q3VBox(this);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_LbAlpha = new QLabel(QString("Alpha: "), m_Hbox1);
	m_LbAlpha->show();
	m_SlAlpha = new QSlider(Qt::Horizontal, m_Hbox1);
	m_SlAlpha->setRange(0, m_SliderMax);
	m_SlAlpha->setValue(m_Alpha * m_SliderMax);
	m_SlAlpha->show();
	m_LeAlpha = new QLineEdit(QString::number((int)0), m_Hbox1);
	m_LeAlpha->setAlignment(Qt::AlignRight);
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
	QString text = m_LeAlpha->text();
	QFontMetrics fm = m_LeAlpha->fontMetrics();
	QRect rect = fm.boundingRect(text);
	m_LeAlpha->setFixedSize(rect.width() + 10, rect.height() + 4);
	m_LeAlpha->show();
	m_Hbox1->show();

	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Allslices = new QCheckBox(QString("3D"), m_Hbox2);
	m_Allslices->show();
	m_Hbox2->show();

	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_CloseButton = new QPushButton("Close", m_Hbox3);
	m_CloseButton->show();
	m_ApplyButton = new QPushButton("Apply", m_Hbox3);
	m_ApplyButton->show();
	m_Hbox3->show();

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());
	setFixedSize(m_Vbox1->size());

	QObject_connect(m_CloseButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_ApplyButton, SIGNAL(clicked()), this, SLOT(ApplyPushed()));
	QObject_connect(m_LeAlpha, SIGNAL(editingFinished()), this, SLOT(AlphaChanged()));
	QObject_connect(m_SlAlpha, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));

	}

ImageOverlay::~ImageOverlay()
{
	delete m_Vbox1;
	free(m_BkpWork);
}

void ImageOverlay::closeEvent(QCloseEvent* e)
{
	QDialog::closeEvent(e);

	if (e->isAccepted())
	{
		// Undo overlay
		DataSelection data_selection;
		data_selection.allSlices = false;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this, false);

		m_Bmphand->Copy2work(m_BkpWork, m_Bmphand->ReturnMode(false));

		emit EndDatachange(this, iseg::NoUndo);
	}
}

void ImageOverlay::ApplyPushed()
{
	// Swap modified work with original work backup for undo operation
	m_BkpWork = m_Bmphand->SwapWorkPointer(m_BkpWork);

	DataSelection data_selection;
	data_selection.allSlices = m_Allslices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Allslices->isChecked())
	{
		m_Handler3D->BmpOverlay(m_Alpha);
	}
	else
	{
		m_Bmphand->BmpOverlay(m_Alpha);
	}
	m_Bmphand->Copyfromwork(m_BkpWork);

	// Reset alpha
	m_Alpha = 0.0f;
	m_SlAlpha->setValue(m_Alpha * m_SliderMax);
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));

	emit EndDatachange(this);
}

void ImageOverlay::SlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void ImageOverlay::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
	m_Bmphand->Copyfromwork(m_BkpWork);
}

void ImageOverlay::Newloaded()
{
	free(m_BkpWork);
	m_BkpWork = (float*)malloc(sizeof(float) *
														 m_Handler3D->GetActivebmphandler()->ReturnArea());
}

void ImageOverlay::AlphaChanged()
{
	bool b1;
	float value = m_LeAlpha->text().toFloat(&b1);
	if (!b1)
	{
		m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
		m_SlAlpha->setValue(m_Alpha * m_SliderMax);
		QApplication::beep();
		return;
	}

	if (value < 0.0f)
	{
		value = 0.0f;
	}
	else if (value > 1.0f)
	{
		value = 1.0f;
	}
	m_Alpha = value;
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));
	m_SlAlpha->setValue(m_Alpha * m_SliderMax);

	DataSelection data_selection;
	data_selection.allSlices = false;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this, false);

	m_Bmphand->Copy2work(m_BkpWork, m_Bmphand->ReturnMode(false));
	m_Bmphand->BmpOverlay(m_Alpha);

	emit EndDatachange(this, iseg::NoUndo);
}

void ImageOverlay::SliderChanged(int newval)
{
	m_Alpha = newval / (float)m_SliderMax;
	m_LeAlpha->setText(QString::number(m_Alpha, 'f', m_SliderPrecision));

	DataSelection data_selection;
	data_selection.allSlices = false;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this, false);

	m_Bmphand->Copy2work(m_BkpWork, m_Bmphand->ReturnMode(false));
	m_Bmphand->BmpOverlay(m_Alpha);

	emit EndDatachange(this, iseg::NoUndo);
}

CleanerParams::CleanerParams(int* rate1, int* minsize1, QWidget* parent, Qt::WindowFlags wFlags)
{
	m_Rate = rate1;
	m_Minsize = minsize1;
	m_Hbox1 = new Q3HBox(this);
	m_Vbox1 = new Q3VBox(m_Hbox1);
	m_Vbox2 = new Q3VBox(m_Hbox1);
	m_LbRate = new QLabel(QString("Rate: "), m_Vbox1);
	m_LbMinsize = new QLabel(QString("Pixel Size: "), m_Vbox1);
	m_PbDoit = new QPushButton("OK", m_Vbox1);
	m_SbRate = new QSpinBox(3, 10000, 1, m_Vbox2);
	m_SbRate->setValue(4);
	m_SbRate->setToolTip(Format("1/rate is the percentage of the total (tissue) volume needed to force small regions to be kept (overrides Pixel Size criterion)."));
	m_SbMinsize = new QSpinBox(2, 10000, 1, m_Vbox2);
	m_SbMinsize->setValue(10);
	m_SbMinsize->setToolTip(Format("Minimum number of pixels required by an island."));
	m_PbDontdoit = new QPushButton("Cancel", m_Vbox2);
	QObject_connect(m_PbDoit, SIGNAL(clicked()), this, SLOT(DoitPressed()));
	QObject_connect(m_PbDontdoit, SIGNAL(clicked()), this, SLOT(DontdoitPressed()));

	m_Hbox1->setFixedSize(m_Hbox1->sizeHint());
	setFixedSize(m_Hbox1->size());
}

CleanerParams::~CleanerParams() { delete m_Hbox1; }

void CleanerParams::DoitPressed()
{
	*m_Rate = m_SbRate->value();
	*m_Minsize = m_SbMinsize->value();
	close();
}

void CleanerParams::DontdoitPressed()
{
	*m_Rate = 0;
	*m_Minsize = 0;
	close();
}

MergeProjectsDialog::MergeProjectsDialog(QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags)
{
	setModal(true);

	m_HboxOverall = new Q3HBoxLayout(this);
	m_VboxFileList = new Q3VBoxLayout(this);
	m_VboxButtons = new Q3VBoxLayout(this);
	m_VboxEditButtons = new Q3VBoxLayout(this);
	m_VboxExecuteButtons = new Q3VBoxLayout(this);

	m_FileListWidget = new QListWidget(this);
	m_FileListWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);

	m_AddButton = new QPushButton("Add...", this);
	m_RemoveButton = new QPushButton("Remove", this);
	m_MoveUpButton = new QPushButton("Move up", this);
	m_MoveDownButton = new QPushButton("Move down", this);
	m_ExecuteButton = new QPushButton("Execute", this);
	m_CancelButton = new QPushButton("Cancel", this);

	m_HboxOverall->setMargin(10);

	m_VboxFileList->addWidget(m_FileListWidget);

	m_VboxEditButtons->setAlignment(Qt::AlignTop);
	m_VboxEditButtons->addWidget(m_AddButton);
	m_VboxEditButtons->addWidget(m_RemoveButton);
	m_VboxEditButtons->addWidget(m_MoveUpButton);
	m_VboxEditButtons->addWidget(m_MoveDownButton);

	m_VboxExecuteButtons->setAlignment(Qt::AlignBottom);
	m_VboxExecuteButtons->addWidget(m_ExecuteButton);
	m_VboxExecuteButtons->addWidget(m_CancelButton);

	m_VboxButtons->addLayout(m_VboxEditButtons);
	m_VboxButtons->addLayout(m_VboxExecuteButtons);

	m_HboxOverall->addLayout(m_VboxFileList);
	m_HboxOverall->addSpacing(10);
	m_HboxOverall->addLayout(m_VboxButtons);

	QObject_connect(m_AddButton, SIGNAL(clicked()), this, SLOT(AddPressed()));
	QObject_connect(m_RemoveButton, SIGNAL(clicked()), this, SLOT(RemovePressed()));
	QObject_connect(m_MoveUpButton, SIGNAL(clicked()), this, SLOT(MoveUpPressed()));
	QObject_connect(m_MoveDownButton, SIGNAL(clicked()), this, SLOT(MoveDownPressed()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(accept()));
	QObject_connect(m_CancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	setWindowTitle("Merge Projects");
}

MergeProjectsDialog::~MergeProjectsDialog() { delete m_HboxOverall; }

void MergeProjectsDialog::AddPressed()
{
	QStringList openfilenames = QFileDialog::getOpenFileNames(
			this, "Select one or more files to add", QString::null, "Projects (*.prj)");
	m_FileListWidget->addItems(openfilenames);
}

void MergeProjectsDialog::RemovePressed()
{
	const auto remove_items = m_FileListWidget->selectedItems();
	for (const auto& item: remove_items)
	{
		delete m_FileListWidget->takeItem(m_FileListWidget->row(item));
	}
}

void MergeProjectsDialog::MoveUpPressed()
{
	QList<QListWidgetItem*> move_items = m_FileListWidget->selectedItems();
	if (move_items.empty())
	{
		return;
	}

	int row_first = m_FileListWidget->row(*move_items.begin());
	int row_last = m_FileListWidget->row(*(move_items.end() - 1));
	if (row_first <= 0)
	{
		return;
	}

	m_FileListWidget->insertItem(row_last, m_FileListWidget->takeItem(row_first - 1));
}

void MergeProjectsDialog::MoveDownPressed()
{
	QList<QListWidgetItem*> move_items = m_FileListWidget->selectedItems();
	if (move_items.empty())
	{
		return;
	}

	int row_first = m_FileListWidget->row(*move_items.begin());
	int row_last = m_FileListWidget->row(*(move_items.end() - 1));
	if (row_last >= m_FileListWidget->count() - 1)
	{
		return;
	}

	m_FileListWidget->insertItem(row_first, m_FileListWidget->takeItem(row_last + 1));
}

void MergeProjectsDialog::GetFilenames(std::vector<QString>& filenames)
{
	filenames.clear();
	for (int row = 0; row < m_FileListWidget->count(); ++row)
	{
		filenames.push_back(m_FileListWidget->item(row)->text());
	}
}

CheckBoneConnectivityDialog::CheckBoneConnectivityDialog(SlicesHandler* hand3D, const char* name, QWidget* parent /*=0*/, Qt::WindowFlags wFlags /*=0*/)
		: QWidget(parent, wFlags), m_Handler3D(hand3D)
{
	m_MainBox = new Q3HBox(this);
	m_Vbox1 = new Q3VBox(m_MainBox);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_Hbox4 = new Q3HBox(m_Vbox1);

	m_Vbox1->setMargin(3);
	m_Hbox1->setMargin(3);
	m_Hbox2->setMargin(3);
	m_Hbox3->setMargin(3);
	m_Hbox4->setMargin(3);

	m_MainBox->setFixedSize(QSize(420, 420));

	m_Hbox1->setMaximumHeight(30);
	m_Hbox2->setMaximumHeight(30);

	m_BonesFoundCb = new QCheckBox(QString("Bones Found"), m_Hbox1);
	m_BonesFoundCb->setEnabled(false);

	m_ExecuteButton = new QPushButton("Execute", m_Hbox2);
	m_CancelButton = new QPushButton("Cancel", m_Hbox2);

	m_FoundConnectionsTable = new QTableWidget(m_Hbox3);
	m_FoundConnectionsTable->setColumnCount(eBoneConnectionColumn::kColumnNumber);

	m_ExportButton = new QPushButton("Export", m_Hbox4);
	m_ExportButton->setMaximumWidth(60);
	m_ExportButton->setEnabled(false);
	m_ProgressText = new QLabel(m_Hbox4);
	m_ProgressText->setAlignment(Qt::AlignRight);

	QStringList table_header;
	table_header << "Bone 1"
							 << "Bone 2"
							 << "Slice #";
	m_FoundConnectionsTable->setHorizontalHeaderLabels(table_header);
	m_FoundConnectionsTable->verticalHeader()->setVisible(false);
	m_FoundConnectionsTable->setColumnWidth(eBoneConnectionColumn::kTissue1, 160);
	m_FoundConnectionsTable->setColumnWidth(eBoneConnectionColumn::kTissue2, 160);
	m_FoundConnectionsTable->setColumnWidth(eBoneConnectionColumn::kSliceNumber, 60);

	m_Hbox1->show();
	m_Hbox2->show();
	m_Hbox3->show();
	m_Hbox4->show();
	m_Vbox1->show();

	m_MainBox->show();

	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(ExecutePressed()));
	QObject_connect(m_CancelButton, SIGNAL(clicked()), this, SLOT(CancelPressed()));
	QObject_connect(m_ExportButton, SIGNAL(clicked()), this, SLOT(ExportPressed()));
	QObject_connect(m_FoundConnectionsTable, SIGNAL(cellClicked(int,int)), this, SLOT(CellClicked(int,int)));

	CheckBoneExist();
}

CheckBoneConnectivityDialog::~CheckBoneConnectivityDialog() = default;

void CheckBoneConnectivityDialog::ShowText(const std::string& text)
{
	m_ProgressText->setText(QString::fromStdString(text));
}

bool CheckBoneConnectivityDialog::IsBone(const std::string& label_name) const
{
	std::string name = label_name;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	if (name.find("cortical") != std::string::npos)
	{
		return true;
	}
	else if (name.find("cancellous") != std::string::npos)
	{
		return true;
	}
	else if (name.find("marrow") != std::string::npos)
	{
		return true;
	}
	else if (name.find("tooth") != std::string::npos)
	{
		return true;
	}
	else if (name.find("bone") != std::string::npos)
	{
		return true;
	}
	return false;
}

void CheckBoneConnectivityDialog::CheckBoneExist()
{
	int bones_found = 0;

	tissues_size_t tissuecount = TissueInfos::GetTissueCount();
	for (tissues_size_t i = 0; i <= tissuecount; i++)
	{
		TissueInfo* tissue_info = TissueInfos::GetTissueInfo(i);
		if (IsBone(tissue_info->m_Name))
		{
			bones_found++;
			if (bones_found > 1)
			{
				break;
			}
		}
	}

	if (bones_found > 1)
	{
		ShowText("Bones found. Press Execute to look for bone connections");
	}
	else if (bones_found == 1)
	{
		ShowText("Only one bone found. No possible connections");
	}
	else
	{
		ShowText("One bone found. No possible connection");
	}

	m_BonesFoundCb->setChecked(bones_found > 1);
	m_ExecuteButton->setEnabled(bones_found > 1);
}

void CheckBoneConnectivityDialog::LookForConnections()
{
	m_FoundConnections.clear();

	unsigned short width = m_Handler3D->Width();
	unsigned short height = m_Handler3D->Height();
	unsigned short start_sl = m_Handler3D->StartSlice();
	unsigned short end_sl = m_Handler3D->EndSlice();

	int num_tasks = end_sl - start_sl;
	QProgressDialog progress("Looking for connected bones...", "Cancel", 0, num_tasks, m_MainBox);
	progress.show();
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.setValue(0);

	std::vector<std::string> label_names;
	tissues_size_t tissuecount = TissueInfos::GetTissueCount();
	for (tissues_size_t i = 0; i <= tissuecount; i++)
	{
		TissueInfo* tissue_info = TissueInfos::GetTissueInfo(i);
		label_names.push_back(tissue_info->m_Name);
	}

	std::vector<int> same_bone_map(label_names.size(), -1);
	auto replace = [](std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
		{
			return false;
		}
		str.replace(start_pos, from.length(), to);
		return true;
	};

	for (size_t i = 0; i < label_names.size(); i++)
	{
		if (IsBone(label_names[i]))
		{
			std::string name = label_names[i];
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);
			replace(name, "cancellous", "cortical");
			replace(name, "red_marrow", "cortical");
			replace(name, "marrow", "cortical");
			size_t idx = 0;
			for (; idx < label_names.size(); idx++)
			{
				std::string namei = label_names[idx];
				std::transform(namei.begin(), namei.end(), namei.begin(), ::tolower);
				if (namei == name)
				{
					break;
				}
			}
			if (idx < label_names.size())
			{
				same_bone_map[i] = (int)idx;
			}
			else
			{
				same_bone_map[i] = (int)i;
			}
		}
	}

	for (int slice_n = start_sl; slice_n < end_sl - 1; slice_n++)
	{
		tissues_size_t* tissues_main = m_Handler3D->ReturnTissues(0, slice_n);
		unsigned pos = 0;
		//for( int y=height-1; y>=0; y-- )
		for (int y = height - 2; y >= 1; y--)
		{
			for (int x = 1; x < width - 1; x++)
			{
				pos = y * width + x;

				tissues_size_t tissue_value = tissues_main[pos];
				if (std::find(same_bone_map.begin(), same_bone_map.end(), tissue_value) != same_bone_map.end())
				{
					// check neighbour connection
					std::vector<tissues_size_t> neighbour_tissues;

					//Same slice:
					neighbour_tissues.push_back(tissues_main[pos - width - 1]);
					neighbour_tissues.push_back(tissues_main[pos - width]);
					neighbour_tissues.push_back(tissues_main[pos - width + 1]);

					neighbour_tissues.push_back(tissues_main[pos - 1]);
					neighbour_tissues.push_back(tissues_main[pos + 1]);

					neighbour_tissues.push_back(tissues_main[pos + width - 1]);
					neighbour_tissues.push_back(tissues_main[pos + width]);
					neighbour_tissues.push_back(tissues_main[pos + width + 1]);

					//Not needed
					//-1 slice:
					//if( sliceN-1 >= startSl)
					//{
					//	tissues_size_t *tissues_minus1 = handler3D->return_tissues(0,sliceN-1);
					//
					//	neighbour_tissues.push_back( tissues_minus1[pos-width-1] );
					//	neighbour_tissues.push_back( tissues_minus1[pos-width] );
					//	neighbour_tissues.push_back( tissues_minus1[pos-width+1] );
					//
					//	neighbour_tissues.push_back( tissues_minus1[pos-1] );
					//	neighbour_tissues.push_back( tissues_minus1[pos] );
					//	neighbour_tissues.push_back( tissues_minus1[pos+1] );
					//
					//	neighbour_tissues.push_back( tissues_minus1[pos+width-1] );
					//	neighbour_tissues.push_back( tissues_minus1[pos+width] );
					//	neighbour_tissues.push_back( tissues_minus1[pos+width+1] );
					//}

					//+1 slice:
					if (slice_n + 1 < end_sl)
					{
						tissues_size_t* tissues_plus1 =
								m_Handler3D->ReturnTissues(0, slice_n + 1);

						neighbour_tissues.push_back(tissues_plus1[pos - width - 1]);
						neighbour_tissues.push_back(tissues_plus1[pos - width]);
						neighbour_tissues.push_back(tissues_plus1[pos - width + 1]);

						neighbour_tissues.push_back(tissues_plus1[pos - 1]);
						neighbour_tissues.push_back(tissues_plus1[pos]);
						neighbour_tissues.push_back(tissues_plus1[pos + 1]);

						neighbour_tissues.push_back(tissues_plus1[pos + width - 1]);
						neighbour_tissues.push_back(tissues_plus1[pos + width]);
						neighbour_tissues.push_back(tissues_plus1[pos + width + 1]);
					}

					//remove the same bone values
					std::sort(neighbour_tissues.begin(), neighbour_tissues.end());
					neighbour_tissues.erase(std::unique(neighbour_tissues.begin(), neighbour_tissues.end()), neighbour_tissues.end());
					neighbour_tissues.erase(std::remove(neighbour_tissues.begin(), neighbour_tissues.end(), tissue_value), neighbour_tissues.end());

					for (size_t k = 0; k < neighbour_tissues.size(); k++)
					{
						if (std::find(same_bone_map.begin(), same_bone_map.end(), neighbour_tissues[k]) != same_bone_map.end())
						{
							tissues_size_t tis1, tis2;
							if (tissue_value < neighbour_tissues[k])
							{
								tis1 = tissue_value;
								tis2 = neighbour_tissues[k];
							}
							else
							{
								tis1 = neighbour_tissues[k];
								tis2 = tissue_value;
							}
							BoneConnectionInfo match_found(tis1, tis2, slice_n);
							m_FoundConnections.push_back(match_found);
						}
					}
				}
			}
		}

		progress.setValue(slice_n);

		if (progress.wasCanceled())
		{
			break;
		}
	}

	progress.setValue(num_tasks);

	std::sort(m_FoundConnections.begin(), m_FoundConnections.end());
	m_FoundConnections.erase(std::unique(m_FoundConnections.begin(), m_FoundConnections.end()), m_FoundConnections.end());
}

void CheckBoneConnectivityDialog::FillConnectionsTable()
{
	while (m_FoundConnectionsTable->rowCount() > 0)
	{
		m_FoundConnectionsTable->removeRow(0);
	}

	for (size_t i = 0; i < m_FoundConnections.size(); i++)
	{
		int row = m_FoundConnectionsTable->rowCount();
		m_FoundConnectionsTable->insertRow(row);

		BoneConnectionInfo new_line_info = m_FoundConnections.at(i);

		TissueInfo* tissue_info1 = TissueInfos::GetTissueInfo(new_line_info.m_TissueID1);
		TissueInfo* tissue_info2 = TissueInfos::GetTissueInfo(new_line_info.m_TissueID2);

		m_FoundConnectionsTable->setItem(row, eBoneConnectionColumn::kTissue1, new QTableWidgetItem(ToQ(tissue_info1->m_Name)));
		m_FoundConnectionsTable->setItem(row, eBoneConnectionColumn::kTissue2, new QTableWidgetItem(ToQ(tissue_info2->m_Name)));
		m_FoundConnectionsTable->setItem(row, eBoneConnectionColumn::kSliceNumber, new QTableWidgetItem(QString::number(new_line_info.m_SliceNumber + 1)));
	}

	m_ExportButton->setEnabled(!m_FoundConnections.empty());
}

void CheckBoneConnectivityDialog::CellClicked(int row, int col)
{
	if (row < m_FoundConnectionsTable->rowCount())
	{
		int slice_number =
				m_FoundConnectionsTable->item(row, kSliceNumber)->text().toInt() - 1;
		m_Handler3D->SetActiveSlice(slice_number);
		emit SliceChanged();
	}
}

void CheckBoneConnectivityDialog::ExecutePressed()
{
	m_ExecuteButton->setEnabled(false);

	LookForConnections();
	FillConnectionsTable();

	ShowText(std::to_string(m_FoundConnections.size()) + " connections found.");

	m_ExecuteButton->setEnabled(true);
}

void CheckBoneConnectivityDialog::CancelPressed() { close(); }

void CheckBoneConnectivityDialog::ExportPressed()
{
	std::ofstream output_file("BoneConnections.txt");

	for (unsigned int i = 0; i < m_FoundConnections.size(); i++)
	{
		TissueInfo* tissue_info1 =
				TissueInfos::GetTissueInfo(m_FoundConnections[i].m_TissueID1);
		TissueInfo* tissue_info2 =
				TissueInfos::GetTissueInfo(m_FoundConnections[i].m_TissueID2);
		output_file << tissue_info1->m_Name << " "
								<< tissue_info2->m_Name << " "
								<< m_FoundConnections[i].m_SliceNumber + 1 << endl;
	}
	output_file.close();

	ShowText("Export finished to BoneConnections.txt");
}

} // namespace iseg
