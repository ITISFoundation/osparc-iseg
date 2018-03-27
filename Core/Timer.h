/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
/*
(C) 2002-2008 Dominik Szczerba
*/

#ifndef TIMER_H
#define TIMER_H

#include <ctime>
#include <iostream>
//#include <time.h>

#ifndef WIN32
#	include <iostream>
#	include <sys/resource.h>
#	include <sys/time.h>
#endif

class Timer
{
public:
	Timer() { reset(); }

	void start() { reset(); };

	void reset(void)
	{
#ifdef WIN32
		c0 = clock();
		c1 = c0;
		time(&t0);
		t1 = t0;
#else
		gettimeofday(&_tstart, &tz);
		// 			gettimeofday(&_tend, &tz);
		_tend = _tstart;
		getrusage(RUSAGE_SELF, &u0);
		// 			getrusage(RUSAGE_SELF, &u1);
		u1 = u0;
#endif
	}

	static void wait(int seconds)
	{
		clock_t endwait;
		endwait = clock() + seconds * CLOCKS_PER_SEC;
		while (clock() < endwait) {}
	}

	void print()
	{
		const double dc = runningClock();
		const double dsc = runningSystemClock();
		const double dt = runningTime();

		std::cerr << "day time [s] = " << dt << std::endl;
		std::cerr << "cpu time [s] = " << dc << std::endl;
		std::cerr << "system time [s] = " << dsc << std::endl;
	}

	double runningClock()
	{
#ifdef WIN32
		c1 = clock();
		return double(c1 - c0) / CLOCKS_PER_SEC;
#else
		getrusage(RUSAGE_SELF, &u1);
		double t1 = u1.ru_utime.tv_sec + (double)u1.ru_utime.tv_usec / 1.e6;
		double t0 = u0.ru_utime.tv_sec + (double)u0.ru_utime.tv_usec / 1.e6;
		return t1 - t0;
#endif
	}

	double runningSystemClock()
	{
#ifdef WIN32
		std::cerr << "warning, system clock not implemented for WIN32" << std::endl;
		return 0.0;
#else
		getrusage(RUSAGE_SELF, &u1);
		double t1 = u1.ru_stime.tv_sec + u1.ru_stime.tv_usec / 1.e6;
		double t0 = u0.ru_stime.tv_sec + u0.ru_stime.tv_usec / 1.e6;
		return t1 - t0;
#endif
	}

	double runningTime()
	{
#ifdef WIN32
		time(&t1);
		return difftime(t1, t0);
#else
		gettimeofday(&_tend, &tz);
		double t2 = (double)_tend.tv_sec + (double)_tend.tv_usec / 1.e6;
		double t1 = (double)_tstart.tv_sec + (double)_tstart.tv_usec / 1.e6;
		return t2 - t1;
#endif
		//return double(t1-t0);
	}

private:
#ifdef WIN32
	time_t t0, t1;
	clock_t c0, c1;
	//float c0, c1;
#else
	struct timeval _tstart, _tend;
	struct timezone tz;
	struct rusage u0, u1;
#endif

}; // class Timer

#endif
