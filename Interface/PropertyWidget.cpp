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

PropertyWidget::PropertyWidget(Property_ptr p, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	SetProperty(p);
}

void PropertyWidget::SetProperty(Property_ptr p)
{
	if (p && p != m_Property)
	{
		m_Property = p;

		m_WidgetPropertyMap.clear();
		m_CollapseButtonAreaMap.clear();

		auto layout = new QVBoxLayout(this);
		Build(m_Property, layout);
		layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
		setLayout(layout);
	}
}

QSize PropertyWidget::sizeHint() const 
{
	const auto sz = QWidget::sizeHint();
	return QSize(std::max(sz.width(), 300), sz.height());
}

void PropertyWidget::UpdateState(QWidget* w, Property_cptr p)
{
	w->setEnabled(p->Enabled());
	w->setVisible(p->Visible());
}

void PropertyWidget::UpdateDescription(QWidget* w, Property_cptr p)
{
	// description/name ? 
	w->setToolTip(Format(p->ToolTip()));
}

QWidget* PropertyWidget::MakePropertyUi(Property& prop, QWidget* container)
{
	const auto make_line_edit = [this, container](const Property& p) {
		// generic attributes
		auto edit = new QLineEdit(this);
		edit->setToolTip(QString::fromStdString(p.ToolTip()));
		container->setEnabled(p.Enabled());
		container->setVisible(p.Visible());
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyInt*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyReal*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyString*>(p.get());
				edit->setText(QString::fromStdString(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);
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
		checkbox->setStyleSheet("QCheckBox::indicator {width: 13px; height: 13px; }");
		UpdateState(container, prop.shared_from_this());
		QObject_connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(checkbox), [checkbox, container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyBool*>(p.get());
				checkbox->setChecked(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);
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
			combo->insertItem(QString::fromStdString(d.second), d.first);
		}
		combo->setCurrentItem(ptyped->Value());
		UpdateState(container, prop.shared_from_this());
		QObject_connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(combo), [combo, container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyEnum*>(p.get());
				combo->setCurrentItem(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);

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
		UpdateState(container, prop.shared_from_this());
		QObject_connect(button, SIGNAL(released()), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(button), [button, container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);
			}
			else if (type == Property::eChangeType::kDescriptionChanged)
			{
				UpdateDescription(button, p);
			}
		});
		return button;
	}
	default:
		return new QWidget; // \todo so we have two columns
	}
}

void PropertyWidget::Build(Property_ptr prop, QBoxLayout* layout)
{
	const auto label_text = prop->Description().empty() ? prop->Name() : prop->Description();
	auto container = new QWidget;
	auto field = MakePropertyUi(*prop, container);
	field->setMaximumHeight(row_height);

	// for callbacks
	m_WidgetPropertyMap[field] = prop;

	if (prop->Type() == Property::kGroup)
	{
		auto collapse_button = new QToolButton;
		collapse_button->setStyleSheet("QToolButton { border: none; }");
		collapse_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		collapse_button->setArrowType(Qt::ArrowType::DownArrow);
		collapse_button->setCheckable(true);
		collapse_button->setChecked(true);
		collapse_button->setIconSize(QSize(5, 5));
		collapse_button->setMaximumHeight(row_height);

		auto label = new QLabel(QString::fromStdString(label_text));
		label->setMaximumHeight(row_height);

		auto header_hbox = new QHBoxLayout;
		header_hbox->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
		header_hbox->addWidget(collapse_button);
		header_hbox->addWidget(label);
		header_hbox->setSpacing(0);

		auto header_area = new QWidget;
		header_area->setLayout(header_hbox);

		auto child_vbox = new QVBoxLayout;
		child_vbox->setMargin(0);
		if (!prop->Properties().empty())
		{
			auto header_line = new QFrame;
			header_line->setFrameShape(QFrame::HLine);
			header_line->setFrameShadow(QFrame::Plain);
			header_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

			for (const auto& p : prop->Properties())
			{
				Build(p, child_vbox);
			}
		}
		auto child_area = new QWidget;
		child_area->setLayout(child_vbox);

		auto collapse_vbox = new QVBoxLayout;
		collapse_vbox->addWidget(header_area);
		collapse_vbox->addWidget(child_area);
		collapse_vbox->setContentsMargins(child_indent, 0, 0, 0);
		collapse_vbox->setSpacing(0);

		m_CollapseButtonAreaMap[collapse_button] = child_area;

		container->setLayout(collapse_vbox);
		layout->addWidget(container);

		QObject_connect(collapse_button, SIGNAL(clicked(bool)), this, SLOT(ToggleCollapsable(bool)));

		Connect(prop->onModified, QSharedPtrHolder::WeakPtr(container), [container, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(container, p);
			}
		});
	}
	else
	{
		auto hbox = new QHBoxLayout;

		if (prop->Type() != Property::kButton)
		{
			auto label = new QLabel(QString::fromStdString(label_text));
			label->setMaximumHeight(row_height);

			// split at 50%
			QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
			sp.setHorizontalStretch(1);
			label->setSizePolicy(sp);
			field->setSizePolicy(sp);

			hbox->addWidget(label);
		}
		hbox->addWidget(field);
		hbox->setSpacing(0);
		hbox->setContentsMargins(child_indent, 0, 0, 0);

		container->setLayout(hbox);
		layout->addWidget(container);
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

void PropertyWidget::ToggleCollapsable(bool checked)
{
	if (auto btn = dynamic_cast<QToolButton*>(QObject::sender()))
	{
		btn->setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);

		if (auto w = m_CollapseButtonAreaMap[btn])
		{
			auto animation = new QPropertyAnimation(w, "maximumHeight");
			animation->setDuration(100);
			animation->setStartValue(w->maximumHeight());
			animation->setEndValue(checked ? w->sizeHint().height() : 0);
			animation->start();

			QObject_connect(animation, SIGNAL(finished()), this, SLOT(update()));
		}
	}
}

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

PropertyTreeWidget::PropertyTreeWidget(Property_ptr prop, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QTreeWidget(parent)
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
	//splitter->setAttribute(Qt::WA_Hover);
	//splitter->setMouseTracking(true);

	//setAlternatingRowColors(true);
	//setStyleSheet("alternate-background-color: yellow;background-color: red;");

	setItemDelegate(m_ItemDelegate);

	// setup the headers
	QStringList header_list;
	header_list.append("Property");
	header_list.append("Value");

	setHeaderLabels(header_list);
	//header()->setSectionResizeMode( QHeaderView::ResizeToContents );
	setColumnWidth(0, 150);
	setColumnWidth(1, 150);

	header()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
	header()->setSectionsMovable(false);
#endif

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	setStyleSheet(
			"QLineEdit { background-color: rgb(40,40,40) }"
			"QPushButton { background-color: rgb(50,50,50); font: bold }");

	SetProperty(prop);
}

int PropertyTreeWidget::RowFromIndex(const QModelIndex& index) const
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

Property::ePropertyType PropertyTreeWidget::ItemType(const QTreeWidgetItem* item) const
{
	auto type = Property::kePropertyTypeSize;
	auto found = m_ItemTypeMap.find(item);
	if (found != m_ItemTypeMap.end())
	{
		type = found->second;
	}
	return type;
}

void PropertyTreeWidget::SetProperty(Property_ptr prop)
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
void PropertyTreeWidget::VisitLeaves(QTreeWidgetItem* item, const TFunctor& functor)
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

void PropertyTreeWidget::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
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

	//QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
	QPen pen(QColor(30, 30, 30));
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

void PropertyTreeWidget::Build(Property_ptr prop, QTreeWidgetItem* item, QTreeWidgetItem* parent_item)
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

		for (const auto& p : prop->Properties())
		{
			Build(p, new QTreeWidgetItem, item);
		}
	}
}

QWidget* PropertyTreeWidget::MakePropertyUi(Property& prop, QTreeWidgetItem* item)
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, item, this](Property_ptr p, Property::eChangeType type) {
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, item, this](Property_ptr p, Property::eChangeType type) {
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, item, this](Property_ptr p, Property::eChangeType type) {
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
		checkbox->setStyleSheet("QCheckBox::indicator {width: 13px; height: 13px; }");
		UpdateState(item, prop.shared_from_this());
		QObject_connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(checkbox), [checkbox, item, this](Property_ptr p, Property::eChangeType type) {
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
		combo->setFrame(true);
		for (auto d : ptyped->Values())
		{
			combo->insertItem(QString::fromStdString(d.second), d.first);
		}
		combo->setCurrentItem(ptyped->Value());
		UpdateState(item, prop.shared_from_this());
		QObject_connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(combo), [combo, item, this](Property_ptr p, Property::eChangeType type) {
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(button), [button, item, this](Property_ptr p, Property::eChangeType type) {
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
		return new QWidget; // \todo so we have two columns
	}
}

void PropertyTreeWidget::UpdateState(QTreeWidgetItem* item, Property_cptr p)
{
	item->setDisabled(!p->Enabled());
	item->setHidden(!p->Visible());

	if (auto w = itemWidget(item, 1))
	{
		w->setEnabled(p->Enabled());
	}
	setItemHidden(item, !p->Visible());
}

void PropertyTreeWidget::UpdateDescription(QWidget* w, Property_cptr p)
{
	// description/name ? 
	w->setToolTip(Format(p->ToolTip()));
}

void PropertyTreeWidget::Edited()
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


PropertyFormWidget::PropertyFormWidget(Property_ptr prop, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent)
{
	m_FormLayout = new QFormLayout;
	m_FormLayout->setVerticalSpacing(0);
	m_FormLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	setLayout(m_FormLayout);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	SetProperty(prop);
}

void PropertyFormWidget::SetProperty(Property_ptr prop)
{
	if (prop)
	{
		m_Property = prop;
		m_WidgetPropertyMap.clear();

		Build(prop, 0);
	}
}

int PropertyFormWidget::Build(Property_ptr prop, int row)
{
	const std::string name = prop->Description().empty() ? prop->Name() : prop->Description();

	if (prop->Type() != Property::kGroup)
	{
		auto widget = MakePropertyUi(*prop, row);

		// add to parent widget for extra padding
		auto harea = new QWidget;
		auto hbox = new QHBoxLayout;
		hbox->setContentsMargins(0, 2, 0, 2);
		hbox->addWidget(widget);
		harea->setLayout(hbox);

		if (prop->Type() == Property::kButton)
		{
			m_FormLayout->addRow(harea);
		}
		else
		{
			m_FormLayout->addRow(name.c_str(), harea);
		}

		m_WidgetPropertyMap[widget] = prop;
	}
	else
	{
		m_FormLayout->addRow(name.c_str(), new QWidget);
	}

	for (const auto& p : prop->Properties())
	{
		row = Build(p, row + 1);
	}

	return row;
}

QWidget* PropertyFormWidget::MakePropertyUi(Property& prop, int row)
{
	const auto make_line_edit = [this, row](const Property& p) {
		// generic attributes
		auto edit = new QLineEdit(this);
		edit->setFrame(false);
		edit->setToolTip(QString::fromStdString(p.ToolTip()));
		UpdateState(row, p.shared_from_this());
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, row, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyInt*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(row, p);
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, row, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyReal*>(p.get());
				edit->setText(QString::number(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(row, p);
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

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(edit), [edit, row, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyString*>(p.get());
				edit->setText(QString::fromStdString(ptyped->Value()));
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(row, p);
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
		checkbox->setStyleSheet("QCheckBox::indicator {width: 13px; height: 13px; }");
		UpdateState(row, prop.shared_from_this());
		QObject_connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(checkbox), [checkbox, row, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyBool*>(p.get());
				checkbox->setChecked(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(row, p);
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
		combo->setFrame(true);
		for (auto d : ptyped->Values())
		{
			combo->insertItem(QString::fromStdString(d.second), d.first);
		}
		combo->setCurrentItem(ptyped->Value());
		UpdateState(row, prop.shared_from_this());
		QObject_connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(combo), [combo, row, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kValueChanged)
			{
				auto ptyped = static_cast<const PropertyEnum*>(p.get());
				combo->setCurrentItem(ptyped->Value());
			}
			else if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(row, p);

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
		UpdateState(row, prop.shared_from_this());
		QObject_connect(button, SIGNAL(released()), this, SLOT(Edited()));

		Connect(prop.onModified, QSharedPtrHolder::WeakPtr(button), [button, row, this](Property_ptr p, Property::eChangeType type) {
			if (type == Property::eChangeType::kStateChanged)
			{
				UpdateState(row, p);
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
		return new QWidget; // \todo so we have two columns
	}
}

void PropertyFormWidget::UpdateState(int row, Property_cptr p)
{
	if (auto item0 = m_FormLayout->itemAt(row, QFormLayout::ItemRole::LabelRole))
	{
		if (auto w0 = item0->widget())
		{
			w0->setEnabled(p->Enabled());
			w0->setVisible(p->Visible());
		}
	}
	if (auto item1 = m_FormLayout->itemAt(row, QFormLayout::ItemRole::FieldRole))
	{
		if (auto w1 = item1->widget())
		{
			w1->setEnabled(p->Enabled());
			w1->setVisible(p->Visible());
		}
	}
}

void PropertyFormWidget::UpdateDescription(QWidget* w, Property_cptr p)
{
	// description/name ? 
	w->setToolTip(Format(p->ToolTip()));
}

void PropertyFormWidget::Edited()
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


std::weak_ptr<char> QSharedPtrHolder::WeakPtr(QObject* parent)
{
	return (new QSharedPtrHolder(parent))->m_Lifespan;
}

} // namespace iseg