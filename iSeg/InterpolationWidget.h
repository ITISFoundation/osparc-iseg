/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef Interpolate_20April05
#define Interpolate_20April05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Plugin/WidgetInterface.h"

#include <q3listbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

class InterpolationWidget : public WidgetInterface
{
	Q_OBJECT
public:
	InterpolationWidget(SlicesHandler* hand3D, QWidget* parent = 0,
						const char* name = 0, Qt::WindowFlags wFlags = 0);
	~InterpolationWidget();
	QSize sizeHint() const;
	void init();
	void newloaded();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	std::string GetName() { return std::string("Interpol"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("interpolate.png")).ascii());
	};

private:
	SlicesHandler* handler3D;
	Q3HBox* hboxoverall;
	Q3VBox* vboxmethods;
	Q3VBox* vboxdataselect;
	Q3VBox* vboxparams;
	Q3VBox* vboxexecute;
	Q3HBox* hboxextra;
	Q3HBox* hboxbatch;
	QLabel* txt_slicenr;
	QSpinBox* sb_slicenr;
	QLabel* txt_batchstride;
	QSpinBox* sb_batchstride;
	QPushButton* pushexec;
	QPushButton* pushstart;
	QRadioButton* rb_tissue;
	QRadioButton* rb_tissueall;
	QRadioButton* rb_work;
	QButtonGroup* sourcegroup;
	QRadioButton* rb_inter;
	//	QRadioButton *rb_intergrey;
	QRadioButton* rb_extra;
	QRadioButton* rb_batchinter;
	QButtonGroup* modegroup;
	QRadioButton* rb_4connectivity;
	QRadioButton* rb_8connectivity;
	QButtonGroup* connectivitygroup;
	QCheckBox* cb_medianset;
	unsigned short startnr;
	unsigned short nrslices;
	unsigned short tissuenr;

signals:
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void slicenr_changed();
	void tissuenr_changed(int tissuetype);
	void handler3D_changed();

private slots:
	void startslice_pressed();
	void execute();
	void method_changed();
	void source_changed();
};

} // namespace iseg

#endif
