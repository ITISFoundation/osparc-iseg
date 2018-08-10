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

#include <boost/shared_ptr.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace iseg {

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

enum eSeverityLevel
{
	debug = 0,
	timing,
	info,
	warning,
	error
};

using Logger = src::severity_logger<eSeverityLevel>;

boost::shared_ptr<Logger> iSegLogger()
{
	static boost::shared_ptr<Logger> logger(new Logger);
	return logger;
}

#define ISEG_DEBUG() BOOST_LOG_SEV(*iSegLogger(), eSeverityLevel::debug)
#define ISEG_INFO() BOOST_LOG_SEV(*iSegLogger(), eSeverityLevel::info)
#define ISEG_WARNING() BOOST_LOG_SEV(*iSegLogger(), eSeverityLevel::warning)
#define ISEG_ERROR() BOOST_LOG_SEV(*iSegLogger(), eSeverityLevel::error)


BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", eSeverityLevel)

std::ostream& operator<< (std::ostream& strm, eSeverityLevel level)
{
	static const char* strings[] =
	{
		"DEBUG",
		"TIMING",
		"INFO",
		"WARNING",
		"ERROR"
	};

	if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
		strm << strings[level];
	else
		strm << static_cast<int>(level);

	return strm;
}

auto init_logging(const std::string& log_file_path, eSeverityLevel minimum_level)
{
	logging::formatter formatter =
		expr::stream
		<< severity << ": " << expr::message;

	auto console_sink = logging::add_console_log();
	console_sink->set_formatter(formatter);
	console_sink->set_filter(severity >= eSeverityLevel::warning);

	auto file_sink = logging::add_file_log(log_file_path);
	file_sink->set_formatter(formatter);
	file_sink->set_filter(severity >= minimum_level);

	// Register common attributes
	//logging::add_common_attributes();

	return file_sink;
}

}