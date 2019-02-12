/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include <qdialog.h>
#include <qpushbutton.h>
#include <qspinbox.h>

namespace iseg {

class SlicesHandler;

class ActiveSlicesConfigDialog : public QDialog
{
	Q_OBJECT
public:
	ActiveSlicesConfigDialog(SlicesHandler* hand3D, QWidget* parent = 0,
							 const char* name = 0, Qt::WindowFlags wFlags = 0);

private:
	SlicesHandler* handler3D;

	QSpinBox* sb_start;
	QSpinBox* sb_end;
	QPushButton* pb_close;
	QPushButton* pb_ok;

private slots:
	void ok_pressed();
	void startslice_changed(int startslice1);
	void endslice_changed(int endslice1);
};

} // namespace iseg
