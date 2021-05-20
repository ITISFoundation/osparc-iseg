/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "InterfaceApi.h"

#include "FormatTooltip.h"

#include "Data/DataSelection.h"
#include "Data/Mark.h"

#include <QDir>
#include <QIcon>
#include <QWidget>

class QCursor;

namespace iseg {

class ISEG_INTERFACE_API WidgetInterface : public QWidget
{
	Q_OBJECT
public:
	WidgetInterface(QWidget* parent, const char* name, Qt::WindowFlags wFlags);
	virtual void Init() {}
	virtual void NewLoaded() {}
	virtual void Cleanup() {}
	virtual FILE* SaveParams(FILE* fp, int version) { return fp; };
	virtual FILE* LoadParams(FILE* fp, int version) { return fp; };
	virtual void HideParamsChanged() {}
	static void SetHideParams(bool hide) { hideparams = hide; }
	static bool GetHideParams() { return hideparams; }
	virtual std::string GetName() { return ""; }
	virtual QIcon GetIcon(QDir picdir) = 0;

	virtual void OnTissuenrChanged(int i) {}
	virtual void OnSlicenrChanged() {}

	QCursor* GetCursor() { return m_MCursor; }

protected:
	virtual void OnMouseClicked(Point p) {}
	virtual void OnMouseReleased(Point p) {}
	virtual void OnMouseMoved(Point p) {}

signals:
	void VmChanged(std::vector<Mark>* vm1);
	void VpdynChanged(std::vector<Point>* vpdyn_arg);

	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);

public slots:
	virtual void BmpChanged() {}
	virtual void WorkChanged() {}
	virtual void TissuesChanged() {}
	virtual void MarksChanged() {}

	void TissuenrChanged(int i) { OnTissuenrChanged(i); }
	void SlicenrChanged() { OnSlicenrChanged(); }

	// \todo duplicate slots (and connections) and pass argument source_or_target to callbacks
	void MouseClicked(Point p) { OnMouseClicked(p); }
	void MouseReleased(Point p) { OnMouseReleased(p); }
	void MouseMoved(Point p) { OnMouseMoved(p); }

protected:
	QCursor* m_MCursor;

	static bool hideparams;
};

} // namespace iseg
