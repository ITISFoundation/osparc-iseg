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

#include "OutlineCorrectionParameterViews.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedLayout>

#include <initializer_list>

namespace iseg {

std::map<QAbstractButton*, int> _widget_page;

QHBoxLayout* make_hbox(std::initializer_list<QWidget*> list)
{
	auto hbox = new QHBoxLayout;
	for (auto w : list)
	{
		hbox->addWidget(w);
	}
	return hbox;
}

QButtonGroup* make_button_group(QWidget* parent, std::initializer_list<QRadioButton*> list)
{
	auto group = new QButtonGroup(parent);
	for (auto w : list)
	{
		group->addButton(w);
	}
	return group;
}

} // namespace iseg
