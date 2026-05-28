#include "TimeStamp.h"

#include <atomic>

namespace iseg {

void TimeStamp::Modified()
{
	static std::atomic<modify_time_t> id{0};
	this->m_ModifiedTime = id.fetch_add(1, std::memory_order_relaxed) + 1;
}

} // namespace iseg
