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

#include "iSegData.h"

#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace iseg {

class Property;
using Property_ptr = std::shared_ptr<Property>;
using Property_cptr = std::shared_ptr<const Property>;
using Property_wptr = std::weak_ptr<Property>;

class ISEG_DATA_API Property : public std::enable_shared_from_this<Property>
{
public:
	enum eValueType {
		kInteger = 0,
		kReal,
		kString,
		kBool,
		kButton,
		kGroup
	};

	virtual eValueType Type() const = 0;

	virtual std::string StringValue() const { return ""; }
	virtual void SetStringValue(const std::string&) {}

	const std::string& Name() const { return m_Name; }
	void SetName(const std::string& v) { m_Name = v; }

	const std::string& Description() const { return m_Description; }
	void SetDescription(const std::string& v) { m_Description = v; }

	const std::string& ToolTip() const { return m_ToolTip; }
	void SetToolTip(const std::string& v) { m_ToolTip = v; }

	bool Enabled() const { return m_Enabled; }
	void SetEnabled(bool v) { m_Enabled = v; }

	bool Visible() const { return m_Visible; }
	void SetVisible(bool v) { m_Visible = v; }

	const std::vector<Property_ptr>& Properties() const { return m_Properties; }

	void DumpTree() const;

protected:
	Property() = default;
	virtual ~Property() = default;

	Property_ptr Parent() { return m_Parent.lock(); }
	Property_cptr Parent() const { return m_Parent.lock(); }

	void DumpTree(const Property* p, int indent) const;

	template<typename T>
	T AddChild(const std::string& name, T v)
	{
		m_Properties.push_back(v);
		v->SetName(name);
		v->m_Parent = shared_from_this();
		return v;
	}

	std::string m_Name;
	std::string m_Description;
	std::string m_ToolTip;
	std::vector<Property_ptr> m_Properties;
	Property_wptr m_Parent;
	bool m_Enabled = true;
	bool m_Visible = true;
};

template<class T>
class PropertyT : public Property
{
public:
	using value_type = T;

	value_type Value() const { return m_Value; }
	void SetValue(value_type v);

	value_type MinValue() const { return m_MinValue; }
	void SetMinValue(value_type v) { m_MinValue = v; }

	value_type MaxValue() const { return m_MaxValue; }
	void SetMaxValue(value_type v) { m_MaxValue = v; }

protected:
	PropertyT(value_type value, value_type min_value, value_type max_value)
			: m_Value(value), m_MinValue(min_value), m_MaxValue(max_value)
	{
	}

private:
	value_type m_Value;
	value_type m_MinValue;
	value_type m_MaxValue;
};

template<class T>
void PropertyT<T>::SetValue(value_type v)
{
	if (m_Value != v)
	{
		m_Value = v;
	}
}

class ISEG_DATA_API PropertyInt : public PropertyT<int>
{
public:
	static std::shared_ptr<PropertyInt> Create(
			value_type value,
			value_type min_value = -std::numeric_limits<value_type>::max(),
			value_type max_value = std::numeric_limits<value_type>::max());

	eValueType Type() const override { return eValueType::kInteger; }

	std::string StringValue() const override { return std::to_string(Value()); }
	void SetStringValue(const std::string& v) override { SetValue(atoi(v.c_str())); }

protected:
	PropertyInt(value_type value, value_type min_value, value_type max_value)
			: PropertyT<int>(value, min_value, max_value)
	{
	}
};

class ISEG_DATA_API PropertyReal : public PropertyT<double>
{
public:
	static std::shared_ptr<PropertyReal> Create(
			value_type value,
			value_type min_value = -std::numeric_limits<value_type>::max(),
			value_type max_value = std::numeric_limits<value_type>::max());

	eValueType Type() const override { return eValueType::kReal; }

	std::string StringValue() const override { return std::to_string(Value()); }
	void SetStringValue(const std::string& v) override { SetValue(atof(v.c_str())); }

protected:
	PropertyReal(value_type value, value_type min_value, value_type max_value)
			: PropertyT<double>(value, min_value, max_value)
	{
	}
};

class ISEG_DATA_API PropertyBool : public Property
{
public:
	using value_type = bool;

	static std::shared_ptr<PropertyBool> Create(value_type value);

	value_type Value() const { return m_Value; }
	void SetValue(value_type v) { m_Value = v; }

	eValueType Type() const override { return eValueType::kBool; }

	std::string StringValue() const override { return Value() ? "True" : "False"; }
	void SetStringValue(const std::string& v) override { SetValue(v == "True" ? true : false); }

protected:
	PropertyBool(value_type value)
			: m_Value(value)
	{
	}

	value_type m_Value;
};

class ISEG_DATA_API PropertyString : public Property
{
public:
	using value_type = std::string;

	static std::shared_ptr<PropertyString> Create(value_type value);

	const value_type& Value() const { return m_Value; }
	void SetValue(const value_type& v) { m_Value = v; }

	eValueType Type() const override { return eValueType::kString; }

	std::string StringValue() const override { return Value(); }
	void SetStringValue(const std::string& v) override { SetValue(v); }

protected:
	PropertyString(const value_type& value)
			: m_Value(value)
	{
	}

	value_type m_Value;
};

class ISEG_DATA_API PropertyButton : public Property
{
public:
	using value_type = std::function<void()>;

	static std::shared_ptr<PropertyButton> Create(value_type value);

	const value_type& Value() const { return m_Value; }
	void SetValue(value_type v) { m_Value = v; }

	const std::string& ButtonText() const { return m_ButtonText; }
	void SetButtonText(const std::string& v) { m_ButtonText = v; }

	eValueType Type() const override { return eValueType::kButton; }

protected:
	PropertyButton(const value_type& value)
			: m_Value(value)
	{
	}

	value_type m_Value;
	std::string m_ButtonText;
};

class ISEG_DATA_API PropertyGroup : public Property
{
public:
	static std::shared_ptr<PropertyGroup> Create();

	eValueType Type() const override { return eValueType::kGroup; }

	template<typename T>
	T Add(const std::string& name, T v)
	{
		return AddChild(name, v);
	}

protected:
    PropertyGroup() = default;
};

} // namespace iseg
