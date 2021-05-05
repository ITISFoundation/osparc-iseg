#include "Property.h"

#include "Logger.h"

namespace iseg {

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

} // namespace iseg
