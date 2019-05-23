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

#include <QColor>
#include <QPushButton>

namespace iseg
{

class SelectColorButton : public QPushButton
{
	Q_OBJECT
public:
	SelectColorButton(QWidget* parent);

	void setColor(const QColor& color);
	const QColor& getColor();

signals:
	void onColorChanged(QColor color);

private slots:
	void updateColor();
	void changeColor();

private:
	QColor color;
};

} // namespace iseg
