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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>

#include <initializer_list>

namespace iseg {

inline QHBoxLayout* make_hbox(std::initializer_list<QWidget*> list)
{
	auto box = new QHBoxLayout;
	for (auto w : list)
	{
		box->addWidget(w);
	}
	return box;
}

inline QVBoxLayout* make_vbox(std::initializer_list<QWidget*> list)
{
	auto box = new QVBoxLayout;
	for (auto w : list)
	{
		box->addWidget(w);
	}
	return box;
}

inline QButtonGroup* make_button_group(QWidget* parent, std::initializer_list<QRadioButton*> list)
{
	auto group = new QButtonGroup(parent);
	for (auto w : list)
	{
		group->addButton(w);
	}
	return group;
}

inline QButtonGroup* make_button_group(std::initializer_list<QString> button_names, const int checked_id = -1)
{
	auto group = new QButtonGroup;
	int id = 0;
	for (const auto &n : button_names)
	{
		auto rb = new QRadioButton(n);
		rb->setChecked(id == checked_id);
		group->addButton(rb, id++);
	}
	return group;
}

}