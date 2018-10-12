/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "LoadPlugin.h"

#include "../Data/Logger.h"

//  warning C4996: 'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead.
#pragma warning(disable : 4996)
#pragma warning(disable : 4091)

#include <cstdlib>

#include <fstream>
#include <iomanip>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

// Boost chrono used for timing measurements
#include <boost/chrono.hpp>

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
inline TargType locate_symbol(handle h, std::string const& symbol)
{
	return reinterpret_cast<TargType>(GetProcAddress(h, symbol.c_str()));
}

//_________________________________________________________________//

inline void close(handle h)
{
	if (h)
		FreeLibrary(h);
}

//_________________________________________________________________//

inline std::string error()
{
	LPTSTR msg = nullptr;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  nullptr, GetLastError(),
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0,
				  nullptr);

	std::string res(msg == nullptr ? "Unable to get any error message" : msg);

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
inline TargType locate_symbol(handle h, std::string const& symbol)
{
	return reinterpret_cast<TargType>(dlsym(h, symbol.c_str()));
}

//_________________________________________________________________//

inline void close(handle h)
{
	if (h)
		dlclose(h);
}

//_________________________________________________________________//

inline std::string error() { return dlerror(); }

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

namespace iseg { namespace plugin {

bool LoadPlugin(const std::string& plugin_file_path)
{
	dyn_lib::handle lib_handle = dyn_lib::open(plugin_file_path);
	if (!lib_handle)
		throw std::logic_error(std::string("Unable to load plugin ")
								   .append(plugin_file_path)
								   .append(" : ")
								   .append(dyn_lib::error()));

	return true;
}

bool LoadPlugins(const std::string& directory_path)
{
	ISEG_INFO("Loading plugins directory " << directory_path);

	bool ok = true;
	try
	{
		fs::path plugin_folder(directory_path);
		if (fs::exists(plugin_folder) && fs::is_directory(plugin_folder) &&
			!fs::is_empty(plugin_folder))
		{
			fs::directory_iterator dir_itr(plugin_folder);
			fs::directory_iterator end_iter;
			for (; dir_itr != end_iter; ++dir_itr)
			{
				fs::path module_file(dir_itr->path());
				std::string module_file_s(module_file.string());
#if defined(WIN32)
				if (algo::iends_with(module_file_s, ".ext.dll"))
#elif defined(__APPLE__)
				if (algo::iends_with(module_file_s, ".ext.dylib"))
#else
				if (algo::iends_with(module_file_s, ".ext.so"))
#endif
				{
					try
					{
						ok = LoadPlugin(module_file_s) && ok;
					}
					catch (std::exception& e)
					{
						std::cerr << "Unable to load plugin: " << e.what() << std::endl;
						ok = false;
					}
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
		std::cerr << "Unable to load plugin: " << e.what() << std::endl;
		ok = false;
	}
	return ok;
}

}} // namespace iseg::plugin

//____________________________________________________________________________//
