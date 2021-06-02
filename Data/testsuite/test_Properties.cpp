/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <boost/test/unit_test.hpp>

#include "../Property.h"

#include <set>

namespace iseg {

BOOST_AUTO_TEST_SUITE(iSeg_suite);
// TestRunner.exe --run_test=iSeg_suite/Property_suite --log_level=message
BOOST_AUTO_TEST_SUITE(Property_suite);

BOOST_AUTO_TEST_CASE(Property_signals)
{
	auto group = PropertyGroup::Create();
	group->SetDescription("Settings");
	auto pi = group->Add("Iterations", PropertyInt::Create(5));
	pi->SetDescription("Number of iterations");
	auto pf = group->Add("Relaxation", PropertyReal::Create(0.5));
	auto child_group = group->Add("Advanced", PropertyGroup::Create());
	child_group->SetDescription("Advanced Settings");
	auto pi2 = child_group->Add("Internal Iterations", PropertyInt::Create(2));
	auto pb = child_group->Add("Enable", PropertyBool::Create(true));
	auto pbtn0 = child_group->Add("Update", PropertyButton::Create([]() {}));
	auto ps = child_group->Add("Name", PropertyString::Create("Bar"));
	auto pe = child_group->Add("Method", PropertyEnum::Create({"Foo", "Bar", "Hello"}, 0));


	iseg::Connect(pb->onModified, pb, [&](Property_ptr p, Property::eChangeType type) {
		pe->SetVisible(pb->Value());
		ps->SetEnabled(pb->Value());
		pi2->SetEnabled(pb->Value());
	});

	std::set<Property_ptr> prop_value_changed;
	std::set<Property_ptr> prop_state_changed;
	iseg::Connect(group->onChildModified, group, [&](Property_ptr p, Property::eChangeType type) {
		if (type == Property::kValueChanged)
		{
			prop_value_changed.insert(p);
		}
		else if (type == Property::kStateChanged)
		{
			prop_state_changed.insert(p);
		}
	});

	const auto clear = [&]() {
		prop_value_changed.clear();
		prop_state_changed.clear();
	};

	// nothing changed
	pe->SetEnabled(0, true);
	BOOST_CHECK_EQUAL(prop_value_changed.size(), 0);
	BOOST_CHECK_EQUAL(prop_state_changed.size(), 1);
	clear();

	// value changed
	pi->SetValue(pi->Value() + 1);
	BOOST_CHECK_EQUAL(prop_value_changed.size(), 1);
	BOOST_CHECK_EQUAL(prop_state_changed.size(), 0);
	clear();

	// value changed, triggering state changes in other props
	pb->SetValue(!pb->Value());
	BOOST_CHECK_EQUAL(prop_value_changed.size(), 1);
	BOOST_CHECK_EQUAL(prop_state_changed.size(), 3);
	clear();

	pb->SetVisible(false);
	BOOST_CHECK_EQUAL(prop_value_changed.size(), 0);
	BOOST_CHECK_EQUAL(prop_state_changed.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
