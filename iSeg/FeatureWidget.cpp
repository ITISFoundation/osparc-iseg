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

#include "FeatureWidget.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Data/addLine.h"

#include "Core/ImageForestingTransform.h"
#include "Core/Pair.h"

#include <QGridLayout>
#include <QSpacerItem>

namespace iseg {

FeatureWidget::FeatureWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Obtain information about the gray value and tissue distribution."
										"<br>"
										"A rectangular area is marked by pressing down the left mouse "
										"button and moving the mouse."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Selecting = false;

	auto layout = new QGridLayout;
	layout->addWidget(m_LbMap = new QLabel(""), 0, 0);
	layout->addWidget(m_LbAv = new QLabel("Average: "), 1, 0);
	layout->addWidget(m_LbStddev = new QLabel("Std Dev.: "), 2, 0);
	layout->addWidget(m_LbMin = new QLabel("Minimum: "), 3, 0);
	layout->addWidget(m_LbMax = new QLabel("Maximum: "), 4, 0);
	layout->addWidget(m_LbGrey = new QLabel("Grey val.:"), 5, 0);
	layout->addWidget(m_LbPt = new QLabel("Coord.: "), 6, 0);
	layout->addWidget(m_LbTissue = new QLabel("Tissue: "), 7, 0);

	layout->addWidget(m_LbMapValue = new QLabel("Source"), 0, 1);
	layout->addWidget(m_LbAvValue = new QLabel(""), 1, 1);
	layout->addWidget(m_LbStddevValue = new QLabel(""), 2, 1);
	layout->addWidget(m_LbMinValue = new QLabel(""), 3, 1);
	layout->addWidget(m_LbMaxValue = new QLabel(""), 4, 1);
	layout->addWidget(m_LbGreyValue = new QLabel(""), 5, 1);
	layout->addWidget(m_LbPtValue = new QLabel(""), 6, 1);
	layout->addWidget(m_LbDummy = new QLabel(""), 7, 1);

	layout->addWidget(m_LbWorkMapValue = new QLabel("Target"), 0, 2);
	layout->addWidget(m_LbWorkAvValue = new QLabel(""), 1, 2);
	layout->addWidget(m_LbWorkStddevValue = new QLabel(""), 2, 2);
	layout->addWidget(m_LbWorkMinValue = new QLabel(""), 3, 2);
	layout->addWidget(m_LbWorkMaxValue = new QLabel(""), 4, 2);
	layout->addWidget(m_LbWorkGreyValue = new QLabel(""), 5, 2);
	layout->addWidget(m_LbWorkPtValue = new QLabel(""), 6, 2);
	layout->addWidget(m_LbTissuename = new QLabel(""), 7, 2);

	setLayout(layout);
}

void FeatureWidget::OnMouseClicked(Point p)
{
	m_Selecting = true;
	m_Pstart = p;
}

void FeatureWidget::OnMouseMoved(Point p)
{
	if (m_Selecting)
	{
		Point p1;
		m_Dynamic.clear();
		p1.px = p.px;
		p1.py = m_Pstart.py;
		addLine(&m_Dynamic, p1, p);
		addLine(&m_Dynamic, p1, m_Pstart);
		p1.px = m_Pstart.px;
		p1.py = p.py;
		addLine(&m_Dynamic, p1, p);
		addLine(&m_Dynamic, p1, m_Pstart);
		emit VpdynChanged(&m_Dynamic);
	}

	Pair psize = m_Handler3D->GetPixelsize();

	m_LbPtValue->setText(QString::number(m_Handler3D->Width() - 1 - p.px) +
											 QString(":") + QString::number(p.py));
	m_LbWorkPtValue->setText(QString("(") +
													 QString::number((m_Handler3D->Width() - 1 - p.px) * psize.high) +
													 QString(":") + QString::number(p.py * psize.low) + QString(" mm)"));
	m_LbGreyValue->setText(QString::number(m_Bmphand->BmpPt(p)));
	m_LbWorkGreyValue->setText(QString::number(m_Bmphand->WorkPt(p)));
	tissues_size_t tnr =
			m_Bmphand->TissuesPt(m_Handler3D->ActiveTissuelayer(), p);
	if (tnr == 0)
		m_LbTissuename->setText("- (0)");
	else
		m_LbTissuename->setText(ToQ(TissueInfos::GetTissueName(tnr)) + QString(" (") + QString::number((int)tnr) + QString(")"));
}

void FeatureWidget::OnMouseReleased(Point p)
{
	m_Dynamic.clear();
	emit VpdynChanged(&m_Dynamic);

	m_Selecting = false;
	float av = m_Bmphand->ExtractFeature(m_Pstart, p);
	float stddev = m_Bmphand->ReturnStdev();
	Pair extrema = m_Bmphand->ReturnExtrema();

	m_LbAvValue->setText(QString::number(av));
	m_LbStddevValue->setText(QString::number(stddev));
	m_LbMinValue->setText(QString::number(extrema.low));
	m_LbMaxValue->setText(QString::number(extrema.high));

	av = m_Bmphand->ExtractFeaturework(m_Pstart, p);
	stddev = m_Bmphand->ReturnStdev();
	extrema = m_Bmphand->ReturnExtrema();

	m_LbWorkAvValue->setText(QString::number(av));
	m_LbWorkStddevValue->setText(QString::number(stddev));
	m_LbWorkMinValue->setText(QString::number(extrema.low));
	m_LbWorkMaxValue->setText(QString::number(extrema.high));
}

void FeatureWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void FeatureWidget::Init()
{
	OnSlicenrChanged();
}

void FeatureWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

} // namespace iseg
