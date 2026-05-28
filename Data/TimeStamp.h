/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in
 Society (IT'IS).
 *
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 *
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#pragma once

#include "iSegData.h"

#include <cstdint>

namespace iseg {

class ISEG_DATA_API TimeStamp
{
public:
	using modify_time_t = std::uint64_t;

	void Modified();

	modify_time_t GetMTime() const
	{
		return m_ModifiedTime;
	}

	bool operator>(const TimeStamp& ts) const
	{
		return (m_ModifiedTime > ts.m_ModifiedTime);
	}

	bool operator<(const TimeStamp& ts) const
	{
		return (m_ModifiedTime < ts.m_ModifiedTime);
	}

	operator modify_time_t() const { return m_ModifiedTime; }

private:
	modify_time_t m_ModifiedTime{0};
};

} // namespace iseg
