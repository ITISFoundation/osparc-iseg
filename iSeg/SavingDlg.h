/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <qmessagebox.h>
#include <qwidget.h>

class SavingDlg : public QMessageBox
{
	Q_OBJECT

public:
	SavingDlg(QWidget *parent = 0);

	int exec();

public slots:
	void accept();
	void OKButtonClicked(QAbstractButton *button);
	//void savingFinished();
};