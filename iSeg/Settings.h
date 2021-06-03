/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	~Settings() override;

public slots:
	void accept() override;
	void reject() override;

private:
	Ui::Settings* m_Ui;
	MainWindow* m_MainWindow;
};

} // namespace iseg

#endif // SETTINGS_H
