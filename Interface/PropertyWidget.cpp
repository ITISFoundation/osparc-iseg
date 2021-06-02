#include "PropertyWidget.h"

#include "CollapsibleWidget.h"
#include "FormatTooltip.h"
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
		auto widget = MakePropertyUi(*prop, item);
		//widget->setContentsMargins(5, 2, 2, 2);
		widget->setMaximumHeight(row_height);

		auto hbox = new QHBoxLayout;
		hbox->addWidget(widget);
		auto harea = new QWidget;
		harea->setLayout(hbox);
		hbox->setContentsMargins(5, 2, 2, 2);

		if (prop->Type() == Property::kButton)
		{
			setItemWidget(item, 0, harea);
		}
		else
		{
			item->setText(0, QString::fromStdString(name));
			setItemWidget(item, 1, harea);
		}

		m_WidgetPropertyMap[widget] = prop;
	}
	else
	{
		item->setText(0, QString::fromStdString(name));

		Connect(prop->onModified, m_Lifespan, [this, item](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
			}
		});

		for (const auto& p : prop->Properties())
		{
			Build(p, new QTreeWidgetItem, item);
		}
	}
}

QWidget* PropertyWidget::MakePropertyUi(Property& prop, QTreeWidgetItem* item)
{
	const auto make_line_edit = [this, item](const Property& p) {
		// generic attributes
		auto edit = new QLineEdit(this);
		edit->setFrame(false);
		edit->setToolTip(QString::fromStdString(p.ToolTip()));
		item->setDisabled(!p.Enabled());
		item->setHidden(!p.Visible());
		QObject_connect(edit, SIGNAL(editingFinished()), this, SLOT(Edited()));
		return edit;
	};

	const auto label_text = prop.Description().empty() ? prop.Name() : prop.Description();

	switch (prop.Type())
	{
	case Property::kInteger: {
		auto ptyped = static_cast<const PropertyInt*>(&prop);
		auto edit = make_line_edit(prop);
		edit->setText(QString::number(ptyped->Value())); // ? should this be set via Property::StringValue?
		edit->setValidator(new QIntValidator(ptyped->Minimum(), ptyped->Maximum()));

		Connect(prop.onModified, m_Lifespan, [edit, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyInt*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(edit, p);
			}
		});
		return edit;
	}
	case Property::kReal: {
		auto ptyped = static_cast<const PropertyReal*>(&prop);
		auto edit = make_line_edit(prop);
		edit->setText(QString::number(ptyped->Value())); // ? should this be set via Property::StringValue?
		edit->setValidator(new QDoubleValidator(ptyped->Minimum(), ptyped->Maximum(), 10));

		Connect(prop.onModified, m_Lifespan, [edit, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyReal*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(edit, p);
			}
		});
		return edit;
	}
	case Property::kString: {
		auto ptyped = static_cast<const PropertyString*>(&prop);
		auto edit = make_line_edit(prop);
		edit->setText(QString::fromStdString(ptyped->Value()));

		Connect(prop.onModified, m_Lifespan, [edit, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyString*>(p.get());
				edit->setText(QString::fromStdString(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(edit, p);
			}
		});
		return edit;
	}
	case Property::kBool: {
		auto ptyped = static_cast<const PropertyBool*>(&prop);
		auto checkbox = new QCheckBox(this);
		checkbox->setChecked(ptyped->Value());
		UpdateState(item, prop.shared_from_this());
		QObject_connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(Edited()));

		Connect(prop.onModified, m_Lifespan, [checkbox, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyBool*>(p.get());
				checkbox->setChecked(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(checkbox, p);
			}
		});
		return checkbox;
	}
	case Property::kEnum: {
		auto ptyped = static_cast<const PropertyEnum*>(&prop);
		auto combo = new QComboBox(this);
		for (auto d : ptyped->Values())
		{
			combo->insertItem(d.first, QString::fromStdString(d.second));
		}
		combo->setCurrentIndex(ptyped->Value());
		UpdateState(item, prop.shared_from_this());
		UpdateComboState(combo, ptyped);
		UpdateComboToolTips(combo, ptyped);

		QObject_connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(Edited()));

		Connect(prop.onModified, m_Lifespan, [combo, item, ptyped, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyEnum*>(p.get());
				combo->setCurrentIndex(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
				UpdateComboState(combo, ptyped);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(combo, p);
				UpdateComboToolTips(combo, ptyped);
			}
		});
		return combo;
	}
	case Property::kButton: {
		auto p = dynamic_cast<PropertyButton*>(&prop);
		const auto button_text = p->ButtonText().empty() ? prop.Name() : p->ButtonText();
		auto button = new QPushButton(QString::fromStdString(button_text), this);
		button->setAutoDefault(false);
		UpdateState(item, prop.shared_from_this());
		QObject_connect(button, SIGNAL(released()), this, SLOT(Edited()));

		Connect(prop.onModified, m_Lifespan, [button, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(button, p);
			}
		});
		return button;
	}
	default:
		assert(false && "Unexpected property type");
		return new QWidget; // \todo handle PropertyGroup here?
	}
}

void PropertyWidget::UpdateState(QTreeWidgetItem* item, Property_cptr p)
{
	const auto set_state = [this, visible=p->Visible(), enabled=p->Enabled()](QTreeWidgetItem* item) 
	{
		// TODO BL: BUG!! if parent is visible, this will make child visible, even if its property is NOT
		item->setDisabled(!enabled);
		item->setHidden(!visible);

		for (int col = 0; col < 2; ++col)
		{
			if (auto w = itemWidget(item, col))
			{
				w->setEnabled(enabled);
			}
		}
	};

	VisitItems(item, set_state);
}

void PropertyWidget::UpdateDescription(QWidget* w, Property_cptr p)
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
			default:
				break;
			}
		}
	}
}

} // namespace iseg