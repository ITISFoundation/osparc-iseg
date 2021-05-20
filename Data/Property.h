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

#include <cassert>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
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

	enum eChangeType {
		kValueChanged,
		kDescriptionChanged,
		kStateChanged
	};

	virtual eValueType Type() const = 0;

	virtual std::string StringValue() const { return ""; }
	virtual void SetStringValue(const std::string&) { assert(false && "SetStringValue not implemented for this type"); }

	const std::string& Name() const { return m_Name; }
	void SetName(const std::string& v) { m_Name = v; }

	const std::string& Description() const { return m_Description; }
	void SetDescription(const std::string& v);

	const std::string& ToolTip() const { return m_ToolTip; }
	void SetToolTip(const std::string& v);

	bool Enabled() const { return m_Enabled; }
	void SetEnabled(bool v);

	bool Visible() const { return m_Visible; }
	void SetVisible(bool v);

	const std::vector<Property_ptr>& Properties() const { return m_Properties; }

	using change_callback_type = std::function<void(Property_ptr, eChangeType)>;

	change_callback_type onModified; // NOLINT

	change_callback_type onChildModified; // NOLINT

	void DumpTree() const;

protected:
	Property() = default;
	virtual ~Property() = default;

	Property(const Property&) = delete;
	Property& operator=(const Property&) = delete;

	Property_ptr Parent() { return m_Parent.lock(); }
	Property_cptr Parent() const { return m_Parent.lock(); }

	void OnModified(eChangeType change);

	void OnChildModified(Property_ptr child, eChangeType change) const;

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

protected:
	PropertyT(value_type value)
			: m_Value(value)
	{
	}

private:
	value_type m_Value;
};

template<class T>
void PropertyT<T>::SetValue(value_type v)
{
	if (m_Value != v)
	{
		m_Value = v;
		OnModified(eChangeType::kValueChanged);
	}
}

template<class T>
class PropertyTR : public PropertyT<T>
{
public:
	using value_type = T;

	value_type MinValue() const { return m_MinValue; }
	void SetMinValue(value_type v) { m_MinValue = v; }

	value_type MaxValue() const { return m_MaxValue; }
	void SetMaxValue(value_type v) { m_MaxValue = v; }

protected:
	PropertyTR(value_type value, value_type min_value, value_type max_value)
			: PropertyT<T>(value), m_MinValue(min_value), m_MaxValue(max_value)
	{
	}

private:
	value_type m_MinValue;
	value_type m_MaxValue;
};

class ISEG_DATA_API PropertyInt : public PropertyTR<int>
{
public:
	static std::shared_ptr<PropertyInt> Create(value_type value, value_type min_value = -std::numeric_limits<value_type>::max(), value_type max_value = std::numeric_limits<value_type>::max());

	eValueType Type() const override { return eValueType::kInteger; }

	std::string StringValue() const override { return std::to_string(Value()); }
	void SetStringValue(const std::string& v) override { SetValue(atoi(v.c_str())); }

protected:
	PropertyInt(value_type value, value_type min_value, value_type max_value)
			: PropertyTR<int>(value, min_value, max_value)
	{
	}
};

class ISEG_DATA_API PropertyReal : public PropertyTR<double>
{
public:
	static std::shared_ptr<PropertyReal> Create(value_type value, value_type min_value = -std::numeric_limits<value_type>::max(), value_type max_value = std::numeric_limits<value_type>::max());

	eValueType Type() const override { return eValueType::kReal; }

	std::string StringValue() const override { return std::to_string(Value()); }
	void SetStringValue(const std::string& v) override { SetValue(atof(v.c_str())); }

protected:
	PropertyReal(value_type value, value_type min_value, value_type max_value)
			: PropertyTR<double>(value, min_value, max_value)
	{
	}
};

class ISEG_DATA_API PropertyBool : public PropertyT<bool>
{
public:
	static std::shared_ptr<PropertyBool> Create(value_type value);

	eValueType Type() const override { return eValueType::kBool; }

	std::string StringValue() const override { return Value() ? "True" : "False"; }
	void SetStringValue(const std::string& v) override { SetValue(v == "True" ? true : false); }

protected:
	PropertyBool(value_type value)
			: PropertyT<bool>(value)
	{
	}
};

class ISEG_DATA_API PropertyString : public PropertyT<std::string>
{
public:
	static std::shared_ptr<PropertyString> Create(value_type value);

	eValueType Type() const override { return eValueType::kString; }

	std::string StringValue() const override { return Value(); }
	void SetStringValue(const std::string& v) override { SetValue(v); }

protected:
	PropertyString(const value_type& value)
			: PropertyT<std::string>(value)
	{
	}
};

class ISEG_DATA_API PropertyButton : public Property
{
public:
	using value_type = std::function<void()>;

	static std::shared_ptr<PropertyButton> Create(value_type value);

	value_type Value() const { return m_Value; }
	void SetValue(const value_type& v)
	{
		m_Value = v;
		OnModified(Property::kValueChanged);
	}

	const std::string& ButtonText() const { return m_ButtonText; }
	void SetButtonText(const std::string& v) { m_ButtonText = v; }

	eValueType Type() const override { return eValueType::kButton; }

protected:
	PropertyButton(const value_type& value) : m_Value(value) {}

private:
	value_type m_Value;
	std::string m_ButtonText;
};

class ISEG_DATA_API PropertyEnum : public Property
{
public:
	/// Types
	using value_type = unsigned int;
	using description_type = std::string;
	using values_type = std::map<value_type, description_type>;
	using descriptions_type = std::vector<description_type>;

	/// Invalid value type constant
	static const value_type k_invalid_value = static_cast<value_type>(-1);

	value_type Value() const;
	void SetValue(const value_type value);

	/// The map of values and descriptions
	const values_type& Values() const;

	/// Value description
	description_type ValueDescription() const;

	/*  Replaces all values (i.e. the enum-options) and
			leaves the actual value (the index) intact. Only sends
			a modified signal if the current value is not contained
			in new_values.
 		*/
	void ReplaceValues(values_type const& new_values);
	void ReplaceDescriptions(descriptions_type const& new_descr);

	/// Set alternative invalid value description
	void SetInvalidDescription(const description_type& descr);

protected:
	/** Constructor.		
			Accepts a list of descriptions and the value corresponding to the index of the descriptions.
		*/
	explicit PropertyEnum(const descriptions_type& descriptions = descriptions_type(), const value_type value = k_invalid_value);

private:
	values_type m_Values;
	/// Currently selected value
	value_type m_Value;
	/// Invalid value description. Default: L"#Invalid"
	description_type m_Invalid;
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
