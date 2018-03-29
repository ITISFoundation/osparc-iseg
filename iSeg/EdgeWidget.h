/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef EDGEWIDGET_3MARCH05
#define EDGEWIDGET_3MARCH05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Plugin/WidgetInterface.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class edge_widget : public WidgetInterface
{
	Q_OBJECT
public:
	edge_widget(SlicesHandler* hand3D, QWidget* parent = 0,
				const char* name = 0, Qt::WindowFlags wFlags = 0);
	~edge_widget();
	QSize sizeHint() const;
	void init();
	void newloaded();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	void hideparams_changed();
	std::string GetName() { return std::string("Edge"); };
	//virtual QIcon GetIcon() {return QIcon("images/edge.png");};
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("edge.png")).ascii());
	};

private:
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hboxoverall;
	Q3VBox* vboxmethods;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3VBox* vbox1;
	QLabel* txt_sigmal;
	QLabel* txt_sigma2;
	QLabel* txt_thresh11;
	QLabel* txt_thresh12;
	QLabel* txt_thresh21;
	QLabel* txt_thresh22;
	QSlider* sl_sigma;
	QSlider* sl_thresh1;
	QSlider* sl_thresh2;
	QPushButton* btn_exec;
	QRadioButton* rb_sobel;
	QRadioButton* rb_laplacian;
	QRadioButton* rb_interquartile;
	QRadioButton* rb_momentline;
	QRadioButton* rb_gaussline;
	QRadioButton* rb_canny;
	QRadioButton* rb_laplacianzero;
	QButtonGroup* modegroup;

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
	void method_changed(int);
	void slider_changed(int newval);
};

} // namespace iseg

#endif
