/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#pragma once

#include <boost/chrono.hpp>

#include <string>

namespace iseg {

template<typename TUnit=boost::chrono::milliseconds>
class ScopedTimerT
{
public:
	ScopedTimerT(const std::string& scope_name, const std::string& unit) 
		: _scope_name(scope_name), _unit(unit)
	{
		_before = boost::chrono::high_resolution_clock::now();
	}
	~ScopedTimerT()
	{
		new_scope(_scope_name);
	}

	void new_scope(const std::string& scope_name)
	{
		auto const after = boost::chrono::high_resolution_clock::now();
		double count = static_cast<double>( boost::chrono::duration_cast<TUnit>(after - _before).count() );
		std::cerr << "TIMER: " << count << "[" + _unit + "] in " + _scope_name + "\n";

		_before = after;
		_scope_name = scope_name;
	}

private:
	std::string _scope_name;
	std::string _unit;
	boost::chrono::high_resolution_clock::time_point _before;
};

class ScopedTimer : public ScopedTimerT<boost::chrono::seconds>
{
public:
	ScopedTimer(const std::string& scope_name) 
		: ScopedTimerT<boost::chrono::seconds>(scope_name, "[s]") {}
};

class ScopedTimerMilli : public ScopedTimerT<boost::chrono::milliseconds>
{
public:
	ScopedTimerMilli(const std::string& scope_name) 
		: ScopedTimerT<boost::chrono::milliseconds>(scope_name, "[ms]") {}
};

}