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

	void setNumberOfSteps(int N) override;
	void increment() override;
	bool wasCanceled() const override;
	void setValue(int percent) override;

private slots:
	void cancel();

private:
	bool canceled = false;
	QProgressDialog* progress = nullptr;
	std::atomic<int> count;
};

} // namespace iseg
