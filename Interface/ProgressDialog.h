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

#include "../Data/ProgressInfo.h"

#include <qobject.h>

#include <atomic>

class QWidget;
class QProgressDialog;

namespace iseg {

class ISEG_INTERFACE_API ProgressDialog
		: public QObject
		, public ProgressInfo
{
	Q_OBJECT
public:
	ProgressDialog(const char* msg, QWidget* parent);

	void SetNumberOfSteps(int N) override;
	void Increment() override;
	bool WasCanceled() const override;
	void SetValue(int percent) override;

private slots:
	void Cancel();

private:
	bool m_Canceled = false;
	QProgressDialog* m_Progress = nullptr;
	std::atomic<int> m_Count;
};

} // namespace iseg
