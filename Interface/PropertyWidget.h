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

class ItemDelegate;

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyWidget : public QTreeWidget
{
	Q_OBJECT
public:
	PropertyWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetProperty(Property_ptr p);

signals:
	void OnPropertyEdited(Property_ptr);

private slots:
	void Edited();

	void drawRow2(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;// override;

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
	std::shared_ptr<char> m_Lifespan;
};


} // namespace iseg
