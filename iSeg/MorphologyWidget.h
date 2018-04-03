/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef MORPHO_4MARCH05
#define MORPHO_4MARCH05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Plugin/WidgetInterface.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

class MorphologyWidget : public WidgetInterface
{
	Q_OBJECT
public:
	MorphologyWidget(SlicesHandler* hand3D, QWidget* parent = 0,
					 const char* name = 0, Qt::WindowFlags wFlags = 0);
	~MorphologyWidget();
	QSize sizeHint() const;
	void init();
	void newloaded();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	void hideparams_changed();
	std::string GetName() { return std::string("Morpho"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("morphology.png")).ascii());
	};

private:
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hboxoverall;
	Q3VBox* vboxmethods;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3VBox* vbox1;
	QLabel* txt_n;
	QSpinBox* sb_n;
	QPushButton* btn_exec;
	QRadioButton* rb_8connect;
	QRadioButton* rb_4connect;
	QButtonGroup* connectgroup;
	QRadioButton* rb_open;
	QRadioButton* rb_close;
	QRadioButton* rb_erode;
	QRadioButton* rb_dilate;
	QButtonGroup* modegroup;
	QCheckBox* allslices;

signals:
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler* bmph);
	void execute();
};

} // namespace iseg

#endif
