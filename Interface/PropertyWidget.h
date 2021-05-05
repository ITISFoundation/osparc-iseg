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

#include <memory>
#include <map>

class QBoxLayout;

namespace iseg {

class Property;

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyWidget : public QWidget
{
	Q_OBJECT
public:
	using Property_ptr = std::shared_ptr<Property>;
	using Property_wptr = std::weak_ptr<Property>;

	PropertyWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void setProperty(Property_ptr p);
	Property_ptr property() { return _property; }

signals:
	void onPropertyEdited(Property_ptr);

private slots:
	void toggleCollapsable(bool checked);
	void edited();

private:
	void build(Property_ptr p, QBoxLayout* layout);
	QWidget* makePropertyUI(Property& prop);

	Property_ptr _property;
	std::map<QWidget*, Property_wptr> _widgetPropertyMap;
	std::map<QWidget*, QBoxLayout*> _collapseButtonLayoutMap;
};

} // namespace iseg
