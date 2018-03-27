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
#include <cassert>
#include <iostream>

#include "Settings.h"
#include "ui_Settings.h"

#include "MainWindow.h"
#include "SlicesHandler.h"

Settings::Settings(QWidget *parent)
		: QDialog(parent), mainWindow((MainWindow *)parent), ui(new Ui::Settings)
{
	std::cerr << "Settings::Settings()" << std::endl;
	assert(mainWindow);
	ui->setupUi(this);
	std::cerr << "using compression = " << mainWindow->handler3D->GetCompression()
						<< std::endl;
	this->ui->spinBoxCompression->setValue(
			mainWindow->handler3D->GetCompression());
	this->ui->checkBoxContiguousMemory->setChecked(
			mainWindow->handler3D->GetContiguousMemory());
}

Settings::~Settings() { delete ui; }

void Settings::accept()
{
	std::cerr << "Settings::accept()" << std::endl;
	std::cerr << "setting compression = " << this->ui->spinBoxCompression->value()
						<< std::endl;
	mainWindow->handler3D->SetCompression(this->ui->spinBoxCompression->value());
	mainWindow->handler3D->SetContiguousMemory(
			this->ui->checkBoxContiguousMemory->isChecked());
	mainWindow->SaveSettings();
	this->hide();
}

void Settings::reject()
{
	std::cerr << "Settings::reject()" << std::endl;
	this->hide();
}
