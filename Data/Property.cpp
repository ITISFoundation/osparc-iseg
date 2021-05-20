#include "Property.h"

#include "Logger.h"

namespace iseg {

void Property::OnModified(eChangeType change)
{
	auto me = shared_from_this();

	// signal change
	if (onModified && me)
	{
		onModified(me, change);
	}

	// recursively notify parents
	auto p = Parent();
	while (p)
	{
		p->OnChildModified(me, change);
		p = p->Parent();
	}
}

void Property::OnChildModified(Property_ptr child, eChangeType change) const
{
	// signal change
	if (onModified)
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

std::shared_ptr<PropertyButton> PropertyButton::Create(value_type value)
{
	return std::shared_ptr<PropertyButton>(new PropertyButton(value));
}

std::shared_ptr<PropertyGroup> PropertyGroup::Create()
{
	return std::shared_ptr<PropertyGroup>(new PropertyGroup);
}

PropertyEnum::PropertyEnum(const descriptions_type& descriptions /*= descriptions_type()*/, const value_type value /*= kInvalidValue */)
{
	ReplaceDescriptions(descriptions);
	m_Value = value;
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
	bool changed = false;
	{
		changed = (new_values != m_Values);
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

} // namespace iseg
