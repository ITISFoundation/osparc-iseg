/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "Log.h"

namespace iseg {

void quit(const std::string& msg, const int code)
{
	if (!msg.empty())
		std::cerr << "Tools::quit() : " << msg << "\n";
	if (flog.is_open())
		flog.close();
	exit(code);
}

void error(const std::string& msg, const int code)
{
	const std::string msg2 = "Tools::error() : " + msg;
	std::cerr << "\n"
						<< msg2 << "\n";
	if (flog.is_open())
		flog.close();
	exit(code);
}

void warning(const std::string& msg)
{
	const std::string msg2 = "Tools::warning() : " + msg;
	std::cerr << "\n"
						<< msg2 << "\n\n";
}

bool interceptOutput(const std::string& logname)
{
	if (!flog.is_open())
		flog.open(logname.c_str(), std::ofstream::out | std::ofstream::trunc);

	if (flog.is_open())
	{
		std::cerr.rdbuf(flog.rdbuf());
		std::cout.rdbuf(flog.rdbuf());
		std::clog.rdbuf(flog.rdbuf());
	}
	else
		ISEG_ERROR_MSG("while opening log file");

	FILE *outfile, *errfile;
	if ((outfile = freopen(logname.c_str(), "a", stdout)) == nullptr)
		return false;
	else
		fprintf(stdout, "stdout redirected to %s\n", logname.c_str());
	if ((errfile = freopen(logname.c_str(), "a", stderr)) == nullptr)
		return false;
	else
		fprintf(stderr, "stderr redirected to %s\n", logname.c_str());
	fflush(outfile);
	fflush(errfile);

	std::cerr << "*** Creating log file " + logname + " ***\n";
	return true;
}

} // namespace iseg
