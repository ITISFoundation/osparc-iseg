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

#include "../Data/Property.h"

#include <QTreeWidget>
#include <QWidget>

#include <map>
#include <memory>

class QBoxLayout;
class QFormLayout;

namespace iseg {

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyWidget : public QWidget
{
	Q_OBJECT
public:
	PropertyWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetProperty(Property_ptr p);

	void SetPreferredWidth(int width) { m_PreferredWidth = width; }

	QSize sizeHint() const override;

signals:
	void OnPropertyEdited(Property_ptr);

private slots:
	void ToggleCollapsable(bool checked);
	void Edited();

private:
	void Build(Property_ptr p, QBoxLayout* layout);
	QWidget* MakePropertyUi(Property& prop, QWidget* container);

	void UpdateState(QWidget* w, Property_cptr p);
	void UpdateDescription(QWidget* w, Property_cptr p);

	Property_ptr m_Property;
	std::map<QWidget*, Property_wptr> m_WidgetPropertyMap;
	std::map<QWidget*, QWidget*> m_CollapseButtonAreaMap;
	int m_PreferredWidth = 100;
};

class ItemDelegate;

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	PropertyTreeWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetProperty(Property_ptr p);

signals:
	void OnPropertyEdited(Property_ptr);

private slots:
	void Edited();

	void drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
	void Build(Property_ptr p, QTreeWidgetItem* item, QTreeWidgetItem* parent_item);
	QWidget* MakePropertyUi(Property& prop, QTreeWidgetItem* item);

	void UpdateState(QTreeWidgetItem* item, Property_cptr p);
	void UpdateDescription(QWidget* w, Property_cptr p);

	int RowFromIndex(const QModelIndex& index) const;

	template<typename TFunctor>
	void VisitLeaves(QTreeWidgetItem* item, const TFunctor& functor);

	Property::ePropertyType ItemType(const QTreeWidgetItem* item) const;

	ItemDelegate* m_ItemDelegate;
	Property_ptr m_Property;
	std::map<const QTreeWidgetItem*, Property::ePropertyType> m_ItemTypeMap;
	std::map<QWidget*, Property_wptr> m_WidgetPropertyMap;
};

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyFormWidget : public QWidget
{
	Q_OBJECT
public:
	PropertyFormWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetProperty(Property_ptr p);

signals:
	void OnPropertyEdited(Property_ptr);

private slots:
	void Edited();

private:
	int Build(Property_ptr p, int row);
	QWidget* MakePropertyUi(Property& prop, int row);

	void UpdateState(int row, Property_cptr p);
	void UpdateDescription(QWidget* w, Property_cptr p);

	QFormLayout* m_FormLayout;
	Property_ptr m_Property;
	std::map<QWidget*, Property_wptr> m_WidgetPropertyMap;
};

/* \brief Create a std::shared_ptr/std::weak_ptr with the same lifespan as 'object'
**/
class QSharedPtrHolder : public QObject
{
	Q_OBJECT
public:
	QSharedPtrHolder(QObject* object) : QObject(object), m_Lifespan(new char) {}
	~QSharedPtrHolder() override = default;

	static std::weak_ptr<char> WeakPtr(QObject* object);

private:
	std::shared_ptr<char> m_Lifespan;
};

} // namespace iseg
