/********************************************
*
*	main.c - Entry point
*
********************************************/

/*
	This component provides program startup
	and shutdown facilities.
*/

#define _CRTDBG_MAP_ALLOC

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <crtdbg.h>

/* win32 specific? */
#include <fcntl.h>

#include "defs.h"

CRITICAL_SECTION _dbgDisplayMutex;

void DbgDisplayDebugOut (const char* msg, ...) {
	va_list args;

	EnterCriticalSection (&_dbgDisplayMutex);

#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
	printf ("\n("NDBG_NAME") ");
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
	if(msg) {
		va_start (args, msg);
		vprintf (msg, args);
		va_end (args);
	}
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif

	LeaveCriticalSection (&_dbgDisplayMutex);
}

void DbgDisplayError (const char* msg, ...) {
	va_list args;

	EnterCriticalSection (&_dbgDisplayMutex);

#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
	printf ("\n\r("NDBG_NAME") ");
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
	if(msg) {
		va_start (args, msg);
		vprintf (msg, args);
		va_end (args);
	}
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif

	LeaveCriticalSection (&_dbgDisplayMutex);
}

void DbgDisplayMessage (const char* msg, ...) {
	va_list args;

	EnterCriticalSection (&_dbgDisplayMutex);

#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
	printf ("\n("NDBG_NAME") ");
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN);
#endif
	if(msg) {
		va_start (args, msg);
		vprintf (msg, args);
		va_end (args);
	}
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif

	LeaveCriticalSection (&_dbgDisplayMutex);
}

void DbgInfo (void) {
	printf (NDBG_PRODUCT" "NDBG_VERSION);
	printf ("\n"NDBG_COPY" "NDBG_VENDER);
}

void DbgParseCommandLine (int argc, char** argv) {
	if (argv[1]) {
		/* create new session with argv[1] program file */
		dbgSession* session = 0;
		DbgCreateSession (argv[1]);
		do {
			session = DbgGetCurrentSession ();
		}while (session == 0);
		DbgInitialize (session);
	}
}

FILE* DbgOpenPipe (char* name) {
	//fdopen
#ifdef _MSC_VER
	FILE* ret;
	fopen_s (&ret, name, "rb");
	return ret;
#else
	return fopen (name, "r+");
#endif
}

FILE* pipe = 0;
int init=0;
char in[32];

int __stdcall ReadPipeThreadEntry (char* command) {
	init=1;
	printf ("reading from pipe...");
	fread (in,1,32,pipe);
	return 0;
}

int main (int argc, char** argv) {

	DWORD numWritten;
	int i=0;
	HANDLE pipe;
	InitializeCriticalSection(&_dbgDisplayMutex);

	memset(in,0,32);

//	pipe = CreateFile("\\\\.\\pipe\\ndbg",GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, NULL);

	pipe = CreateNamedPipe ("\\\\.\\pipe\\ndbg",
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE
		|PIPE_WAIT|PIPE_ACCEPT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES,
		255,
		255,
		0,
		NULL);

	if (pipe == INVALID_HANDLE_VALUE)
		perror("Error");

	/* reads 1 byte at a time */

	while (TRUE)
	{
		DWORD bytesAvailable;
		PeekNamedPipe(pipe, NULL, 0, NULL, &bytesAvailable, NULL);
		if (bytesAvailable>0) {
			ReadFile(pipe, in,32, &numWritten, NULL);
			printf ("\n\r%s", in);
			memset(in,0,32);
		}
	}

	DbgInfo ();
	printf ("\n");

	/* Create default session; application is in argv[1]. */
	DbgParseCommandLine (argc, argv);
	DbgConsoleEntry ();

	DeleteCriticalSection(&_dbgDisplayMutex);

	_CrtDumpMemoryLeaks();
	return EXIT_SUCCESS;
}
