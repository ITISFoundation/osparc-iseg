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

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <qpushbutton.h>
#include <qradiobutton.h>

namespace iseg {

class PickerWidget : public WidgetInterface
{
	Q_OBJECT
public:
	PickerWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~PickerWidget();
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void init() override;
	void cleanup() override;
	void newloaded() override;
	std::string GetName() override { return std::string("Picker"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("picker.png"))); }

private:
	void on_slicenr_changed() override;

	void on_mouse_clicked(Point p) override;

	void update_active();
	void showborder();

	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned int width;
	unsigned int height;
	bool hasclipboard;
	bool shiftpressed;
	bool clipboardworkortissue;
	bool* mask;
	bool* currentselection;
	float* valuedistrib;
	unsigned char mode;

	QPushButton* pb_copy;
	QPushButton* pb_paste;
	QPushButton* pb_cut;
	QPushButton* pb_delete;
	QRadioButton* rb_work;
	QRadioButton* rb_tissue;
	QRadioButton* rb_erase;
	QRadioButton* rb_fill;

	std::vector<Point> selection;

signals:
	void vp1_changed(std::vector<Point>* vp1);

private slots:
	void copy_pressed();
	void cut_pressed();
	void paste_pressed();
	void delete_pressed();

	void worktissue_changed(int);
	void bmphand_changed(bmphandler* bmph);

protected:
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
};

} // namespace iseg

