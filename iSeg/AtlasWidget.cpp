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

#include "AtlasWidget.h"
#include "WidgetCollection.h"

#include "Interface/QtConnect.h"

#include "Core/ProjectVersion.h"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QScrollBar>
#include <QSlider>
#include <QWheelEvent>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QPen>
#include <QWidget>

namespace iseg {

AtlasWidget::AtlasWidget(const char* filename, QDir picpath, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	m_IsOk = false;
	QString title("Atlas - ");
	title = title + QFileInfo(filename).completeBaseName();
	setCaption(title);
	m_Tissue = nullptr;
	m_Image = nullptr;
	if (!Loadfile(filename))
	{
		return;
	}
	m_IsOk = true;
	m_MPicpath = picpath;

	QVBoxLayout* vbox1 = new QVBoxLayout;
	QHBoxLayout* hbox1 = new QHBoxLayout;
	QHBoxLayout* hbox2 = new QHBoxLayout;
	QHBoxLayout* hbox3 = new QHBoxLayout;

	m_SlContrast = new QSlider(Qt::Horizontal, this);
	m_SlContrast->setRange(0, 99);
	m_SlContrast->setValue(0);
	m_SlBrightness = new QSlider(Qt::Horizontal, this);
	m_SlBrightness->setRange(0, 100);
	m_SlBrightness->setValue(50);
	m_LbContrast = new QLabel("C:", this);
	m_LbContrast->setPixmap(QIcon(m_MPicpath.absoluteFilePath(QString("icon-contrast.png"))).pixmap());
	m_LbBrightness = new QLabel("B:", this);
	m_LbBrightness->setPixmap(QIcon(m_MPicpath.absoluteFilePath(QString("icon-brightness.png"))).pixmap());
	hbox1->addWidget(m_LbContrast);
	hbox1->addWidget(m_SlContrast);
	hbox1->addWidget(m_LbBrightness);
	hbox1->addWidget(m_SlBrightness);

	vbox1->setSpacing(0);
	vbox1->setMargin(2);
	vbox1->addLayout(hbox1);
	m_SaViewer = new QScrollArea(this);
	m_AtlasViewer = new AtlasViewer(m_Image, m_Tissue, 2, m_Dimx, m_Dimy, m_Dimz, m_Dx, m_Dy, m_Dz, &m_ColorR, &m_ColorG, &m_ColorB, this);
	m_SaViewer->setWidget(m_AtlasViewer);
	vbox1->addWidget(m_SaViewer);
	m_ScbSlicenr = new QScrollBar(0, m_Dimz - 1, 1, 5, 0, Qt::Horizontal, this);
	vbox1->addWidget(m_ScbSlicenr);

	m_BgOrient = new QButtonGroup(this);
	//	imgorval->hide();
	m_RbX = new QRadioButton(QString("x"), this);
	m_RbY = new QRadioButton(QString("y"), this);
	m_RbZ = new QRadioButton(QString("z"), this);
	m_BgOrient->insert(m_RbX);
	m_BgOrient->insert(m_RbY);
	m_BgOrient->insert(m_RbZ);
	m_RbZ->setChecked(TRUE);
	hbox2->addWidget(m_RbX);
	hbox2->addWidget(m_RbY);
	hbox2->addWidget(m_RbZ);

	m_SlTransp = new QSlider(Qt::Horizontal, this);
	m_SlTransp->setRange(0, 100);
	m_SlTransp->setValue(50);
	m_LbTransp = new QLabel("Transp:", this);
	hbox3->addWidget(m_LbTransp);
	hbox3->addWidget(m_SlTransp);

	vbox1->addLayout(hbox2);
	vbox1->addLayout(hbox3);
	m_Zoomer = new ZoomWidget(1.0, m_MPicpath, this);
	m_Zoomer->setFixedSize(m_Zoomer->sizeHint());
	vbox1->addWidget(m_Zoomer);
	m_LbName = new QLabel(QString("jljlfds"), this);
	vbox1->addWidget(m_LbName);
	setLayout(vbox1);

	QObject_connect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
	QObject_connect(m_SlTransp, SIGNAL(valueChanged(int)), this, SLOT(SlTranspChanged()));
	QObject_connect(m_BgOrient, SIGNAL(buttonClicked(int)), this, SLOT(XyzChanged()));
	QObject_connect(m_Zoomer, SIGNAL(SetZoom(double)), m_AtlasViewer, SLOT(SetZoom(double)));
	QObject_connect(m_SlBrightness, SIGNAL(valueChanged(int)), this, SLOT(SlBrightcontrMoved()));
	QObject_connect(m_SlContrast, SIGNAL(valueChanged(int)), this, SLOT(SlBrightcontrMoved()));

	QObject_connect(m_AtlasViewer, SIGNAL(MousemovedSign(tissues_size_t)), this, SLOT(PtMoved(tissues_size_t)));
	m_AtlasViewer->setMouseTracking(true);
}

AtlasWidget::~AtlasWidget()
{
	delete[] m_Tissue;
	delete[] m_Image;
}

bool AtlasWidget::Loadfile(const char* filename)
{
	m_Dimx = m_Dimy = m_Dimz = 10;
	m_Dx = m_Dy = m_Dz = 1.0;

	QFile file(filename);
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);

	// Read and check the header
	quint32 magic;
	in >> magic;
	if (magic != 0xD0C0B0A0)
		return false;

	// Read the version
	qint32 combined_version;
	in >> combined_version;
	int version, tissues_version;
	iseg::ExtractTissuesVersion((int)combined_version, version, tissues_version);
	if (version < 1)
		return false;
	if (version > 1)
		return false;

	in.setVersion(QDataStream::Qt_4_0);

	qint32 dummy;
	in >> dummy;
	m_Dimx = (unsigned short)dummy;
	in >> dummy;
	m_Dimy = (unsigned short)dummy;
	in >> dummy;
	m_Dimz = (unsigned short)dummy;
	in >> m_Dx >> m_Dy >> m_Dz;
	in >> dummy;
	tissues_size_t nrtissues = (tissues_size_t)dummy;
	m_TissueNames.resize(nrtissues);
	m_ColorR.resize(nrtissues);
	m_ColorG.resize(nrtissues);
	m_ColorB.resize(nrtissues);

	unsigned dimtot = unsigned(m_Dimx) * unsigned(m_Dimy) * m_Dimz;
	m_Image = new float[dimtot];
	if (m_Image == nullptr)
		return false;
	m_Tissue = new tissues_size_t[dimtot];
	if (m_Tissue == nullptr)
	{
		delete[] m_Image;
		return false;
	}

	for (tissues_size_t i = 0; i < nrtissues; i++)
	{
		in >> m_TissueNames[i] >> m_ColorR[i] >> m_ColorG[i] >> m_ColorB[i];
	}

	int area = m_Dimx * (int)m_Dimy;
	if (tissues_version > 0)
	{
		for (unsigned short i = 0; i < m_Dimz; i++)
		{
			in.readRawData((char*)&(m_Image[area * i]), area * sizeof(float));
			in.readRawData((char*)&(m_Tissue[area * i]), area * sizeof(tissues_size_t));
		}
	}
	else
	{
		char* char_buffer = new char[area];
		for (unsigned short i = 0; i < m_Dimz; i++)
		{
			in.readRawData((char*)&(m_Image[area * i]), area * sizeof(float));
			in.readRawData(char_buffer, area);
			for (int j = 0; j < area; j++)
			{
				m_Tissue[area * i + j] = char_buffer[j];
			}
		}
		delete[] char_buffer;
	}

	//for(unsigned i=0;i<dimtot;i++) image[i]=10*(i%10);
	//for(unsigned i=0;i<dimtot;i++) tissue[i]=((i/10)%10);

	m_Minval = m_Maxval = m_Image[0];
	for (unsigned i = 1; i < dimtot; i++)
	{
		if (m_Minval > m_Image[i])
			m_Minval = m_Image[i];
		if (m_Maxval < m_Image[i])
			m_Maxval = m_Image[i];
	}

	//tissue_names.push_back(QString("a"));
	//tissue_names.push_back(QString("b"));
	//tissue_names.push_back(QString("c"));
	//tissue_names.push_back(QString("d"));
	//tissue_names.push_back(QString("e"));
	//tissue_names.push_back(QString("f"));
	//tissue_names.push_back(QString("g"));
	//tissue_names.push_back(QString("h"));
	//tissue_names.push_back(QString("i"));
	//tissue_names.push_back(QString("j"));

	//color_r.push_back(0.1f);
	//color_r.push_back(0.2f);
	//color_r.push_back(0.3f);
	//color_r.push_back(0.4f);
	//color_r.push_back(0.5f);
	//color_r.push_back(0.6f);
	//color_r.push_back(0.7f);
	//color_r.push_back(0.8f);
	//color_r.push_back(0.9f);

	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);
	//color_g.push_back(1.0f);

	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);
	//color_b.push_back(1.0f);

	return true;
}

void AtlasWidget::ScbSlicenrChanged()
{
	m_AtlasViewer->SlicenrChanged(m_ScbSlicenr->value());
}

void AtlasWidget::SlTranspChanged()
{
	m_AtlasViewer->SetTissueopac(1.0f - m_SlTransp->value() * 0.01f);
}

void AtlasWidget::XyzChanged()
{
	m_ScbSlicenr->setValue(0);
	if (m_RbX->isChecked())
	{
		m_ScbSlicenr->setMaxValue(m_Dimx - 1);
		m_AtlasViewer->OrientChanged(0);
	}
	else if (m_RbY->isChecked())
	{
		m_ScbSlicenr->setMaxValue(m_Dimy - 1);
		m_AtlasViewer->OrientChanged(1);
	}
	else if (m_RbZ->isChecked())
	{
		m_ScbSlicenr->setMaxValue(m_Dimz - 1);
		m_AtlasViewer->OrientChanged(2);
	}
}

void AtlasWidget::PtMoved(tissues_size_t val)
{
	if (val == 0)
		m_LbName->setText("Background");
	else
		m_LbName->setText(m_TissueNames[val - 1]);
}

void AtlasWidget::SlBrightcontrMoved()
{
	float factor, offset;
	factor = 255.0f * (1 + m_SlContrast->value()) / (m_Maxval - m_Minval);
	offset =
			(127.5f - m_Maxval * factor) * (1.0f - m_SlBrightness->value() * 0.01f) +
			0.01f * (127.5f - m_Minval * factor) * m_SlBrightness->value();
	m_AtlasViewer->SetScale(offset, factor);
}

} // namespace iseg
