#include "Logger.h"

namespace iseg {

std::ostream& operator<<(std::ostream& strm, eSeverityLevel level)
{
	static const char* strings[] =
	{
		"DEBUG",
		"TIMING",
		"INFO",
		"WARNING",
		"ERROR"
	};

	if (static_cast<size_t>(level) < sizeof(strings) / sizeof(*strings))
		strm << strings[static_cast<size_t>(level)];
	else
		strm << static_cast<int>(level);

	return strm;
}

boost::shared_ptr<Logger> iSegLogger()
{
	static boost::shared_ptr<Logger> logger = boost::make_shared<Logger>();
	return logger;
}

} // namespace iseg