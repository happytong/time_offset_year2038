/************************************************************
DESCRIPTION

	This file defines header to address the Unix timestamp overflow issue that occurs
	on January 19, 2038, at 03:14:07 UTC, when signed 32-bit integers exceed 0x7FFFFFFF
	(2,147,483,647 seconds since epoch).

	Strategy:
	- If time > 0x7FFFFFFF on 32-bit system, subtract offset before setting OS time
	- Applications will add offset back for correct display

Version:
	01	17 Jan 2026, Tong Zhixiong
		Create.

**************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef YEAR2038_H
#include <time.h>

#define Y2038_TIME_OFFSET 	0x7fffffff  // Overflow when > 0x7FFFFFFF
extern int g_nY2038OffsetActive;//Year 2038 offset flag, must be declared in .c/.cc/.cpp

// Get real time as unsigned long (with offset added back if active)
// Returns actual timestamp that may exceed 0x7FFFFFFF
// DO NOT cast to time_t if value > 0x7FFFFFFF!
static inline unsigned long GetRealTimeUL(void)
{
	time_t tOsTime = time(NULL);
	unsigned long ulOsTime = (unsigned long)tOsTime;
	if (g_nY2038OffsetActive) {
		return ulOsTime + Y2038_TIME_OFFSET;
	}
	return ulOsTime;
}

// Manual conversion from Unix timestamp to date/time (works beyond 2038)
// Based on algorithm that handles unsigned long timestamps
static inline void UnixTimeToTm(unsigned long ulTime, struct tm* result)
{
	unsigned long days, seconds;
	int year, mon;

	// Days since epoch and remaining seconds
	days = ulTime / 86400;
	seconds = ulTime % 86400;

	// Time of day
	result->tm_hour = seconds / 3600;
	result->tm_min = (seconds % 3600) / 60;
	result->tm_sec = seconds % 60;

	// Day of week (Jan 1, 1970 was Thursday = 4)
	result->tm_wday = (days + 4) % 7;

	// Calculate year (approximate then adjust)
	year = 1970 + (days / 365);

	// Leap year calculation
	unsigned long leap_years = ((year - 1) - 1968) / 4 - ((year - 1) - 1900) / 100 + ((year - 1) - 1600) / 400;
	unsigned long days_in_years = (year - 1970) * 365 + leap_years;

	// Adjust if we overshot
	while (days_in_years > days) {
		year--;
		leap_years = ((year - 1) - 1968) / 4 - ((year - 1) - 1900) / 100 + ((year - 1) - 1600) / 400;
		days_in_years = (year - 1970) * 365 + leap_years;
	}

	result->tm_year = year - 1900;
	unsigned long yday = days - days_in_years;
	result->tm_yday = yday;

	// Determine if leap year
	int is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);

	// Days in each month
	static const int days_in_month[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};

	// Find month and day
	mon = 0;
	while (mon < 12 && yday >= (unsigned long)days_in_month[is_leap][mon]) {
		yday -= days_in_month[is_leap][mon];
		mon++;
	}

	result->tm_mon = mon;
	result->tm_mday = yday + 1;
	result->tm_isdst = -1;  // Unknown DST status
}

// Get real time and convert to struct tm (works beyond 2038!)
// Use this instead of localtime(time(NULL))
static inline struct tm* GetRealLocalTime(struct tm* result)
{
	unsigned long ulRealTime = GetRealTimeUL();

	// If offset not active and time < 0x7FFFFFFF, use standard function
	if (!g_nY2038OffsetActive && ulRealTime <= 0x7FFFFFFF) {
		time_t t = (time_t)ulRealTime;
		return localtime_r(&t, result);
	}

	// Otherwise use manual conversion (works for any unsigned long value)
	UnixTimeToTm(ulRealTime, result);
	return result;
}

// Format real time to string (works beyond 2038!)
// Use this for log timestamps
static inline void FormatRealTime(char* buffer, size_t size, const char* format)
{
	struct tm tmResult;
	GetRealLocalTime(&tmResult);
	strftime(buffer, size, format, &tmResult);
}

#endif

#ifdef __cplusplus
}
#endif

