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
class QComboBox;

namespace iseg {

class ItemDelegate;

/* \brief UI class to display and edit properties
**/
class ISEG_INTERFACE_API PropertyWidget : public QTreeWidget
{
	Q_OBJECT
public:
	PropertyWidget(Property_ptr p = nullptr, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetProperty(Property_ptr p);

	QSize minimumSizeHint() const override;

signals:
	void OnPropertyEdited(Property_ptr);

private slots:
	void Edited();
	void SliderPressed();
	void SliderMoved();
	void SliderReleased();
	void SliderRangeEdited();

private:
	void Build(Property_ptr p, QTreeWidgetItem* item, QTreeWidgetItem* parent_item);
	QWidget* MakePropertyUi(Property* prop, QTreeWidgetItem* item);

	void UpdateState(QTreeWidgetItem* item, const Property* p);
	void UpdateDescription(QWidget* w, const Property* p);

	void UpdateComboState(QComboBox* combo, const PropertyEnum* ptyped);
	void UpdateComboToolTips(QComboBox* combo, const PropertyEnum* ptyped);

	template<typename TFunctor>
	void VisitItems(QTreeWidgetItem* item, const TFunctor& functor);

	template<typename TFunctor, typename TCombiner>
	void VisitItems(QTreeWidgetItem* item, const TFunctor& functor, const TCombiner& combine, bool value);

	Property::ePropertyType ItemType(const QTreeWidgetItem* item) const;

	ItemDelegate* m_ItemDelegate;
	Property_ptr m_Property;
	std::map<const QTreeWidgetItem*, Property_wptr> m_ItemPropertyMap;
	std::map<QWidget*, Property_wptr> m_WidgetPropertyMap;
	std::shared_ptr<char> m_Lifespan;
};

} // namespace iseg
