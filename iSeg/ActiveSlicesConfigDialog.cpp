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

#include "ActiveSlicesConfigDialog.h"
#include "SlicesHandler.h"

#include "Interface/QtConnect.h"

#include <QFormLayout>

namespace iseg {

ActiveSlicesConfigDialog::ActiveSlicesConfigDialog(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_Handler3D(hand3D)
{
	m_SbStart = new QSpinBox(1, (int)m_Handler3D->NumSlices(), 1, nullptr);
	m_SbStart->setValue((int)m_Handler3D->StartSlice() + 1);

	m_SbEnd = new QSpinBox(1, (int)m_Handler3D->NumSlices(), 1, nullptr);
	m_SbEnd->setValue((int)m_Handler3D->EndSlice());

	m_PbOk = new QPushButton("OK");
	m_PbClose = new QPushButton("Cancel");

	// layout
	auto layout = new QFormLayout;
	layout->addRow(tr("Start slice"), m_SbStart);
	layout->addRow(tr("End slice"), m_SbEnd);
	layout->addRow(m_PbOk, m_PbClose);

	setLayout(layout);

	// connections
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_PbOk, SIGNAL(clicked()), this, SLOT(OkPressed()));
	QObject_connect(m_SbStart, SIGNAL(valueChanged(int)), this, SLOT(StartsliceChanged(int)));
	QObject_connect(m_SbEnd, SIGNAL(valueChanged(int)), this, SLOT(EndsliceChanged(int)));
}

void ActiveSlicesConfigDialog::OkPressed()
{
	if (m_SbStart->value() > m_SbEnd->value())
		return;

	m_Handler3D->SetStartslice(m_SbStart->value() - 1);
	m_Handler3D->SetEndslice(m_SbEnd->value());

	close();
}

void ActiveSlicesConfigDialog::StartsliceChanged(int startslice1)
{
	if (startslice1 > m_SbEnd->value())
	{
		//sb_end->setValue(startslice1);
		m_PbOk->setEnabled(false);
	}
	else
	{
		m_PbOk->setEnabled(true);
	}
}

void ActiveSlicesConfigDialog::EndsliceChanged(int endslice1)
{
	if (endslice1 < m_SbStart->value())
	{
		m_PbOk->setEnabled(false);
	}
	else
	{
		m_PbOk->setEnabled(true);
	}
}

} // namespace iseg
