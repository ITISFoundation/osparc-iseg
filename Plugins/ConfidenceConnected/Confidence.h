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
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>

class ConfidenceWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	ConfidenceWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
					 const char* name = 0, Qt::WindowFlags wFlags = 0);
	~ConfidenceWidget();
	QSize sizeHint() const;
	void init() override;
	void newloaded() override;
	std::string GetName() override { return std::string("ConfidenceFilter"); }
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("Confidence.png")).ascii());
	}

protected:
	virtual void on_tissuenr_changed(int i) override { std::cerr << "tissue " << i << std::endl; }
	virtual void on_slicenr_changed() override { std::cerr << "slice changed " << std::endl; }

	virtual void on_mouse_clicked(iseg::Point p) override { std::cerr << "on_mouse_clicked" << std::endl; }
	virtual void on_mouse_released(iseg::Point p) override { std::cerr << "on_mouse_released" << std::endl; }
	virtual void on_mouse_moved(iseg::Point p) override { std::cerr << "on_mouse_moved" << std::endl; }

private:
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
	QLabel* txt_h5;
	QSpinBox* sl_h2;
	QSpinBox* sl_h3;
	QSpinBox* sl_h4;
	QSpinBox* sl_h5;
	QSpinBox* sl_h6;
	QSpinBox* sl_h7;

public slots:
	void slicenr_changed();

private slots:
	void do_work();
};
