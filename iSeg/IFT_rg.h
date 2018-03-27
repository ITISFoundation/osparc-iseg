/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef IFTRG_29March05
#define IFTRG_29March05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Addon/qwidget1.h"

#include "Core/Point.h"
#include "Core/common.h"

#include <q3hbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class IFTrg_widget : public QWidget1
{
	Q_OBJECT
public:
	IFTrg_widget(SlicesHandler* hand3D, QWidget* parent = 0,
				 const char* name = 0, Qt::WindowFlags wFlags = 0);
	~IFTrg_widget();
	void init();
	void newloaded();
	void cleanup();
	QSize sizeHint() const;
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	void hideparams_changed();
	std::string GetName() { return std::string("IFT"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("iftrg.png")).ascii());
	};

private:
	void init1();
	void removemarks(Point p);
	float* lbmap;
	IFT_regiongrowing* IFTrg;
	Point last_pt;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hbox1;
	Q3VBox* vbox1;
	QLabel* txt_lower;
	QLabel* txt_upper;
	QSlider* sl_thresh;
	QPushButton* pushexec;
	QPushButton* pushclear;
	QPushButton* pushremove;
	void getrange();
	unsigned tissuenr;
	//	unsigned maxim;
	float thresh;
	float maxthresh;
	std::vector<Mark> vm;
	std::vector<Mark> vmempty;
	std::vector<Point> vmdyn;
	//	std::vector<vector<mark> > vvm;
	unsigned area;

signals:
	void vm_changed(std::vector<Mark>* vm1);
	void vmdyn_changed(std::vector<Point>* vmdyn1);
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void tissuenr_changed(int i);
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler* bmph);
	//	void tissuenr_changed(int i);
	void mouse_clicked(Point p);
	void mouse_released(Point p);
	void mouse_moved(Point p);
	void execute();
	void clearmarks();
	//	void removepressed();
	void slider_changed(int i);
	void slider_pressed();
	void slider_released();
	void bmp_changed();
};

} // namespace iseg

#endif
