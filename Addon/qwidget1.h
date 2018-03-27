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

#include <qcursor.h>
#include <qdir.h>
#include <qicon.h>
#include <qwidget.h>

#include "AddonApi.h"

#define UNREFERENCED_PARAMETER(P) (P)

namespace iseg {

class ADDON_API QWidget1 : public QWidget
{
	Q_OBJECT
public:
	QWidget1(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
	{
		m_cursor = new QCursor(Qt::CrossCursor);
	};
	virtual void init() {}
	virtual void newloaded() {}
	QCursor* m_cursor;
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

protected:
	static bool hideparams;

public slots:
	virtual void slicenr_changed(){};
};

} // namespace iseg
