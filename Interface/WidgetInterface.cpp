/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "WidgetInterface.h"

#include <QCursor>

namespace iseg {

bool WidgetInterface::hideparams = false;

WidgetInterface::WidgetInterface(QWidget* parent)
		: QWidget(parent)
{
	m_MCursor = new QCursor(Qt::CrossCursor);
}

} // namespace iseg
