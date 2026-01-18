/************************************************************
DESCRIPTION:
	TestY2038 app to verify the time-offset solution for Year 2038 issue

Version:
	01	17 Jan 2026, Tong Zhixiong
		Create.
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "year2038.h" 

int g_nY2038OffsetActive = 0;
void TestTimeSync(unsigned long ulDateTime)
{
	time_t tOsTime;  // Time value to set in OS

	if (ulDateTime > Y2038_TIME_OFFSET && sizeof(time_t) == 4)
	{
		// Time beyond 2038 on 32-bit system - use offset
		if (!g_nY2038OffsetActive)
		{
			// First time crossing threshold
			g_nY2038OffsetActive = 1;
			printf("Y2038: Activating time offset mode (real time: 0x%lx)\n", ulDateTime);

		}

		// Subtract offset to keep OS time in valid range
		tOsTime = (time_t)(ulDateTime - Y2038_TIME_OFFSET);

		// Verify result is still valid for 32-bit signed
		if (tOsTime > Y2038_TIME_OFFSET || tOsTime < 0)
		{
			printf("ERROR: Time 0x%lx still out of range after offset\n\n", ulDateTime);

			return;
		}
	}
	else
	{
		// Normal operation - no offset needed
		tOsTime = (time_t)ulDateTime;
		if (g_nY2038OffsetActive && ulDateTime <= Y2038_TIME_OFFSET)
		{
			// Time went back below threshold (shouldn't happen normally)
			g_nY2038OffsetActive = 0;
			printf( "Y2038 offset deactivated: %ld\n", ulDateTime);
		}
	}

	time_t tNow = time(NULL);
	int nDiffTime = (int)difftime(tNow, tOsTime);
	if (nDiffTime > 2 || nDiffTime < -2) //only set time when there is a gap
	{
		if (g_nY2038OffsetActive)
		{
			printf("TimeSync (offset mode) diff=%ds (OS=%ld -> %ld, Real=%ld)\n",
					nDiffTime, tNow, (long)tOsTime, (long)ulDateTime);
		}
		else
		{
			printf("TimeSync diff=%ds (%ld -> %ld)\n", nDiffTime, tNow, (long)tOsTime);
		}

		struct timespec stime;
		stime.tv_sec = tOsTime;  // Use offset-adjusted time
		stime.tv_nsec = 0;

		if (clock_settime( CLOCK_REALTIME, &stime) == -1) //set system time
		{
			int n = errno; //22: if time value > 0x7fffffff (invalid argument): > 20380119 03:14:07
			printf("TimeSync error: %d (%s)\n\n", n, strerror(n));
		}
		else
		{
			// Print real time after sync (with offset added back)
			struct tm tmReal;
			UnixTimeToTm(ulDateTime, &tmReal);

			struct tm tmOs;
			time_t tOsTimeNow = time(NULL);
			UnixTimeToTm((unsigned long)tOsTimeNow, &tmOs);
			
			printf("Time synchronized, diff=%ds\n App time: %04d-%02d-%02d %02d:%02d:%02d (%lu)\n OS time: %04d-%02d-%02d %02d:%02d:%02d (%ld)\n\n",
				   nDiffTime,
				   tmReal.tm_year + 1900, tmReal.tm_mon + 1, tmReal.tm_mday,
				   tmReal.tm_hour, tmReal.tm_min, tmReal.tm_sec,
				   ulDateTime, 
				   tmOs.tm_year + 1900, tmOs.tm_mon + 1, tmOs.tm_mday,
				   tmOs.tm_hour, tmOs.tm_min, tmOs.tm_sec,
				   (long)tOsTimeNow);
		}
	}
}

int main(int argc, char *argv[])
{
	unsigned long ulDateTime = 0;
	char szInput[32] = {0};
	char *pszValue = NULL;

	if (argc > 1)
	{
		// Use command line argument
		pszValue = argv[1];
	}
	else
	{
		// Prompt for input
		printf("Enter a time value (EPOCH, hex or decimal, max 0xFFFFFFFF): ");
		if (fgets(szInput, sizeof(szInput), stdin) != NULL)
		{
			// Remove trailing newline
			szInput[strcspn(szInput, "\n")] = '\0';
			pszValue = szInput;
		}
		else
		{
			printf("Error reading input\n");
			return 1;
		}
	}

	// Parse input - support both hex (0x prefix) and decimal
	if (strncmp(pszValue, "0x", 2) == 0 || strncmp(pszValue, "0X", 2) == 0)
	{
		ulDateTime = strtoul(pszValue, NULL, 16);
	}
	else
	{
		ulDateTime = strtoul(pszValue, NULL, 10);
	}

	printf("Calling TestTimeSync with value: 0x%lX (%lu)\n", ulDateTime, ulDateTime);
	TestTimeSync(ulDateTime);

	return 0;
}
