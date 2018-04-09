/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SOW_19April05
#define SOW_19April05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include <q3hbox.h>
#include <q3listbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

class SaveOutlinesWidget : public QDialog
{
	Q_OBJECT
public:
	SaveOutlinesWidget(SlicesHandler* hand3D, QWidget* parent = 0,
					   const char* name = 0, Qt::WindowFlags wFlags = 0);
	~SaveOutlinesWidget();

private:
	SlicesHandler* handler3D;
	//	char *filename;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3HBox* hbox5;
	Q3HBox* hbox6;
	Q3HBox* hbox7;
	Q3HBox* hbox8;
	Q3HBox* hbox9;
	Q3HBox* hboxslicesbetween;
	Q3VBox* vbox1;
	Q3ListBox* lbo_tissues;
	QPushButton* pb_file;
	QPushButton* pb_exec;
	QPushButton* pb_close;
	QLineEdit* le_file;
	QButtonGroup* filetype;
	QButtonGroup* simplif;
	QRadioButton* rb_line;
	QRadioButton* rb_triang;
	QRadioButton* rb_none;
	QRadioButton* rb_dougpeuck;
	QRadioButton* rb_dist;
	QLabel* lb_simplif;
	QLabel* lb_file;
	QLabel* lb_dist;
	QLabel* lb_f1;
	QLabel* lb_f2;
	QLabel* lb_minsize;
	QLabel* lb_between;
	QSpinBox* sb_dist;
	QSpinBox* sb_minsize;
	QSpinBox* sb_between;
	QSlider* sl_f;
	QSpinBox* sb_topextrusion;
	QSpinBox* sb_bottomextrusion;
	QLabel* lb_topextrusion;
	QLabel* lb_bottomextrusion;
	QCheckBox* cb_extrusion;
	QCheckBox* cb_smooth;
	QCheckBox* cb_marchingcubes;

private slots:
	void mode_changed();
	void simplif_changed();
	void file_pushed();
	void save_pushed();
};

} // namespace iseg

#endif
