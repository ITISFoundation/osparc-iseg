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

#include <QColor>
#include <QPushButton>

namespace iseg {

class SelectColorButton : public QPushButton
{
	Q_OBJECT
public:
	SelectColorButton(QWidget* parent);

	void SetColor(const QColor& color);
	const QColor& GetColor();

signals:
	void OnColorChanged(QColor color);

private slots:
	void UpdateColor();
	void ChangeColor();

private:
	QColor m_Color;
};

} // namespace iseg
