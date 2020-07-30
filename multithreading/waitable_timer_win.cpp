//
// Console Windows application.
// Did it in Visual Studio.

#include "stdafx.h"

#define UNICODE 1
#define _UNICODE 1

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define _SECOND 10000000

typedef struct _MYDATA
{
	TCHAR *szBaseName;
	DWORD dwCounter;
	DWORD dwCounter2;
} MYDATA;

VOID CALLBACK TimerAPCProc(
	LPVOID lpArg,			// Data value
	DWORD dwTimerLowValue,	// Timer low value
	DWORD dwTimerHighValue) // Timer high value
{
	// Formal parameters not used in this example.
	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);

	MYDATA *pMyData = (MYDATA *)lpArg;

	_tprintf(TEXT("\nGenerating data for base \"%s\"\n"), pMyData->szBaseName);

	int sleepInterval = 1000; // pMyData->dwCounter * 10 - pMyData->dwCounter2 * 50;

	_tprintf(TEXT("      sleepInterval: %d\n"), sleepInterval);
	Sleep(abs(sleepInterval));

	_tprintf(TEXT("      Counter2: %d\n"), pMyData->dwCounter2);
	pMyData->dwCounter2++;

	_tprintf(TEXT("Done. Counter: %d\n\n"), pMyData->dwCounter);

	MessageBeep(0);
}

int main(void)
{
	HANDLE hTimer;
	BOOL bSuccess;
	__int64 qwDueTime;
	LARGE_INTEGER liDueTime;
	MYDATA MyData;

	MyData.szBaseName = (wchar_t *)L"My base"; //  TEXT("This is my data");
	MyData.dwCounter = 100;
	MyData.dwCounter2 = 0;

	int somearray[4] = {1, 2, 3};

	int aaaa = somearray[1];

	hTimer = CreateWaitableTimer(
		NULL,			  // Default security attributes
		FALSE,			  // Create auto-reset timer
		TEXT("MyTimer")); // Name of waitable timer

	if (hTimer != NULL)
	{
		__try
		{
			// Create an integer that will be used to signal the timer
			// 5 seconds from now.
			qwDueTime = -5 * _SECOND;

			// Copy the relative time into a LARGE_INTEGER.
			liDueTime.LowPart = (DWORD)(qwDueTime & 0xFFFFFFFF);
			liDueTime.HighPart = (LONG)(qwDueTime >> 32);

			bSuccess = SetWaitableTimer(
				hTimer,		  // Handle to the timer object
				&liDueTime,	  // When timer will become signaled
				2000,		  // Periodic timer interval of 2 seconds
				TimerAPCProc, // Completion routine
				&MyData,	  // Argument to the completion routine
				FALSE);		  // Do not restore a suspended system

			if (bSuccess)
			{
				for (; MyData.dwCounter < 10000000; MyData.dwCounter += 1)
				{
					SleepEx(
						1,	   //INFINITE,     // Wait forever
						TRUE); // Put thread in an alertable state
					printf(".");
				}
			}
			else
			{
				printf("SetWaitableTimer failed with error %d\n", GetLastError());
			}
		}
		__finally
		{
			CloseHandle(hTimer);
		}
	}
	else
	{
		printf("CreateWaitableTimer failed with error %d\n", GetLastError());
	}

	return 0;
}
