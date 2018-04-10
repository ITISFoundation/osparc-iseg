/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#define BOOST_TEST_MODULE TestRunner
#define BOOST_TEST_NO_MAIN

#include <boost/test/unit_test.hpp>

//  warning C4996: 'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead.
#pragma warning(disable : 4996)
#pragma warning(disable : 4091)

#include <stdlib.h>

#include <fstream>
#include <iomanip>
#include <map>

#include <boost/test/tree/traverse.hpp>
#include <boost/test/tree/visitor.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include <boost/chrono.hpp>

namespace rt = boost::runtime;
namespace cla = boost::runtime::cla;
namespace fs = boost::filesystem;
namespace algo = boost::algorithm;

// STL
#include <cstdio>
#include <fstream>
#include <iostream>

//_________________________________________________________________//

// System API

namespace dyn_lib {

#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32) // WIN32 API

#	include <windows.h>

typedef HINSTANCE handle;

inline handle open(std::string const& file_name)
{
	return LoadLibrary(file_name.c_str());
}

//_________________________________________________________________//

template<typename TargType>
inline TargType
		locate_symbol(handle h, std::string const& symbol)
{
	return reinterpret_cast<TargType>(GetProcAddress(h, symbol.c_str()));
}

//_________________________________________________________________//

inline void
		close(handle h)
{
	if (h)
		FreeLibrary(h);
}

//_________________________________________________________________//

inline std::string
		error()
{
	LPTSTR msg = NULL;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&msg,
			0, NULL);

	std::string res(msg == NULL ? "Unable to get any error message" : msg);

	if (msg)
		LocalFree(msg);

	return res;
}

fs::path get_exe_dir()
{
	CHAR path[_MAX_PATH] = {'\0'};
	if (!GetModuleFileNameA(0, path, _MAX_PATH))
	{
		return "";
	}

	fs::path exe_path(path);
	return exe_path.branch_path();
}

//_________________________________________________________________//

#elif defined(BOOST_HAS_UNISTD_H) // POSIX API

#	include <dlfcn.h>

#	include <fcntl.h>
#	include <sys/stat.h>
#	include <sys/types.h>

typedef void* handle;

inline handle open(std::string const& file_name)
{
	return dlopen(file_name.c_str(), RTLD_LOCAL | RTLD_LAZY);
}

//_________________________________________________________________//

template<typename TargType>
inline TargType
		locate_symbol(handle h, std::string const& symbol)
{
	return reinterpret_cast<TargType>(dlsym(h, symbol.c_str()));
}

//_________________________________________________________________//

inline void
		close(handle h)
{
	if (h)
		dlclose(h);
}

//_________________________________________________________________//

inline std::string
		error()
{
	return dlerror();
}

fs::path get_exe_dir()
{
	/// \todo For linux this might be enough, but needs to be tested.
	fs::path path = fs::initial_path();
	fs::path last_folder_name = path.filename();
	if (last_folder_name.string() == "test_data")
	{
		return path.parent_path();
	}
	else
	{
		return path;
	}
}

//_________________________________________________________________//

#else

#	error "Dynamic library API is unknown"

#endif

} // namespace dyn_lib

//____________________________________________________________________________//

namespace {
std::string init_func_name("init_unit_test");
fs::path exe_folder;

dyn_lib::handle test_lib_handle;

bool load_test_lib(const std::string& test_lib_name)
{
	test_lib_handle = dyn_lib::open(test_lib_name);
	if (!test_lib_handle)
		throw std::logic_error(std::string("Unable to load test library ")
															 .append(test_lib_name)
															 .append(" : ")
															 .append(dyn_lib::error()));

	return true;
}

bool load_test_libs()
{
	bool ok = true;
	try
	{
		if (fs::exists(exe_folder) && fs::is_directory(exe_folder) && !fs::is_empty(exe_folder))
		{
			fs::directory_iterator dir_itr(exe_folder);
			fs::directory_iterator end_iter;
			for (; dir_itr != end_iter; ++dir_itr)
			{
				fs::path test_file(*dir_itr);
				std::string test_file_s(test_file.string());
#if defined(WIN32)
				if (algo::istarts_with(test_file.filename().string(), "TestSuite_") && algo::iends_with(test_file_s, ".dll"))
#elif defined(__APPLE__)
				if (algo::istarts_with(test_file.filename().string(), "libTestSuite_") && algo::iends_with(test_file_s, ".dylib"))
#else
				if (algo::istarts_with(test_file.filename().string(), "libTestSuite_") && algo::iends_with(test_file_s, ".so"))
#endif
				{
					ok = ok && load_test_lib(test_file_s);
				}
			}
		}
		else
		{
			ok = false;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Unable to load test DLLs: " << e.what() << std::endl;
		ok = false;
	}
	return ok;
}

int SEH_unit_test_main(boost::unit_test::init_unit_test_func init_func, int argc, char* argv[])
{
	return ::boost::unit_test::unit_test_main(init_func, argc, argv);
}
} // namespace

//____________________________________________________________________________//

int BOOST_TEST_CALL_DECL main(int argc, char* argv[])
{
	exe_folder = dyn_lib::get_exe_dir();

	if (load_test_libs())
	{
#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
		extern bool init_unit_test();

		boost::unit_test::init_unit_test_func init_func = &init_unit_test;
#else
		extern ::boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]);

		boost::unit_test::init_unit_test_func init_func = &init_unit_test_suite;
#endif

		// Set data folder
		fs::path current_path = exe_folder / "test_data";
		boost::system::error_code e;
		if (!fs::exists(current_path))
		{
			fs::create_directory(current_path, e);
		}
		fs::current_path(current_path, e);

		::boost::unit_test::master_test_suite_t& mts = ::boost::unit_test::framework::master_test_suite();

		// Parse command-line options
		std::vector<std::vector<char>> argv_vec(argc);
		for (size_t i = 0; i < argv_vec.size(); ++i)
		{
			std::string arg(argv[i]);
			arg.append(1, '\0');
			argv_vec[i] = std::vector<char>(arg.begin(), arg.end());
		}

		// Set some default command-line options
		std::string detect_memory_leaks = "--detect_memory_leaks=0";
		detect_memory_leaks.append(1, '\0');

		// Disabling this option allows to create mini dumps
		std::string catch_system_errors = "--catch_system_errors=no";
		catch_system_errors.append(1, '\0');

#ifdef WIN32
		argv_vec.push_back(std::vector<char>(detect_memory_leaks.begin(), detect_memory_leaks.end()));
		argv_vec.push_back(std::vector<char>(catch_system_errors.begin(), catch_system_errors.end()));
#endif

		int my_argc = (int)argv_vec.size();
		char* my_argv[400];
		for (size_t i = 0; i < argv_vec.size(); ++i)
		{
			my_argv[i] = &argv_vec[i][0];
		}

		return SEH_unit_test_main(init_func, my_argc, my_argv);
	}
	return EXIT_FAILURE;
}
