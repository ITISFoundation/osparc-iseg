/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "SlicesHandler.h"

#include <QCheckBox>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>

namespace iseg {

class UndoConfigurationDialog : public QDialog
{
	Q_OBJECT
public:
	UndoConfigurationDialog(SlicesHandler* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~UndoConfigurationDialog() override;

private:
	SlicesHandler* m_Handler3D;
	QCheckBox* m_CbUndo3D;
	QSpinBox* m_SbNrundo;
	QSpinBox* m_SbNrundoarrays;
	QPushButton* m_PbClose;

private slots:
	void OkPressed();
};

} // namespace iseg
