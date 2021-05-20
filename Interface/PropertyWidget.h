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

#include <QWidget>

#include <map>
#include <memory>

class QBoxLayout;

namespace iseg {

class Property;
using Property_ptr = std::shared_ptr<Property>;
using Property_wptr = std::weak_ptr<Property>;

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyWidget : public QWidget
{
	Q_OBJECT
public:
	PropertyWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void setProperty(Property_ptr p);
	Property_ptr property() { return m_Property; }

signals:
	void OnPropertyEdited(iseg::Property_ptr);

private slots:
	void ToggleCollapsable(bool checked);
	void Edited();

private:
	void Build(Property_ptr p, QBoxLayout* layout);
	QWidget* MakePropertyUi(Property& prop);

	Property_ptr m_Property;
	std::map<QWidget*, Property_wptr> m_WidgetPropertyMap;
	std::map<QWidget*, QBoxLayout*> m_CollapseButtonLayoutMap;
};

} // namespace iseg
