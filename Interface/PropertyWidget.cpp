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
#include <QFormLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QItemDelegate>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QStandardItemModel>
#include <QTreeWidget>

#include <iostream>

namespace iseg {

static const int child_indent = 8;
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
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
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

PropertyWidget::PropertyWidget(Property_ptr prop, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QTreeWidget(parent), m_Lifespan(std::make_shared<char>('1'))
{
	m_ItemDelegate = new ItemDelegate;
	m_ItemDelegate->SetHeight(row_height+4);

	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode( QAbstractItemView::NoSelection );
	//setSelectionMode(QAbstractItemView::ExtendedSelection);

	setColumnCount(2);
	//hideColumn(2); // for property type
	setAcceptDrops(false);
	setDragDropMode(QTreeWidget::NoDragDrop);
	setExpandsOnDoubleClick(false);
	setHeaderHidden(true);
	setFrameStyle(QFrame::Plain);
	setUniformRowHeights(true);
	setAnimated(true);
	setIndentation(10);

	auto splitter = new SplitterHandle(this);

	//setAlternatingRowColors(true);
	//setStyleSheet("alternate-background-color: yellow;background-color: red;");

	setItemDelegate(m_ItemDelegate);

	// setup the headers
	QStringList header_list;
	header_list.append("Property");
	header_list.append("Value");

	setHeaderLabels(header_list);
	setColumnWidth(0, 150);
	setColumnWidth(1, 150);

	header()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
	header()->setSectionsMovable(false);
#endif

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	//setStyleSheet(
	//		"QLineEdit { background-color: rgb(40,40,40) }"
	//		"QPushButton { background-color: rgb(50,50,50); font: bold }");

	SetProperty(prop);
}

int PropertyWidget::RowFromIndex(const QModelIndex& index) const
{
	int count = 0;

	if (index.isValid())
	{
		count = (index.row() + 1) + RowFromIndex(index.parent());

		const QModelIndex parent = index.parent();
		if (parent.isValid())
		{
			for (int r = 0; r < index.row(); ++r)
				count += model()->rowCount(parent.child(r, 0));
		}
	}

	return count;
}

Property::ePropertyType PropertyWidget::ItemType(const QTreeWidgetItem* item) const
{
	auto type = Property::kePropertyTypeSize;
	auto found = m_ItemTypeMap.find(item);
	if (found != m_ItemTypeMap.end())
	{
		type = found->second;
	}
	return type;
}

void PropertyWidget::SetProperty(Property_ptr prop)
{
	if (prop)
	{
		m_Property = prop;
		m_WidgetPropertyMap.clear();
		m_ItemTypeMap.clear();

		clear();

		Build(prop, new QTreeWidgetItem, nullptr);

		const auto set_span = [this](QTreeWidgetItem* item) {
			if (ItemType(item) == Property::kButton)
			{
				item->setFirstColumnSpanned(true);
			}
		};

		for (int i=0; i<topLevelItemCount(); ++i)
		{
			auto item = topLevelItem(i);

			VisitLeaves(item, set_span);
		}
	}
}

template<typename TFunctor>
void PropertyWidget::VisitLeaves(QTreeWidgetItem* item, const TFunctor& functor)
{
	if (item->childCount() == 0)
	{
		functor(item);
	}

	for (int i = 0; i < item->childCount(); ++i)
	{
		auto child = item->child(i);
		VisitLeaves(child, functor);
	}
}

void PropertyWidget::drawRow2(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItemV3 opt = option;
	/*
	bool hasValue = true;
	if (m_editorPrivate)
	{
		QtProperty* property = m_editorPrivate->indexToProperty(index);
		if (property)
			hasValue = property->hasValue();
	}

	if (!hasValue && m_editorPrivate->markPropertiesWithoutValue())
	{
		const QColor c = option.palette.color(QPalette::Dark);
		painter->fillRect(option.rect, c);
		opt.palette.setColor(QPalette::AlternateBase, c);
	}
	else
	{
		const QColor c = m_editorPrivate->calculatedBackgroundColor(m_editorPrivate->indexToBrowserItem(index));
		if (c.isValid())
		{
			painter->fillRect(option.rect, c);
			opt.palette.setColor(QPalette::AlternateBase, c.lighter(112));
		}
	}
	*/
	//const int g = (index.row() % 2 == 0) ? 60 : 80;

	const auto type = ItemType(itemFromIndex(index));

	const QColor c2 = QColor(25, 35, 45);

	const auto c = (type == Property::kGroup) ? QColor(50, 50, 50) : QColor(40, 40, 40);

	painter->fillRect(option.rect, c);
	opt.palette.setColor(QPalette::AlternateBase, c);

	if (false) //type == Property::kButton)
	{
		QStyleOptionViewItem optb = option;

		if (selectionModel()->isSelected(index))
		{
			optb.state |= QStyle::State_Selected;
		}

		int firstSection = header()->logicalIndex(0);
		int lastSection = header()->logicalIndex(header()->count() - 1);
		int left = header()->sectionViewportPosition(firstSection);
		int right = header()->sectionViewportPosition(lastSection) + header()->sectionSize(lastSection);
		int LevelOfThisItem = 1;
		int indent = LevelOfThisItem * indentation();

		left += indent;

		optb.rect.setX(left);
		optb.rect.setWidth(right - left);

		itemDelegate(index)->paint(painter, optb, index);
	}
	else
	{
		QTreeView::drawRow(painter, option, index);
	}

	// draw grid lines
	// https://stackoverflow.com/questions/24636138/showing-gridlines-with-qtreeview

	QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
	QPen pen(color);// QColor(30, 30, 30));
	pen.setWidthF(0);

	auto y = option.rect.y();

	// saving is mandatory to keep alignment through out the row painting
	painter->save();
	painter->setPen(pen);

	// don't draw the horizontal line before the root index
	if (index != model()->index(0, 0))
	{
		painter->drawLine(0, y, option.rect.width(), y);
	}
	// draw vertical lines
	if (type != Property::kButton && type != Property::kGroup)
	{
		painter->translate(visualRect(model()->index(0, 0)).x() - indentation() - .5, -.5);
		for (int id = 0, num_sections = header()->count(); id < num_sections - 1; ++id)
		{
			painter->translate(header()->sectionSize(id), 0);
			painter->drawLine(0, y, 0, y + option.rect.height());
		}
	}
	painter->restore();
}

void PropertyWidget::Build(Property_ptr prop, QTreeWidgetItem* item, QTreeWidgetItem* parent_item)
{
	if (parent_item)
	{
		parent_item->addChild(item);
	}

	const std::string name = prop->Description().empty() ? prop->Name() : prop->Description();
	item->setFlags(Qt::ItemIsEnabled);
	item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

	addTopLevelItem(item);
	expandItem(item);

	m_ItemTypeMap[item] = prop->Type();

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
		//combo->setFrame(true);
		for (auto d : ptyped->Values())
		{
			combo->insertItem(QString::fromStdString(d.second), d.first);
		}
		combo->setCurrentItem(ptyped->Value());
		UpdateState(item, prop.shared_from_this());
		QObject_connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(Edited()));

		Connect(prop.onModified, m_Lifespan, [combo, item, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyEnum*>(p.get());
				combo->setCurrentItem(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(item, p);

				auto ptyped = static_cast<const PropertyEnum*>(p.get());
				auto model = qobject_cast<QStandardItemModel*>(combo->model());
				if (ptyped->HasEnabledFlags() && model)
				{
					for (const auto& i : ptyped->Values())
					{
						if (auto item = model->item(i.first))
						{
							item->setEnabled(ptyped->Enabled(i.first));
						}
					}
				}
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(combo, p);
			}
		});
		return combo;
	}
	case Property::kButton: {
		auto p = dynamic_cast<PropertyButton*>(&prop);
		auto button = new QPushButton(QString::fromStdString(p->ButtonText()), this);
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
	item->setDisabled(!p->Enabled());
	item->setHidden(!p->Visible());

	for (int col=0; col<2; ++col)
	{
		if (auto w = itemWidget(item, col))
		{
			w->setEnabled(p->Enabled());
		}
	}
	setItemHidden(item, !p->Visible());
}

void PropertyWidget::UpdateDescription(QWidget* w, Property_cptr p)
{
	// description/name ? 
	w->setToolTip(Format(p->ToolTip()));
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
						OnPropertyEdited(prop);
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
						OnPropertyEdited(prop);
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
						OnPropertyEdited(prop);
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
						OnPropertyEdited(prop);
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
						OnPropertyEdited(prop);
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