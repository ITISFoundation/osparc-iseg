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

#include "iSegInterface.h"

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
	explicit Spoiler(const QString& title = "", const int animationDuration = 300, QWidget* parent = nullptr);
	void SetContentLayout(QLayout* contentLayout);

private slots:
	void OnCollapse(bool checked);

private:
	QGridLayout m_MainLayout;
	QToolButton m_ToggleButton;
	QFrame m_HeaderLine;
	QParallelAnimationGroup m_ToggleAnimation;
	QScrollArea m_ContentArea;
	int m_AnimationDuration{300};
};