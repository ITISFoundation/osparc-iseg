/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "SelectColorButton.h"

#include "Interface/QtConnect.h"

#include <QColorDialog>

namespace iseg {

SelectColorButton::SelectColorButton(QWidget* parent)
		: m_Color(255, 255, 255) // default white
{
	UpdateColor();

	QObject_connect(this, SIGNAL(clicked()), this, SLOT(ChangeColor()));
}

void SelectColorButton::UpdateColor()
{
	setStyleSheet("background-color: " + m_Color.name());
}

void SelectColorButton::ChangeColor()
{
	QColor new_color = QColorDialog::getColor(m_Color, parentWidget(), "Select Outline Color");
	if (new_color.isValid() && new_color != m_Color)
	{
		SetColor(new_color);
		emit OnColorChanged(new_color);
	}
}

void SelectColorButton::SetColor(const QColor& color)
{
	this->m_Color = color;
	UpdateColor();
}

const QColor& SelectColorButton::GetColor()
{
	return m_Color;
}

} // namespace iseg
