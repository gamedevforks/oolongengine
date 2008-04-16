//---------------------------------------------------------
//
// Timer.cpp
// iGDK
//
// Description: 
//
// Last modified: December 8th, 2007
//---------------------------------------------------------
#include "Timer.h"
#include <sys/time.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <Mathematics.h>


// 
// GetTime
// Time in milliseconds since 1970
//
unsigned long GetTimeInMsSince1970()
{
	timeval tv;
	// The time is expressed in seconds and microseconds since midnight (0 hour), January 1, 1970.
	gettimeofday(&tv,NULL);
	// to receive milliseconds we transform the seconds to milliseconds and the microseconds to milliseconds
	// and then add them
	return (unsigned long)((tv.tv_sec*1000) + (tv.tv_usec/1000.0));
}

//
// This is a replacement for QueryPerformanceFrequency / QueryPerformanceCounter
// returns nanoseconds since system start
//
unsigned long GetTimeInNsSinceCPUStart()
{
	double time;
	
	time = mach_absolute_time();
	
	// this is the timebase info
    static mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    double nano = ( (double) info.numer) / ((double) info.denom);
	
	return nano * time / 1000000000.0;
}

//
// returns Ticks since system start
//
unsigned long GetTimeInTicksSinceCPUStart()
{
		// return value is nanoseconds
		//result = gethrtime();
		//result = _rdtsc();
		INT64BIT time;
		// This function returns its result in terms of the Mach absolute time unit. 
		// This unit is CPU dependent, so you can't just multiply it by a constant to get a real world value. 
		time = (INT64BIT) mach_absolute_time();
		
		return time;
}


