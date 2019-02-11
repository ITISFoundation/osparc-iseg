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

#include "SlicesHandler.h"

#include <qdialog.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qspinbox.h>

namespace iseg {

class UndoConfigurationDialog : public QDialog
{
	Q_OBJECT
public:
	UndoConfigurationDialog(SlicesHandler* hand3D, QWidget* parent = 0, const char* name = 0,
							Qt::WindowFlags wFlags = 0);
	~UndoConfigurationDialog();

private:
	SlicesHandler* handler3D;
	QCheckBox* cb_undo3D;
	QSpinBox* sb_nrundo;
	QSpinBox* sb_nrundoarrays;
	QPushButton* pb_close;

private slots:
	void ok_pressed();
};

} // namespace iseg

