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

#include <QApplication>
#include <QPlastiqueStyle>

namespace iseg {

void SetPlastiqueStyle(QApplication* app)
{
	QApplication::setStyle(new QPlastiqueStyle);

	QPalette palette;
	palette = app->palette();
	palette.setColor(QPalette::Window, QColor(40, 40, 40));
	palette.setColor(QPalette::Base, QColor(70, 70, 70));
	palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
	palette.setColor(QPalette::Text, QColor(255, 255, 255));
	palette.setColor(QPalette::Button, QColor(70, 70, 70));
	palette.setColor(QPalette::Highlight, QColor(150, 150, 150));
	palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
	palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
	app->setPalette(palette);
	// needed on MacOS, but not WIN32?
	app->setStyleSheet(
			"QWidget { color: white; }"
			//"QPushButton:checked { background-color: rgb(150,150,150); font: bold }"
			"QWidget:disabled { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QPushButton:disabled { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QLabel:disabled { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QLineEdit:disabled { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QCheckBox:disabled { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QComboBox:disabled { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QComboBox { background: rgb(40,40,40); border-radius: 5px; }"
			"QComboBox::drop-down { border-left-color: rgb(40,40,40); }"
			// https://doc.qt.io/qt-5/stylesheet-examples.html
			"QTreeView::branch:!has-children:!has-siblings:adjoins-item { background: palette(window); }"
			"QTreeView::branch:!has-children:has-siblings:adjoins-item { background: palette(window); }"
			"QTreeView::branch:!has-children:has-siblings:!adjoins-item { background: palette(window); }"
			"QTreeView::branch:!has-children:has-siblings:!adjoins-item { background: palette(window); }");
	//"QComboBox::down-arrow { border: 1px rgb(40,40,40); }"
	//"QTreeWidget::item { height: 20px; }");
	//"QTreeView::branch { border-image: url(none.png); }"
	//"QTreeWidget::item { padding: 2px; height: 20px; }");
	//"QTreeWidget::branch:selected { border-top : 0px solid #8d8d8d; border-bottom : 0px solid #8d8d8d; }"
}

} // namespace iseg