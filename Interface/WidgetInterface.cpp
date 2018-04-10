/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "WidgetInterface.h"

namespace iseg {

bool WidgetInterface::hideparams = false;

WidgetInterface::WidgetInterface(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	m_cursor = new QCursor(Qt::CrossCursor);
}

} // namespace iseg
