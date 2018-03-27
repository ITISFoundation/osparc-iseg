/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "SavingDlg.h"

SavingDlg::SavingDlg(QWidget *parent) : QMessageBox(parent)
{
	setModal(true);
	setIcon(QMessageBox::Critical);
	setWindowTitle("Saving");
	setText("Wait until the project is saved...");
	setWindowFlags(Qt::WindowStaysOnTopHint);

	//addButton("accept", QMessageBox::AcceptRole);
	//QObject::connect(parent,SIGNAL(buttonClicked(QAbstractButton)),this,SLOT(OKButtonClicked(QAbstractButton)));

	return;
}

int SavingDlg::exec()
{
	int result = QMessageBox::exec();
	return result;
}

void SavingDlg::accept()
{
	//do nothing
}

void SavingDlg::OKButtonClicked(QAbstractButton *button)
{
	//do nothing
}
/*
void SavingDlg::savingFinished()
{
	this->hide();
	this->close();
}*/