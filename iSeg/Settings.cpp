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

#include "Settings.h"
#include "ui_Settings.h"

#include "MainWindow.h"
#include "SlicesHandler.h"

#include "../Core/HDF5Blosc.h"

#include <cassert>
#include <iostream>

namespace iseg {

Settings::Settings(QWidget* parent)
		: QDialog(parent), m_MainWindow((MainWindow*)parent), m_Ui(new Ui::Settings)
{
	assert(m_MainWindow);
	m_Ui->setupUi(this);

	this->m_Ui->spinBoxCompression->setValue(m_MainWindow->m_Handler3D->GetCompression());
	this->m_Ui->checkBoxContiguousMemory->setChecked(m_MainWindow->m_Handler3D->GetContiguousMemory());
	this->m_Ui->checkBoxEnableBlosc->setChecked(BloscEnabled());
	this->m_Ui->checkBoxSaveTarget->setChecked(m_MainWindow->m_Handler3D->SaveTarget());
}

Settings::~Settings() { delete m_Ui; }

void Settings::accept()
{
	ISEG_INFO("setting compression = " << this->m_Ui->spinBoxCompression->value());
	m_MainWindow->m_Handler3D->SetCompression(this->m_Ui->spinBoxCompression->value());
	m_MainWindow->m_Handler3D->SetContiguousMemory(this->m_Ui->checkBoxContiguousMemory->isChecked());
	SetBloscEnabled(this->m_Ui->checkBoxEnableBlosc->isChecked());
	m_MainWindow->m_Handler3D->SetSaveTarget(this->m_Ui->checkBoxSaveTarget->isChecked());

	m_MainWindow->SaveSettings();
	this->hide();
}

void Settings::reject()
{
	this->hide();
}

} // namespace iseg