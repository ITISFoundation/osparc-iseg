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

#include "PluginApi.h"

#include "DataSelection.h"
#include "Point.h"

#include <qcursor.h>
#include <qdir.h>
#include <qicon.h>
#include <qwidget.h>

#define UNREFERENCED_PARAMETER(P) (P)

namespace iseg {

class ISEG_PLUGIN_API WidgetInterface : public QWidget
{
	Q_OBJECT
public:
	WidgetInterface(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
	{
		m_cursor = new QCursor(Qt::CrossCursor);
	}
	virtual void init() {}
	virtual void newloaded() {}
	virtual FILE* SaveParams(FILE* fp, int version)
	{
		UNREFERENCED_PARAMETER(version);
		return fp;
	}
	virtual FILE* LoadParams(FILE* fp, int version)
	{
		UNREFERENCED_PARAMETER(version);
		return fp;
	}
	virtual void hideparams_changed() {}
	static void set_hideparams(bool hide) { hideparams = hide; }
	static bool get_hideparams() { return hideparams; }
	virtual std::string GetName() { return std::string(""); }
	virtual QIcon GetIcon(QDir picdir) = 0;

	virtual void on_tissuenr_changed(int i) {}
	virtual void on_slicenr_changed() {}

	virtual void on_mouse_clicked(Point p) {}
	virtual void on_mouse_released(Point p) {}
	virtual void on_mouse_moved(Point p) {}

public:
	QCursor* m_cursor;


signals:
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

private slots:
	void tissuenr_changed(int i) { on_tissuenr_changed(i); }
	//TODOvoid slicenr_changed() { on_slicenr_changed(); }

	void mouse_clicked(Point p) { on_mouse_clicked(p); }
	void mouse_released(Point p) { on_mouse_released(p); }
	void mouse_moved(Point p) { on_mouse_moved(p); }

public slots:
	virtual void slicenr_changed(){} // BL TODO

protected:
	static bool hideparams;
};

} // namespace iseg
