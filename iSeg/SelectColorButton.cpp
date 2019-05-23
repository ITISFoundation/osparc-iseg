/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "SelectColorButton.h"

#include <QColorDialog>

namespace iseg {

SelectColorButton::SelectColorButton(QWidget* parent)
	: color(255, 255, 255) // default white
{
	updateColor();

	connect(this, SIGNAL(clicked()), this, SLOT(changeColor()));
}

void SelectColorButton::updateColor()
{
	setStyleSheet("background-color: " + color.name());
}

void SelectColorButton::changeColor()
{
	QColor new_color = QColorDialog::getColor(color, parentWidget(), "Select Outline Color");
	if (new_color.isValid() && new_color != color)
	{
		setColor(new_color);
		onColorChanged(new_color);
	}
}

void SelectColorButton::setColor(const QColor& color)
{
	this->color = color;
	updateColor();
}

const QColor& SelectColorButton::getColor()
{
	return color;
}

}
