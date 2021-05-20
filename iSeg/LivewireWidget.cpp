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

#include "LivewireWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/addLine.h"

#include "Interface/LayoutTools.h"

#include "Core/ImageForestingTransform.h"
#include "Core/Pair.h"

#include <QFormLayout>
#include <QTime>

namespace iseg {

LivewireWidget::LivewireWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Use the Auto Trace to follow ideal contour path or draw "
										"contours around a tissue to segment it."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Drawing = false;
	m_Lw = m_Lwfirst = nullptr;

	// parameters
	m_Straight = new QRadioButton(QString("Straight"));
	m_Autotrace = new QRadioButton(QString("Auto Trace"));
	m_Autotrace->setToolTip(Format("The Livewire (intelligent scissors) algorithm "
																 "to automatically identify the ideal contour path.This algorithm uses "
																 "information "
																 "about the strength and orientation of the(smoothed) image gradient, "
																 "the zero - crossing of the Laplacian (for fine tuning) together with "
																 "some weighting "
																 "to favor straighter lines to determine the most likely contour path. "
																 "The "
																 "contouring is started by clicking the left mouse button. Each "
																 "subsequent left "
																 "button click fixes another point and the suggested contour line in "
																 "between. "
																 "<br>"
																 "Successive removing of unwanted points is achieved by clicking the "
																 "middle "
																 "mouse button. A double left click closes the contour while a double "
																 "middle "
																 "click aborts the line drawing process."));
	m_Freedraw = new QRadioButton(QString("Free"));
	auto drawmode = make_button_group(this, {m_Straight, m_Autotrace, m_Freedraw});
	m_Autotrace->setChecked(true);

	m_CbFreezing = new QCheckBox;
	m_CbFreezing->setToolTip(Format("Specify the number of seconds after which a line segment is "
																	"frozen even without mouse click if it has not changed."));
	m_SbFreezing = new QSpinBox(1, 10, 1, nullptr);
	m_SbFreezing->setValue(3);

	m_CbClosing = new QCheckBox;
	m_CbClosing->setChecked(true);

	// layout
	auto method_layout = make_vbox({m_Straight, m_Autotrace, m_Freedraw});
	auto method_area = new QFrame;
	method_area->setLayout(method_layout);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);
	method_area->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

	m_ParamsLayout = new QFormLayout;
	m_ParamsLayout->addRow("Close contour", m_CbClosing);
	m_ParamsLayout->addRow("Freezing", m_CbFreezing);
	m_ParamsLayout->addRow("Freezing delay (s)", m_SbFreezing);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addLayout(m_ParamsLayout);

	setLayout(top_layout);

	// initialize
	m_Cooling = false;
	m_CbFreezing->setChecked(m_Cooling);

	SbfreezingChanged(m_SbFreezing->value());
	FreezingChanged();
	ModeChanged();

	// connections
	QObject_connect(drawmode, SIGNAL(buttonClicked(int)), this, SLOT(ModeChanged()));
	QObject_connect(m_CbFreezing, SIGNAL(clicked()), this, SLOT(FreezingChanged()));
	QObject_connect(m_SbFreezing, SIGNAL(valueChanged(int)), this, SLOT(SbfreezingChanged(int)));
}

LivewireWidget::~LivewireWidget()
{
	if (m_Lw != nullptr)
		delete m_Lw;
	if (m_Lwfirst != nullptr)
		delete m_Lwfirst;
}

void LivewireWidget::OnMouseClicked(Point p)
{
	if (!m_Drawing)
	{
		if (m_Autotrace->isChecked())
		{
			if (m_Lw == nullptr)
			{
				m_Lw = m_Bmphand->Livewireinit(p);
				m_Lwfirst = m_Bmphand->Livewireinit(p);
			}
			else
			{
				m_Lw->ChangePt(p);
				m_Lwfirst->ChangePt(p);
			}
			//		lw=bmphand->livewireinit(p);
			//		lwfirst=bmphand->livewireinit(p);
		}
		m_P1 = m_P2 = p;
		m_Clicks.clear();
		m_Establishedlengths.clear();
		m_Clicks.push_back(p);
		m_Establishedlengths.push_back(0);
		m_Drawing = true;
		if (m_Cooling)
		{
			m_Dynamicold.clear();
			m_Times.clear();
		}
	}
	else
	{
		if (m_Straight->isChecked())
		{
			addLine(&m_Established, m_P1, p);
			m_Dynamic.clear();
			if (m_CbClosing->isChecked())
				addLine(&m_Dynamic, m_P2, p);
			m_P1 = p;
		}
		else
		{
			m_Lw->ReturnPath(p, &m_Dynamic);
			m_Established.insert(m_Established.end(), m_Dynamic.begin(), m_Dynamic.end());
			if (m_CbClosing->isChecked())
				m_Lwfirst->ReturnPath(p, &m_Dynamic);
			else
				m_Dynamic.clear();
			m_Lw->ChangePt(p);
			if (m_Cooling)
			{
				m_Times.clear();
				m_Dynamicold.clear();
			}
		}
		m_Establishedlengths.push_back(static_cast<int>(m_Established.size()));
		m_Clicks.push_back(p);

		emit Vp1dynChanged(&m_Established, &m_Dynamic);
	}
}

void LivewireWidget::PtDoubleclicked(Point p)
{
	if (m_Drawing && !m_Freedraw->isChecked())
	{
		m_Clicks.push_back(p);
		if (m_Straight->isChecked())
		{
			addLine(&m_Established, m_P1, p);
			addLine(&m_Established, m_P2, p);
		}
		else if (m_Autotrace->isChecked())
		{
			m_Lw->ReturnPath(p, &m_Dynamic);
			m_Established.insert(m_Established.end(), m_Dynamic.begin(), m_Dynamic.end());
			m_Lwfirst->ReturnPath(p, &m_Dynamic);
			m_Established.insert(m_Established.end(), m_Dynamic.begin(), m_Dynamic.end());
		}

		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);

		m_Bmphand->FillContour(&m_Established, true);

		emit EndDatachange(this);

		m_Dynamic.clear();
		m_Established.clear();
		if (m_Cooling)
		{
			m_Dynamicold.clear();
			m_Times.clear();
		}

		m_Drawing = false;

		emit Vp1dynChanged(&m_Established, &m_Dynamic);
	}
}

void LivewireWidget::PtMidclicked(Point p)
{
	if (m_Drawing)
	{
		if (m_Clicks.size() == 1)
		{
			PtDoubleclickedmid(p);
			return;
		}
		else
		{
			if (m_Straight->isChecked())
			{
				m_Established.clear();
				m_Dynamic.clear();
				m_Clicks.pop_back();
				m_Establishedlengths.pop_back();
				m_P1 = m_Clicks.back();
				addLine(&m_Dynamic, m_P1, p);
				auto it = m_Clicks.begin();
				Point pp = m_P2;
				it++;
				while (it != m_Clicks.end())
				{
					addLine(&m_Established, pp, *it);
					pp = *it;
					it++;
				}
				if (m_CbClosing->isChecked())
					addLine(&m_Dynamic, m_P2, p);
				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
			else if (m_Autotrace->isChecked())
			{
				m_Dynamic.clear();
				m_Clicks.pop_back();
				m_Establishedlengths.pop_back();
				auto it1 = m_Established.begin();
				for (int i = 0; i < m_Establishedlengths.back(); i++)
					it1++;
				m_Established.erase(it1, m_Established.end());
				m_P1 = m_Clicks.back();
				m_Lw->ChangePt(m_P1);
				m_Lw->ReturnPath(p, &m_Dynamic);
				if (m_Cooling)
				{
					m_Times.resize(m_Dynamic.size());
					QTime now = QTime::currentTime();
					auto tit = m_Times.begin();
					while (tit != m_Times.end())
					{
						*tit = now;
						tit++;
					}
					m_Dynamicold.clear();
				}
				if (m_CbClosing->isChecked())
					m_Lwfirst->ReturnPath(p, &m_Dynamic);

				emit Vp1dynChanged(&m_Established, &m_Dynamic);
			}
		}
	}
}

void LivewireWidget::PtDoubleclickedmid(Point p)
{
	if (m_Drawing)
	{
		m_Dynamic.clear();
		m_Established.clear();
		m_Establishedlengths.clear();
		m_Clicks.clear();
		m_Drawing = false;

		if (m_Cooling)
		{
			m_Dynamicold.clear();
			m_Times.clear();
		}
		emit Vp1dynChanged(&m_Established, &m_Dynamic);
	}
}

void LivewireWidget::OnMouseReleased(Point p)
{
	if (m_Freedraw->isChecked() && m_Drawing)
	{
		m_Clicks.push_back(p);
		addLine(&m_Dynamic, m_P1, p);
		addLine(&m_Dynamic, m_P2, p);

		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);

		m_Bmphand->FillContour(&m_Dynamic, true);

		emit EndDatachange(this);

		m_Dynamic.clear();
		m_Dynamic1.clear();
		m_Established.clear();

		m_Drawing = false;

		emit Vp1dynChanged(&m_Established, &m_Dynamic);
	}
}

void LivewireWidget::OnMouseMoved(Point p)
{
	if (m_Drawing)
	{
		if (m_Freedraw->isChecked())
		{
			m_Dynamic1.clear();
			addLine(&m_Dynamic, m_P1, p);
			m_Dynamic1.insert(m_Dynamic1.begin(), m_Dynamic.begin(), m_Dynamic.end());
			addLine(&m_Dynamic1, m_P2, p);
			emit VpdynChanged(&m_Dynamic1);
			m_P1 = p;
			m_Clicks.push_back(p);
		}
		else if (m_Straight->isChecked())
		{
			m_Dynamic.clear();
			addLine(&m_Dynamic, m_P1, p);
			if (m_CbClosing->isChecked())
				addLine(&m_Dynamic, m_P2, p);
			emit VpdynChanged(&m_Dynamic);
		}
		else
		{
			m_Lw->ReturnPath(p, &m_Dynamic);
			if (m_CbClosing->isChecked())
				m_Lwfirst->ReturnPath(p, &m_Dynamic1);

			if (m_Cooling)
			{
				std::vector<Point>::reverse_iterator rit, ritold;
				rit = m_Dynamic.rbegin();
				ritold = m_Dynamicold.rbegin();
				m_Times.resize(m_Dynamic.size());
				auto tit = m_Times.begin();
				while (rit != m_Dynamic.rend() && ritold != m_Dynamicold.rend() &&
							 (*rit).px == (*ritold).px && (*rit).py == (*ritold).py)
				{
					rit++;
					ritold++;
					tit++;
				}

				QTime now = QTime::currentTime();
				while (tit != m_Times.end())
				{
					*tit = now;
					tit++;
				}

				m_Dynamicold.clear();
				m_Dynamicold.insert(m_Dynamicold.begin(), m_Dynamic.begin(), m_Dynamic.end());

				tit = m_Times.begin();

				if (tit->msecsTo(now) >= m_Tlimit2)
				{
					rit = m_Dynamic.rbegin();
					while (tit != m_Times.end() && tit->msecsTo(now) >= m_Tlimit1)
					{
						tit++;
						rit++;
					}

					rit--;
					m_Lw->ChangePt(*rit);
					rit++;
					m_Established.insert(m_Established.end(), m_Dynamic.rbegin(), rit);
					m_Times.erase(m_Times.begin(), tit);

					if (m_CbClosing->isChecked())
						m_Dynamic.insert(m_Dynamic.end(), m_Dynamic1.begin(), m_Dynamic1.end());
					emit Vp1dynChanged(&m_Established, &m_Dynamic);
				}
				else
				{
					if (m_CbClosing->isChecked())
						m_Dynamic.insert(m_Dynamic.end(), m_Dynamic1.begin(), m_Dynamic1.end());
					emit VpdynChanged(&m_Dynamic);
				}
			}
			else
			{
				if (m_CbClosing->isChecked())
					m_Dynamic.insert(m_Dynamic.end(), m_Dynamic1.begin(), m_Dynamic1.end());
				emit VpdynChanged(&m_Dynamic);
			}
		}
	}
}

void LivewireWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		m_Bmphand = m_Handler3D->GetActivebmphandler();

		m_Dynamic.clear();
		m_Established.clear();
		m_Clicks.clear();
		m_Establishedlengths.clear();

		if (m_Cooling)
		{
			m_Dynamicold.clear();
			m_Times.clear();
		}

		if (m_Lw != nullptr)
		{
			delete m_Lw;
			m_Lw = nullptr;
		}
		if (m_Lwfirst != nullptr)
		{
			delete m_Lwfirst;
			m_Lwfirst = nullptr;
		}

		emit Vp1dynChanged(&m_Established, &m_Dynamic);
	}

	Init1();

	HideParamsChanged();
}

void LivewireWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Dynamic.clear();
	m_Established.clear();
	m_Clicks.clear();
	m_Establishedlengths.clear();

	if (m_Cooling)
	{
		m_Dynamicold.clear();
		m_Times.clear();
	}

	if (m_Lw != nullptr)
	{
		delete m_Lw;
		m_Lw = nullptr;
	}
	if (m_Lwfirst != nullptr)
	{
		delete m_Lwfirst;
		m_Lwfirst = nullptr;
	}
}

void LivewireWidget::Init1()
{
	Point p;
	p.px = p.py = 0;

	m_Drawing = false;
}

void LivewireWidget::Cleanup()
{
	m_Dynamic.clear();
	m_Established.clear();
	m_Clicks.clear();
	m_Establishedlengths.clear();

	if (m_Cooling)
	{
		m_Dynamicold.clear();
		m_Times.clear();
	}

	m_Drawing = false;
	if (m_Lw != nullptr)
		delete m_Lw;
	if (m_Lw != nullptr)
		delete m_Lwfirst;
	m_Lw = m_Lwfirst = nullptr;

	emit Vp1dynChanged(&m_Established, &m_Dynamic);
}

void LivewireWidget::BmpChanged()
{
	Cleanup();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	Init1();
}

void LivewireWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void LivewireWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;

	m_Dynamic.clear();
	m_Established.clear();
	m_Clicks.clear();
	m_Establishedlengths.clear();

	if (m_Cooling)
	{
		m_Dynamicold.clear();
		m_Times.clear();
	}

	if (m_Lw != nullptr)
	{
		delete m_Lw;
		m_Lw = nullptr;
	}
	if (m_Lwfirst != nullptr)
	{
		delete m_Lwfirst;
		m_Lwfirst = nullptr;
	}

	Init1();

	emit Vp1dynChanged(&m_Established, &m_Dynamic);
}

void LivewireWidget::ModeChanged()
{
	m_CbFreezing->setEnabled(m_Autotrace->isChecked());
	m_ParamsLayout->labelForField(m_CbFreezing)->setEnabled(m_Autotrace->isChecked());
	FreezingChanged();

	m_CbClosing->setEnabled(!m_Freedraw->isChecked());
	m_ParamsLayout->labelForField(m_CbClosing)->setEnabled(m_CbClosing->isEnabled());
}

void LivewireWidget::FreezingChanged()
{
	m_Cooling = m_CbFreezing->isChecked();
	m_SbFreezing->setEnabled(m_Cooling && m_Autotrace->isChecked());
	m_ParamsLayout->labelForField(m_SbFreezing)->setEnabled(m_SbFreezing->isEnabled());
}

void LivewireWidget::SbfreezingChanged(int i)
{
	m_Tlimit1 = i * 1000;
	m_Tlimit2 = (float(i) + 0.5f) * 1000;
}

FILE* LivewireWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SbFreezing->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)m_Autotrace->isChecked();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)m_Straight->isChecked();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)m_Freedraw->isChecked();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_CbFreezing->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_CbClosing->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* LivewireWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		//QObject_disconnect(drawmode, SIGNAL(buttonClicked(int)), this, SLOT(ModeChanged()));
		QObject_disconnect(m_CbFreezing, SIGNAL(clicked()), this, SLOT(FreezingChanged()));
		QObject_disconnect(m_SbFreezing, SIGNAL(valueChanged(int)), this, SLOT(SbfreezingChanged(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SbFreezing->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Autotrace->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Straight->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Freedraw->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_CbFreezing->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_CbClosing->setChecked(dummy > 0);

		SbfreezingChanged(m_SbFreezing->value());
		FreezingChanged();
		ModeChanged();

		//QObject_connect(drawmode, SIGNAL(buttonClicked(int)), this, SLOT(ModeChanged()));
		QObject_connect(m_CbFreezing, SIGNAL(clicked()), this, SLOT(FreezingChanged()));
		QObject_connect(m_SbFreezing, SIGNAL(valueChanged(int)), this, SLOT(SbfreezingChanged(int)));
	}
	return fp;
}

void LivewireWidget::HideParamsChanged()
{
	ModeChanged();
}

} // namespace iseg
