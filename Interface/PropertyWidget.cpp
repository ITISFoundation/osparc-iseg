#include "PropertyWidget.h"

#include "CollapsibleWidget.h"
#include "FormatTooltip.h"
#include "QSliderEditableRange.h"
#include "SplitterHandle.h"

#include "../Data/Logger.h"
#include "../Data/Property.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QIntValidator>
#include <QItemDelegate>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSignalMapper>
#include <QStandardItemModel>
#include <QTreeWidget>

#include <iostream>

namespace iseg {

static const int row_height = 20;

namespace {
template<typename T>
struct Type
{
};

double toType(QString const& q, Type<double>)
{
	return q.toDouble();
}

int toType(QString const& q, Type<int>)
{
	return q.toInt();
}

std::string toType(QString const& q, Type<std::string>)
{
	return q.toStdString();
}

template<typename T>
auto toType(QString const& q)
		-> decltype(toType(q, Type<T>{}))
{
	return toType(q, Type<T>{});
}

int CountProps(Property_cptr parent)
{
	int count = 1;

	for (const auto& child : parent->Properties())
	{
		count += CountProps(child);
	}
	return count;
}

} // namespace

class ItemDelegate : public QItemDelegate
{
private:
	int m_iHeight;

public:
	ItemDelegate(QObject* poParent = nullptr, int iHeight = -1) : QItemDelegate(poParent), m_iHeight(iHeight)
	{
	}

	void SetHeight(int iHeight)
	{
		m_iHeight = iHeight;
	}

	// Use this for setting tree item height.
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QSize sz = QItemDelegate::sizeHint(option, index);

		if (m_iHeight != -1)
		{
			// Set tree item height.
			sz.setHeight(m_iHeight);
		}

		return sz;
	}
};

PropertyWidget::PropertyWidget(Property_ptr prop, QWidget* parent, Qt::WindowFlags wFlags)
		: QTreeWidget(parent), m_ItemDelegate(new ItemDelegate), m_Lifespan(std::make_shared<char>('1'))
{
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::NoSelection);

	setColumnCount(2);
	setAcceptDrops(false);
	setDragDropMode(QTreeWidget::NoDragDrop);
	setExpandsOnDoubleClick(false);
	setHeaderHidden(true);
	setFrameStyle(QFrame::Plain);
	setUniformRowHeights(true);
	setAnimated(true);
	setIndentation(10);

	m_ItemDelegate->SetHeight(row_height + 4);
	setItemDelegate(m_ItemDelegate);

	// move column spitter interactively
	new SplitterHandle(this);

	// setup the headers
	QStringList header_list;
	header_list.append("Property");
	header_list.append("Value");

	setHeaderLabels(header_list);
	header()->setMinimumSectionSize(100);
	header()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
	header()->setSectionsMovable(false);
#endif

	SetProperty(prop);
	if (auto root = topLevelItem(0))
	{
		UpdateState(root, prop.get());
	}

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

QSize PropertyWidget::minimumSizeHint() const
{
	auto sz = QTreeWidget::minimumSizeHint();
	sz.setHeight(CountProps(m_Property) * (row_height+2));
	return sz;
}

Property::ePropertyType PropertyWidget::ItemType(const QTreeWidgetItem* item) const
{
	auto found = m_ItemPropertyMap.find(item);
	if (found != m_ItemPropertyMap.end())
	{
		if (auto p = found->second.lock())
		{
			return p->Type();
		}
	}
	return Property::kePropertyTypeSize;
}

void PropertyWidget::SetProperty(Property_ptr prop)
{
	if (prop)
	{
		m_Property = prop;
		m_WidgetPropertyMap.clear();
		m_ItemPropertyMap.clear();

		clear();

		Build(prop, new QTreeWidgetItem, nullptr);

		const auto set_span = [this](QTreeWidgetItem* item) {
			if (ItemType(item) == Property::kButton)
			{
				item->setFirstColumnSpanned(true);
			}
		};

		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			auto item = topLevelItem(i);

			VisitItems(item, set_span);
		}
	}
}

template<typename TFunctor>
void PropertyWidget::VisitItems(QTreeWidgetItem* item, const TFunctor& functor)
{
	functor(item);

	for (int i = 0; i < item->childCount(); ++i)
	{
		auto child = item->child(i);
		VisitItems(child, functor);
	}
}

template<typename TFunctor, typename TCombiner>
void PropertyWidget::VisitItems(QTreeWidgetItem* item, const TFunctor& functor, const TCombiner& combine, bool value)
{
	functor(item, value);

	for (int i = 0; i < item->childCount(); ++i)
	{
		auto child = item->child(i);
		VisitItems(child, functor, combine, combine(child, value));
	}
}

void PropertyWidget::Build(Property_ptr prop, QTreeWidgetItem* item, QTreeWidgetItem* parent_item)
{
	if (parent_item)
	{
		parent_item->addChild(item);
	}
	else
	{
		addTopLevelItem(item);
	}

	const std::string name = prop->Description().empty() ? prop->Name() : prop->Description();
	item->setFlags(Qt::ItemIsEnabled);
	item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
	expandItem(item);

	m_ItemPropertyMap[item] = prop;

	if (prop->Type() != Property::kGroup)
	{
		// create widget
		auto widget = MakePropertyUi(prop.get(), item);

		// use uniform row size
		widget->setMaximumHeight(row_height);
		auto hbox = new QHBoxLayout;
		hbox->addWidget(widget);
		auto harea = new QWidget;
		harea->setLayout(hbox);
		hbox->setContentsMargins(5, 2, 2, 2);

		// add widget to item
		if (prop->Type() == Property::kButton)
		{
			setItemWidget(item, 0, harea);
		}
		else
		{
			item->setText(0, QString::fromStdString(name));
			setItemWidget(item, 1, harea);
		}

		// set state and tooltips
		UpdateDescription(widget, prop.get());

		// need to map widget to property for 'Edited'
		m_WidgetPropertyMap[widget] = prop;
	}
	else
	{
		item->setText(0, QString::fromStdString(name));
		item->setToolTip(0, QString::fromStdString(prop->ToolTip()));

		Connect(prop->onModified, m_Lifespan, [this, item](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
		});

		for (const auto& p : prop->Properties())
		{
			Build(p, new QTreeWidgetItem, item);
		}
	}
}

QWidget* PropertyWidget::MakePropertyUi(Property* prop, QTreeWidgetItem* item)
{
	const auto make_line_edit = [this](const Property* p) {
		// generic attributes
		auto edit = new QLineEdit(this);
		edit->setFrame(false);
		QObject_connect(edit, SIGNAL(editingFinished()), this, SLOT(Edited()));
		return edit;
	};

	static_assert(Property::kePropertyTypeSize == 8, "Property types changed");
	switch (prop->Type())
	{
	case Property::kInteger: {
		auto ptyped = static_cast<const PropertyInt*>(prop);
		auto edit = make_line_edit(prop);
		edit->setText(QString::number(ptyped->Value())); // ? should this be set via Property::StringValue?
		edit->setValidator(new QIntValidator(ptyped->Minimum(), ptyped->Maximum()));

		Connect(prop->onModified, m_Lifespan, [edit, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyInt*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kValueRangeChanged)
			{
				auto ptyped = static_cast<const PropertyInt*>(p.get());
				edit->setValidator(new QIntValidator(ptyped->Minimum(), ptyped->Maximum()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(edit, p.get());
			}
		});
		return edit;
	}
	case Property::kReal: {
		auto ptyped = static_cast<const PropertyReal*>(prop);
		auto edit = make_line_edit(prop);
		edit->setText(QString::number(ptyped->Value())); // ? should this be set via Property::StringValue?
		edit->setValidator(new QDoubleValidator(ptyped->Minimum(), ptyped->Maximum(), 10));

		Connect(prop->onModified, m_Lifespan, [edit, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyReal*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kValueRangeChanged)
			{
				auto ptyped = static_cast<const PropertyReal*>(p.get());
				edit->setValidator(new QDoubleValidator(ptyped->Minimum(), ptyped->Maximum(), 10));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(edit, p.get());
			}
		});
		return edit;
	}
	case Property::kString: {
		auto ptyped = static_cast<const PropertyString*>(prop);
		auto edit = make_line_edit(prop);
		edit->setText(QString::fromStdString(ptyped->Value()));

		Connect(prop->onModified, m_Lifespan, [edit, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyString*>(p.get());
				edit->setText(QString::fromStdString(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(edit, p.get());
			}
		});
		return edit;
	}
	case Property::kBool: {
		auto ptyped = static_cast<const PropertyBool*>(prop);
		auto checkbox = new QCheckBox(this);
		checkbox->setChecked(ptyped->Value());
		QObject_connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(Edited()));

		Connect(prop->onModified, m_Lifespan, [checkbox, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyBool*>(p.get());
				checkbox->setChecked(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(checkbox, p.get());
			}
		});
		return checkbox;
	}
	case Property::kEnum: {
		auto ptyped = static_cast<const PropertyEnum*>(prop);
		auto combo = new QComboBox(this);
		for (auto d : ptyped->Values())
		{
			combo->insertItem(d.first, QString::fromStdString(d.second));
		}
		combo->setCurrentIndex(ptyped->Value());
		UpdateComboState(combo, ptyped);
		UpdateComboToolTips(combo, ptyped);

		QObject_connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(Edited()));

		Connect(prop->onModified, m_Lifespan, [combo, item, ptyped, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyEnum*>(p.get());
				combo->setCurrentIndex(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, ptyped);
				UpdateComboState(combo, ptyped);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(combo, ptyped);
				UpdateComboToolTips(combo, ptyped);
			}
		});
		return combo;
	}
	case Property::kButton: {
		auto p = dynamic_cast<PropertyButton*>(prop);
		const auto button_text = p->ButtonText().empty() ? p->Name() : p->ButtonText();
		auto button = new QPushButton(QString::fromStdString(button_text), this);
		button->setAutoDefault(false);
		QObject_connect(button, SIGNAL(released()), this, SLOT(Edited()));

		Connect(prop->onModified, m_Lifespan, [button, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(button, p.get());
			}
		});
		return button;
	}
	case Property::kSlider: {
		auto ptyped = static_cast<const PropertySlider*>(prop);
		auto slider = new QSliderEditableRange;
		slider->setValue(ptyped->Value());
		slider->setRange(ptyped->Minimum(), ptyped->Maximum());
		slider->setMinimumVisible(ptyped->EditMinimum());
		slider->setMaximumVisible(ptyped->EditMaximum());

		QObject_connect(slider, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
		QObject_connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(SliderMoved()));
		QObject_connect(slider, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
		QObject_connect(slider, SIGNAL(rangeChanged(int,int)), this, SLOT(SliderRangeEdited()));

		Connect(prop->onModified, m_Lifespan, [slider, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertySlider*>(p.get());
				slider->setValue(ptyped->Value());
			}
			else if (type == Property::eChangeType::kValueRangeChanged)
			{
				auto ptyped = static_cast<const PropertySlider*>(p.get());
				slider->setRange(ptyped->Minimum(), ptyped->Maximum());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p.get());
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(slider, p.get());
			}
		});
		return slider;
	}
	default:
		assert(false && "Unexpected property type");
		return new QWidget; // \todo handle PropertyGroup here?
	}
}

void PropertyWidget::UpdateState(QTreeWidgetItem* item, const Property* prop)
{
	const auto set_enabled = [this](QTreeWidgetItem* item, bool enabled) 
	{
		item->setDisabled(!enabled);

		for (int col = 0; col < 2; ++col)
		{
			if (auto w = itemWidget(item, col))
			{
				w->setEnabled(enabled);
			}
		}
	};

	const auto combine_enabled = [this](QTreeWidgetItem* item, bool v) {
		if (auto p = m_ItemPropertyMap[item].lock())
		{
			return v && p->Enabled();
		}
		return v;
	};

	VisitItems(item, set_enabled, combine_enabled, prop->NetEnabled());

	const auto set_visible = [](QTreeWidgetItem* item, bool visible) {
		item->setHidden(!visible);
	};

	const auto combine_visible = [this](QTreeWidgetItem* item, bool v) {
		if (auto p = m_ItemPropertyMap[item].lock())
		{
			return v && p->Visible();
		}
		return v;
	};

	VisitItems(item, set_visible, combine_visible, prop->NetVisible());
}

void PropertyWidget::UpdateDescription(QWidget* w, const Property* p)
{
	// description/name ?
	w->setToolTip(Format(p->ToolTip()));
}

void PropertyWidget::UpdateComboState(QComboBox* combo, const PropertyEnum* ptyped)
{
	auto model = qobject_cast<QStandardItemModel*>(combo->model());
	if (ptyped->HasEnabledFlags() && model)
	{
		for (const auto& i : ptyped->Values())
		{
			if (auto combo_item = model->item(i.first))
			{
				combo_item->setEnabled(ptyped->Enabled(i.first));
			}
		}
	}
}

void PropertyWidget::UpdateComboToolTips(QComboBox* combo, const PropertyEnum* ptyped)
{
	auto model = qobject_cast<QStandardItemModel*>(combo->model());
	if (ptyped->HasItemToolTips())
	{
		for (const auto& i : ptyped->Values())
		{
			if (auto combo_item = model->item(i.first))
			{
				combo_item->setToolTip(QString::fromStdString(ptyped->ItemToolTip(i.first)));
			}
		}
	}
}

void PropertyWidget::Edited()
{
	if (auto w = dynamic_cast<QWidget*>(QObject::sender()))
	{
		if (auto prop = std::dynamic_pointer_cast<Property>(m_WidgetPropertyMap[w].lock()))
		{
			static_assert(Property::kePropertyTypeSize == 8, "Property types changed");
			switch (prop->Type())
			{
			case Property::kInteger: {
				if (auto edit = dynamic_cast<QLineEdit*>(w))
				{
					auto prop_typed = std::static_pointer_cast<PropertyInt>(prop);
					auto v = toType<int>(edit->text());
					if (v != prop_typed->Value())
					{
						prop_typed->SetValue(v);
						emit OnPropertyEdited(prop);
					}
				}
				break;
			}
			case Property::kReal: {
				if (auto edit = dynamic_cast<QLineEdit*>(w))
				{
					auto prop_typed = std::static_pointer_cast<PropertyReal>(prop);
					auto v = toType<double>(edit->text());
					if (v != prop_typed->Value())
					{
						prop_typed->SetValue(v);
						emit OnPropertyEdited(prop);
					}
				}
				break;
			}
			case Property::kString: {
				if (auto edit = dynamic_cast<QLineEdit*>(w))
				{
					auto prop_typed = std::static_pointer_cast<PropertyString>(prop);
					auto v = toType<std::string>(edit->text());
					if (v != prop_typed->Value())
					{
						prop_typed->SetValue(v);
						emit OnPropertyEdited(prop);
					}
				}
				break;
			}
			case Property::kBool: {
				if (auto cb = dynamic_cast<QCheckBox*>(w))
				{
					auto prop_typed = std::static_pointer_cast<PropertyBool>(prop);
					auto v = cb->isChecked();
					if (v != prop_typed->Value())
					{
						prop_typed->SetValue(v);
						emit OnPropertyEdited(prop);
					}
				}
				break;
			}
			case Property::kEnum: {
				if (auto cb = dynamic_cast<QComboBox*>(w))
				{
					auto prop_typed = std::static_pointer_cast<PropertyEnum>(prop);
					auto v = cb->currentIndex();
					if (v != prop_typed->Value())
					{
						prop_typed->SetValue(v);
						emit OnPropertyEdited(prop);
					}
				}
				break;
			}
			case Property::kButton: {
				auto prop_typed = std::static_pointer_cast<PropertyButton>(prop);
				if (prop_typed->Value())
				{
					prop_typed->Value()();
				}
				break;
			}
			case Property::kSlider: {
				if (auto slider = dynamic_cast<QSliderEditableRange*>(w))
				{
					auto prop_typed = std::static_pointer_cast<PropertySlider>(prop);
					auto v = slider->value();
					if (v != prop_typed->Value())
					{
						prop_typed->SetValue(v);
						emit OnPropertyEdited(prop);
					}
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

void PropertyWidget::SliderPressed()
{
	if (auto w = dynamic_cast<QSliderEditableRange*>(QObject::sender()))
	{
		if (auto prop = std::dynamic_pointer_cast<PropertySlider>(m_WidgetPropertyMap[w].lock()))
		{
			prop->onPressed(w->value());
		}
	}
}

void PropertyWidget::SliderMoved()
{
	if (auto w = dynamic_cast<QSliderEditableRange*>(QObject::sender()))
	{
		if (auto prop = std::dynamic_pointer_cast<PropertySlider>(m_WidgetPropertyMap[w].lock()))
		{
			prop->onMoved(w->value());
		}
	}
}

void PropertyWidget::SliderReleased()
{
	if (auto w = dynamic_cast<QSliderEditableRange*>(QObject::sender()))
	{
		if (auto prop = std::dynamic_pointer_cast<PropertySlider>(m_WidgetPropertyMap[w].lock()))
		{
			prop->SetValue(w->value());
			prop->onReleased(w->value());
			emit OnPropertyEdited(prop);
		}
	}
}

void PropertyWidget::SliderRangeEdited()
{
	if (auto w = dynamic_cast<QSliderEditableRange*>(QObject::sender()))
	{
		if (auto prop = std::dynamic_pointer_cast<PropertySlider>(m_WidgetPropertyMap[w].lock()))
		{
			prop->SetMinimum(w->minimum());
			prop->SetMaximum(w->maximum());
			emit OnPropertyEdited(prop);
		}
	}
}

} // namespace iseg