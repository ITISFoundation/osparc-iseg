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

namespace Ui {
class Settings;
}

namespace iseg {

class MainWindow;

class Settings : public QDialog
{
	Q_OBJECT

public:
	explicit Settings(QWidget* parent = nullptr);
	~Settings();

public slots:
	void accept() override;
	void reject() override;

private:
	Ui::Settings* ui;
	MainWindow* mainWindow;
};

} // namespace iseg

#endif // SETTINGS_H
