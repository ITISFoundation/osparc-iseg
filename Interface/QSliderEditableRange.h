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
#pragma clang system_header

#include "iSegInterface.h"

#include <QWidget>

class QLineEdit;
class QSlider;

namespace iseg {

class ISEG_INTERFACE_API QSliderEditableRange : public QWidget
{
	Q_OBJECT
public:
	QSliderEditableRange(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);

	void setValue(int v);
	int value() const;

	void setRange(int vmin, int vmax);

	void setMinimum(int v);
	int minimum() const;

	void setMaximum(int v);
	int maximum() const;

	void setMinimumVisible(bool v);
	void setMaximumVisible(bool v);

signals:
	void valueChanged(int value);

	void sliderPressed();
	void sliderMoved(int position);
	void sliderReleased();
	void rangeChanged(int min, int max);
	void actionTriggered(int action);

private slots:
	void Edited();

private:
	QSlider* m_Slider;
	QLineEdit* m_MinEdit;
	QLineEdit* m_MaxEdit;
};

} // namespace iseg
