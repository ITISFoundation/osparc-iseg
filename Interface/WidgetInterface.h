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

#include "InterfaceApi.h"

#include "Data/DataSelection.h"
#include "Data/Mark.h"

#include <qcursor.h>
#include <qdir.h>
#include <qicon.h>
#include <qwidget.h>

namespace iseg {

class ISEG_INTERFACE_API WidgetInterface : public QWidget
{
	Q_OBJECT
public:
	WidgetInterface(QWidget* parent, const char* name, Qt::WindowFlags wFlags);
	virtual void init() {}
	virtual void newloaded() {}
	virtual void cleanup() {}
	virtual FILE* SaveParams(FILE* fp, int version) { return fp; };
	virtual FILE* LoadParams(FILE* fp, int version) { return fp; };
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

	QCursor* get_cursor() { return m_cursor; }

signals:
	void vm_changed(std::vector<Mark>* vm1);
	void vpdyn_changed(std::vector<Point>* vpdyn_arg);

	void begin_datachange(iseg::DataSelection& dataSelection,
			QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
			iseg::EndUndoAction undoAction = iseg::EndUndo);

private slots:
	void tissuenr_changed(int i) { on_tissuenr_changed(i); }
	void slicenr_changed() { on_slicenr_changed(); }

	// \todo duplicate slots (and connections) and pass argument source_or_target to callbacks
	void mouse_clicked(Point p) { on_mouse_clicked(p); }
	void mouse_released(Point p) { on_mouse_released(p); }
	void mouse_moved(Point p) { on_mouse_moved(p); }

protected:
	QCursor* m_cursor;

	static bool hideparams;
};

} // namespace iseg
