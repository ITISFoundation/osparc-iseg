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

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

class Spoiler : public QWidget
{
	Q_OBJECT
public:
	explicit Spoiler(const QString& title = "", const int animationDuration = 300, QWidget* parent = 0);
	void setContentLayout(QLayout* contentLayout);

private slots:
	void onCollapse(const bool checked);

private:
	QGridLayout mainLayout;
	QToolButton toggleButton;
	QFrame headerLine;
	QParallelAnimationGroup toggleAnimation;
	QScrollArea contentArea;
	int animationDuration{300};
};