#include "PropertyWidget.h"

#include "CollapsibleWidget.h"
#include "FormatTooltip.h"

#include "../Data/Logger.h"
#include "../Data/Property.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>

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
	const auto make_line_edit = [this](const Property& p) {
		// generic attributes
		auto edit = new QLineEdit(this);
		edit->setToolTip(QString::fromStdString(p.ToolTip()));
		edit->setEnabled(p.Enabled());
		edit->setVisible(p.Visible());
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
		hbox->setSpacing(2);
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

std::weak_ptr<char> QSharedPtrHolder::WeakPtr(QObject* parent)
{
	return (new QSharedPtrHolder(parent))->m_Lifespan;
}

} // namespace iseg