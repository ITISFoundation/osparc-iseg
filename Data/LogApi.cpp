#include "LogApi.h"
#include "Logger.h"

namespace iseg {

void debug(const std::string& msg)
{
	ISEG_DEBUG() << msg;
}

void info(const std::string& msg)
{
	ISEG_INFO() << msg;
}

void warning(const std::string& msg)
{
	ISEG_WARNING() << msg;
}

void error(const std::string& msg)
{
	ISEG_ERROR() << msg;
}

} // namespace iseg