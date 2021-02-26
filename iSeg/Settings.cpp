/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	: QDialog(parent), mainWindow((MainWindow*)parent), ui(new Ui::Settings)
{
	assert(mainWindow);
	ui->setupUi(this);

	this->ui->spinBoxCompression->setValue(
		mainWindow->handler3D->GetCompression());
	this->ui->checkBoxContiguousMemory->setChecked(
		mainWindow->handler3D->GetContiguousMemory());
	this->ui->checkBoxEnableBlosc->setChecked(BloscEnabled());
    this->ui->checkBoxSaveTarget->setChecked(mainWindow->handler3D->SaveTarget());
}

Settings::~Settings() { delete ui; }

void Settings::accept()
{
	ISEG_INFO("setting compression = " << this->ui->spinBoxCompression->value());
	mainWindow->handler3D->SetCompression(
		this->ui->spinBoxCompression->value());
	mainWindow->handler3D->SetContiguousMemory(
		this->ui->checkBoxContiguousMemory->isChecked());
	SetBloscEnabled(this->ui->checkBoxEnableBlosc->isChecked());
    mainWindow->handler3D->SetSaveTarget(
        this->ui->checkBoxSaveTarget->isChecked());

	mainWindow->SaveSettings();
	this->hide();
}

void Settings::reject()
{
	this->hide();
}

}// namespace iseg