/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef PICKERWIDGET_15DEZ08
#define PICKERWIDGET_15DEZ08

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Plugin/WidgetInterface.h"

#include "Core/DataSelection.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class PickerWidget : public WidgetInterface
{
	Q_OBJECT
public:
	PickerWidget(SlicesHandler* hand3D, QWidget* parent = 0,
				 const char* name = 0, Qt::WindowFlags wFlags = 0);
	~PickerWidget();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	QSize sizeHint() const;
	void init();
	void cleanup();
	void newloaded();
	std::string GetName() { return std::string("Picker"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("picker.png")).ascii());
	};

public slots:
	void slicenr_changed();

private:
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
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3VBox* vbox1;
	QPushButton* pb_copy;
	QPushButton* pb_paste;
	QPushButton* pb_cut;
	QPushButton* pb_delete;
	QRadioButton* rb_work;
	QRadioButton* rb_tissue;
	QButtonGroup* worktissuegroup;
	QRadioButton* rb_erase;
	QRadioButton* rb_fill;
	QButtonGroup* erasefillgroup;
	std::vector<Point> selection;
	void update_active();
	void showborder();

signals:
	void vp1_changed(std::vector<Point>* vp1);
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

private slots:
	void copy_pressed();
	void cut_pressed();
	void paste_pressed();
	void delete_pressed();

	void worktissue_changed(int);
	void bmphand_changed(bmphandler* bmph);
	void pt_clicked(Point);

protected:
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
};

} // namespace iseg

#endif
