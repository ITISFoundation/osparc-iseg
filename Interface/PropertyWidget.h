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

#include "../Data/SharedPtr.h"

#include <QWidget>

#include <map>
#include <memory>

class QBoxLayout;

namespace iseg {

ISEG_DECL_CLASS_PTR(Property);

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyWidget : public QWidget
{
	Q_OBJECT
public:
	PropertyWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetProperty(Property_ptr p);

signals:
	void OnPropertyEdited(iseg::Property_ptr);

private slots:
	void ToggleCollapsable(bool checked);
	void Edited();

private:
	void Build(Property_ptr p, QBoxLayout* layout);
	QWidget* MakePropertyUi(Property& prop);

	void UpdateState(QWidget* w, Property_cptr p);
	void UpdateDescription(QWidget* w, Property_cptr p);

	Property_ptr m_Property;
	std::map<QWidget*, Property_wptr> m_WidgetPropertyMap;
	std::map<QWidget*, QBoxLayout*> m_CollapseButtonLayoutMap;
};

class QSharedPtrHolder : public QObject
{
	Q_OBJECT
public:
	QSharedPtrHolder(QObject* parent) : QObject(parent), m_LifeSpan(new char) {}
	~QSharedPtrHolder() override = default;

	/// Get weak ptr to check if object is alive
	std::weak_ptr<char> LifeSpan() { return m_LifeSpan; }

private:
	std::shared_ptr<char> m_LifeSpan;
};

} // namespace iseg
