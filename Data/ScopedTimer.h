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

#include "Logger.h"

#include <chrono>
#include <string>

namespace iseg {

template<typename TUnit>
class ScopedTimerT
{
public:
	ScopedTimerT(const std::string& scope_name, const std::string& unit)
			: m_ScopeName(scope_name), m_Unit(unit)
	{
		m_Before = std::chrono::high_resolution_clock::now();
	}
	~ScopedTimerT()
	{
		NewScope(m_ScopeName);
	}

	void NewScope(const std::string& scope_name)
	{
		auto const after = std::chrono::high_resolution_clock::now();
		double count = static_cast<double>(std::chrono::duration_cast<TUnit>(after - m_Before).count());
		Log::Note("TIMER", "%g %s in %s", count, m_Unit.c_str(), m_ScopeName.c_str());

		m_Before = after;
		m_ScopeName = scope_name;
	}

private:
	std::string m_ScopeName;
	std::string m_Unit;
	std::chrono::high_resolution_clock::time_point m_Before;
};

class ScopedTimer : public ScopedTimerT<std::chrono::seconds>
{
public:
	ScopedTimer(const std::string& scope_name)
			: ScopedTimerT<std::chrono::seconds>(scope_name, "s") {}
};

class ScopedTimerMilli : public ScopedTimerT<std::chrono::milliseconds>
{
public:
	ScopedTimerMilli(const std::string& scope_name)
			: ScopedTimerT<std::chrono::milliseconds>(scope_name, "ms") {}
};

} // namespace iseg