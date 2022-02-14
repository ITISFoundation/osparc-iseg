#include "Property.h"

#include "Logger.h"

namespace iseg {

void Property::OnModified(eChangeType change)
{
	auto me = Self();

	// signal change
	if (me && !m_BlockSignals)
	{
		onModified(me, change);

		// recursively notify parents
		auto p = Parent();
		while (p)
		{
			p->OnChildModified(me, change);
			p = p->Parent();
		}
	}
}

Property_ptr Property::Self()
{
	try
	{
		return shared_from_this();
	}
	catch (const std::exception& e)
	{
		ISEG_WARNING_MSG(e.what());
	}
	return Property_ptr();
}

void Property::OnChildModified(Property_ptr child, eChangeType change) const
{
	// signal change
	if (!m_BlockSignals)
	{
		onChildModified(child, change);
	}
}

void Property::SetDescription(const std::string& v)
{
	if (v != m_Description)
	{
		m_Description = v;
		OnModified(eChangeType::kDescriptionChanged);
	}
}

void Property::SetToolTip(const std::string& v)
{
	if (v != m_ToolTip)
	{
		m_ToolTip = v;
		OnModified(eChangeType::kDescriptionChanged);
	}
}

void Property::SetEnabled(bool v)
{
	if (v != m_Enabled)
	{
		m_Enabled = v;
		OnModified(eChangeType::kStateChanged);
	}
}

void Property::SetVisible(bool v)
{
	if (v != m_Visible)
	{
		m_Visible = v;
		OnModified(eChangeType::kStateChanged);
	}
}

bool Property::NetEnabled() const
{
	bool enabled = Enabled();
	auto p = Parent();
	while (enabled && p)
	{
		enabled = enabled && p->Enabled();
		p = p->Parent();
	}
	return enabled;
}

bool Property::NetVisible() const
{
	bool visible = Visible();
	auto p = Parent();
	while (visible && p)
	{
		visible = visible && p->Visible();
		p = p->Parent();
	}
	return visible;
}

void Property::DumpTree() const
{
	DumpTree(this, 0);
}

void Property::DumpTree(const Property* p, int indent) const
{
	const std::string name = p->Description().empty() ? p->Name() : p->Description();
	ISEG_INFO(std::string(indent, ' ') << name << " : " << p->StringValue());
	for (const auto& child : p->Properties())
	{
		DumpTree(child.get(), indent + 2);
	}
}

std::shared_ptr<PropertyInt> PropertyInt::Create(value_type value, value_type min_value, value_type max_value)
{
	return std::shared_ptr<PropertyInt>(new PropertyInt(value, min_value, max_value));
}

std::shared_ptr<PropertyReal> PropertyReal::Create(value_type value, value_type min_value, value_type max_value)
{
	return std::shared_ptr<PropertyReal>(new PropertyReal(value, min_value, max_value));
}

std::shared_ptr<PropertyBool> PropertyBool::Create(value_type value)
{
	return std::shared_ptr<PropertyBool>(new PropertyBool(value));
}

std::shared_ptr<PropertyString> PropertyString::Create(value_type value)
{
	return std::shared_ptr<PropertyString>(new PropertyString(value));
}

std::shared_ptr<PropertyButton> PropertyButton::Create(value_type value, const std::string& txt)
{
	return std::shared_ptr<PropertyButton>(new PropertyButton(value, txt));
}

void PropertyButton::SetCheckable(bool v)
{
	if (v != m_Checkable)
	{
		m_Checkable = v;
		OnModified(Property::kStateChanged);
	}
}

void PropertyButton::SetChecked(bool v)
{
	if (v != m_Checked)
	{
		m_Checked = v;
		OnModified(Property::kStateChanged);
	}
}

std::shared_ptr<PropertySlider> PropertySlider::Create(value_type value, value_type min_value, value_type max_value)
{
	return std::shared_ptr<PropertySlider>(new PropertySlider(value, min_value, max_value));
}

std::shared_ptr<PropertyGroup> PropertyGroup::Create(const std::string& description)
{
	std::shared_ptr<PropertyGroup> group(new PropertyGroup);
	group->SetDescription(description);
	return group;
}

std::shared_ptr<PropertyEnum> PropertyEnum::Create(const descriptions_type& descriptions, const value_type value)
{
	return std::shared_ptr<PropertyEnum>(new PropertyEnum(descriptions, value));
}

PropertyEnum::PropertyEnum(const descriptions_type& descriptions, const value_type value)
		: m_Value(k_invalid_value)
{
	values_type values;
	for (value_type i = 0; i < (value_type)descriptions.size(); ++i)
		values[i] = descriptions[i];
	m_Values = values;

	if (m_Values.count(value))
	{
		m_Value = value;
	}

	m_Invalid = "#Invalid";
}

const PropertyEnum::value_type PropertyEnum::k_invalid_value;

PropertyEnum::value_type PropertyEnum::Value() const
{
	return m_Value;
}

void PropertyEnum::SetValue(const value_type value)
{
	bool changed = false;
	if (m_Value != value)
	{
		changed = true;
		m_Value = value;
	}
	if (changed)
		OnModified(eChangeType::kValueChanged);
}

PropertyEnum::description_type PropertyEnum::ValueDescription() const
{
	auto it = m_Values.find(m_Value);
	if (it != m_Values.end())
	{
		return it->second;
	}
	return m_Invalid;
}

const PropertyEnum::values_type& PropertyEnum::Values() const
{
	return m_Values;
}

void PropertyEnum::ReplaceValues(values_type const& new_values)
{
	assert(m_Enabled.empty() && "m_Enabled flags may not be up-to-date");
	assert(m_ToolTips.empty() && "m_ToolTips flags may not be up-to-date");
	bool changed = false;
	{
		changed = !(new_values == m_Values);
		if (changed)
		{
			m_Values = new_values;
		}
	}
	if (changed)
	{
		if (m_Values.find(m_Value) == m_Values.end())
		{
			OnModified(eChangeType::kValueChanged);
		}
		else
		{
			OnModified(eChangeType::kDescriptionChanged);
		}
	}
}

void PropertyEnum::ReplaceDescriptions(descriptions_type const& new_descr)
{
	values_type new_values;
	for (value_type i = 0; i < (value_type)new_descr.size(); ++i)
		new_values[i] = new_descr[i];
	ReplaceValues(new_values);
}

void PropertyEnum::SetInvalidDescription(const description_type& descr)
{
	m_Invalid = descr;
}

void PropertyEnum::SetEnabled(value_type index, bool v)
{
	const auto found = m_Enabled.find(index);
	if (found == m_Enabled.end() || found->second != v)
	{
		m_Enabled[index] = v;
		OnModified(eChangeType::kStateChanged);
	}
}

bool PropertyEnum::Enabled(value_type index) const
{
	const auto found = m_Enabled.find(index);
	if (found != m_Enabled.end())
	{
		return found->second;
	}
	return true;
}

void PropertyEnum::SetItemToolTip(value_type index, const std::string& v)
{
	const auto found = m_ToolTips.find(index);
	if (found == m_ToolTips.end() || found->second != v)
	{
		m_ToolTips[index] = v;
		OnModified(eChangeType::kDescriptionChanged);
	}
}

std::string PropertyEnum::ItemToolTip(value_type index) const
{
	const auto found = m_ToolTips.find(index);
	if (found != m_ToolTips.end())
	{
		return found->second;
	}
	return ToolTip();
}

BlockPropertySignal::BlockPropertySignal(Property_ptr p)
		: m_Property(p)
{
	p->m_BlockSignals = true;
}

BlockPropertySignal::~BlockPropertySignal()
{
	if (auto p = m_Property.lock())
	{
		p->m_BlockSignals = false;
	}
}

} // namespace iseg
