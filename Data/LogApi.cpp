#include "LogApi.h"

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>

#include <cassert>
#include <cstdarg>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <vector>

namespace iseg {

namespace {
std::ofstream log_file;
bool print_to_console = true;
bool print_debug_log = false;

std::string to_string(const std::string& format, va_list args, va_list copy)
{
	const int size = 1024;
	char buffer[size];
	const int n = vsnprintf(buffer, size, format.c_str(), args);

	if (n >= size)
	{
		// the return value indicates the proper buffer size
		std::vector<char> dynamic_buffer(n + 1); // allocate a buffer large enough
		dynamic_buffer.back() = '\0';						 // make sure it is a NULL terminated string

		const int nn = vsnprintf(&dynamic_buffer[0], n, format.c_str(), copy);
		assert(nn <= static_cast<int>(dynamic_buffer.size()));
		return std::string(&dynamic_buffer[0]);
	}
	else if (n > -1)
	{
		// all is fine!
		return std::string(buffer);
	}
	return std::string("<encoding error occurred>");
}

void _note(const std::string& msg)
{
	if (print_to_console)
	{
		std::cout << msg << "\n";
		std::cout.flush();
	}

	if (log_file.is_open())
	{
		log_file << msg << "\n";
		log_file.flush();
	}
}

std::ostream& init_log_stream()
{
	class LoggerSink
	{
	public:
		using char_type = char;
		using category = boost::iostreams::sink_tag; // NOLINT

		LoggerSink() = default;
		virtual ~LoggerSink() = default;

		virtual std::streamsize write(char_type const* s, std::streamsize n)
		{
			_note(std::string(s, n));
			return n;
		}
	};

	static boost::iostreams::stream_buffer<LoggerSink> logger_buf((LoggerSink()));
	static std::ostream log_stream(&logger_buf);
	return log_stream;
}
} // namespace

bool Log::AttachLogFile(std::string const& logFileName, bool createNewFile /*= true*/)
{
	if (log_file.is_open())
	{
		log_file.close();
	}

	if (logFileName.empty())
	{
		return true;
	}
	else
	{
		// convert file name to UTF8 for portability reasons with LINUX: http://forums.codeguru.com/archive/index.php/t-408048.html
		// I remember reading somewhere that there was debate on whether the file name argument to open() should be unicode or not (in case of wofstream), the point came up about usability, C++ library is used for development on almost all operating systems and as not all operating systems support Unicode file system, so it was decided to keep common ANSI version of open funciton.
		// But windows support unicode file system and that's why VC++ version of C++ library has a workaround, you can use _wopen(CRT) to open a file using Unicode string argument and attach it to the wofstream object.
		log_file.open(logFileName, std::ios_base::out | (createNewFile ? std::ios_base::trunc : std::ios_base::app));
		return log_file.is_open();
	}
}

bool Log::AttachConsole(bool on)
{
	bool v = print_to_console;
	print_to_console = on;
	return v;
}

bool Log::InterceptCerr()
{
	std::ostream& flog = Log::LogStream();
	std::cerr.rdbuf(flog.rdbuf());
	std::clog.rdbuf(flog.rdbuf());

#if 0
	FILE *outfile, *errfile;
	if ((outfile = freopen(logname.c_str(), "a", stdout)) == nullptr)
	{
		return false;
	}
	if ((errfile = freopen(logname.c_str(), "a", stderr)) == nullptr)
	{
		return false;
	}
	fflush(outfile);
	fflush(errfile);
#endif

	return true;
}

void Log::CloseLogFile()
{
	if (log_file.is_open())
	{
		log_file.close();
	}
}

std::ostream& Log::LogStream()
{
	static std::ostream& os = init_log_stream();
	os.unsetf(std::ios_base::unitbuf);
	return os;
}

void Log::Debug(const char* format, ...)
{
	if (print_debug_log)
	{
		va_list args, copy;
		va_start(args, format);
		va_start(copy, format);

		std::string msg = to_string(std::string("[DEBUG] ") + format, args, copy);

		va_end(copy);
		va_end(args);

		_note(msg);
	}
}

void Log::Info(const char* format, ...)
{
	va_list args, copy;
	va_start(args, format);
	va_start(copy, format);

	std::string msg = to_string(std::string("[INFO] ") + format, args, copy);

	va_end(copy);
	va_end(args);

	_note(msg);
}

void Log::Warning(const char* format, ...)
{
	va_list args, copy;
	va_start(args, format);
	va_start(copy, format);

	std::string msg = to_string(std::string("[WARNING] ") + format, args, copy);

	va_end(copy);
	va_end(args);

	_note(msg);
}

void Log::Error(const char* format, ...)
{
	va_list args, copy;
	va_start(args, format);
	va_start(copy, format);

	std::string msg = to_string(std::string("[ERROR] ") + format, args, copy);

	va_end(copy);
	va_end(args);

	_note(msg);
}

void Log::Note(const std::string& channel, const char* format, ...)
{
	va_list args, copy;
	va_start(args, format);
	va_start(copy, format);

	std::string msg = to_string("[" + channel + "] " + format, args, copy);

	va_end(copy);
	va_end(args);

	_note(msg);
}

void init_logging(const std::string& log_file_name, bool print_to_console, bool print_debug_log, bool intercept_cerr)
{
	Log::AttachConsole(print_to_console);
	Log::AttachLogFile(log_file_name);
	if (intercept_cerr)
	{
		Log::InterceptCerr();
	}
	print_debug_log = print_debug_log;
}

} // namespace iseg