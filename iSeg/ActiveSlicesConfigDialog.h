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

#include <QDialog>
#include <QPushButton>
#include <QSpinBox>

namespace iseg {

class SlicesHandler;

class ActiveSlicesConfigDialog : public QDialog
{
	Q_OBJECT
public:
	ActiveSlicesConfigDialog(SlicesHandler* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

private:
	SlicesHandler* m_Handler3D;

	QSpinBox* m_SbStart;
	QSpinBox* m_SbEnd;
	QPushButton* m_PbClose;
	QPushButton* m_PbOk;

private slots:
	void OkPressed();
	void StartsliceChanged(int startslice1);
	void EndsliceChanged(int endslice1);
};

} // namespace iseg
