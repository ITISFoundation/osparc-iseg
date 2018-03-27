/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

class MainWindow;

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
	Q_OBJECT

public:
	explicit Settings(QWidget *parent = 0);
	~Settings();

public slots:
	virtual void accept();
	virtual void reject();

private:
	Ui::Settings *ui;
	MainWindow *mainWindow;
};

#endif // SETTINGS_H
