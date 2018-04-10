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

#include "Plugin/SlicesHandlerInterface.h"
#include "Plugin/WidgetInterface.h"

#include <q3vbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>

class LevelsetWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	LevelsetWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~LevelsetWidget();
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	std::string GetName() override { return std::string("LevelSet"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("LevelSet.png"))); };

private:
	void on_slicenr_changed() override;

	unsigned int* usp;
	iseg::SliceHandlerInterface* handler3D;
	unsigned short activeslice;
	Q3VBox* vbox1;
	QLabel* bias_header;
	QPushButton* bias_exec;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3HBox* hbox5;
	QLabel* txt_h2;
	QLabel* txt_h3;
	QLabel* txt_h4;
	QSpinBox* sl_h2;
	QSpinBox* sl_h3;
	QSpinBox* sl_h4;
	QLabel* txt_h5;
	QSpinBox* sl_h5;

private slots:
	void do_work();
};
