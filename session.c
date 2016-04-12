/********************************************
*
*	session.c - Debug session management
*
********************************************/

/*
	This component provides debug session facilities.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "list.h"

/* current session */
static dbgSession*	_currentSession = 0;

INLINE char* DbgSessionGetProcessName (dbgSession* session) {
	return session->process.name;
}

INLINE dbgPtid* DbgSessionGetPtid (dbgSession* session) {
	return &session->process.id;
}

void DbgRegisterEventProc (dbgSession* session, DbgSessionEventProc proc) {

	if (session) {

//		DbgMutexLock (&session->mutex);
		session->proc = proc;
//		DbgMutexUnlock (&session->mutex);
	}
	else
		printf ("EVENTPROC SESSION INVALID.\n\r");
}

dbgSession* DbgGetCurrentSession (void) {
	return _currentSession;
}

void DbgSetCurrentSession (dbgSession* session) {
	_currentSession = session;
}

dbgSession* DbgSessionNew (char* command, pid_t pid, tid_t tid, handle_t process, handle_t thread) {
	dbgSession* session = (dbgSession*) malloc (sizeof (dbgSession));   
	if (!session)
		return 0;
	session->process.name = command;
	session->process.id.pid = pid;
	session->process.id.tid = tid;
	session->process.process = process;
	session->process.thread = thread;
	session->state = DBG_STATE_CONTINUE;
	session->proc = 0;
	listInit (&session->process.libraryList);
	listInit (&session->process.threadList);
	listInit (&session->process.sourceFileList);
	listInit (&session->process.breakPointList);
	listInit (&session->process.watchPointList);
	return session;
}

void DbgSessionDeleteProc (dbgSession* session) {
	listNode* current;
	size_t    c;

	listFreeAll(&session->process.libraryList);
	listFreeAll(&session->process.threadList);
	current = session->process.sourceFileList.first;
	for (c=0; c<session->process.sourceFileList.count; c++) {
		dbgSourceFile* sourceFile;

		sourceFile = (dbgSourceFile*) current->data;
		free (sourceFile->name);
		sourceFile->name = 0;

		listFreeAll (&sourceFile->sourceLineList);
		current = current->next;
	}
	listFreeAll(&session->process.sourceFileList);
}

void DbgSessionDelete (dbgSession* session) {
	if (!session)
		return;
	if (session->process.id.pid)
		DebugActiveProcessStop (session->process.id.pid);
	if (session->process.thread)
		CloseHandle ((HANDLE)session->process.thread);
	if (session->process.process)
		CloseHandle ((HANDLE)session->process.process);
	DbgSessionDeleteProc (session);
}

//FlushInstructionCache(m_cProcessInfo.hProcess,(void*)dwStartAddress,1);

/**
*	Converts NDBG Context descriptor to Win32 CONTEXT descriptor
*	\param in NDBG Context Descriptor
*	\param out Win32 CONTEXT descriptor
*/
void DbgWin32ContextFromDbg (IN dbgContext* in, OUT CONTEXT* out) {
	/*
		dont support CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS yet.
		When we do, can just use CONTEXT_ALL
	*/
	memset (out, 0, sizeof (CONTEXT));
	out->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

	out->Eip = in->eip;

	out->SegCs = in->sregs.cs;
	out->SegDs = in->sregs.ds;
	out->SegEs = in->sregs.es;
	out->SegFs = in->sregs.fs;
	out->SegGs = in->sregs.gs;
	out->SegSs = in->sregs.ss;

	out->EFlags = in->flags;

	out->Eax = in->regs.eax;
	out->Ebx = in->regs.ebx;
	out->Ecx = in->regs.ecx;
	out->Edx = in->regs.edx;
	out->Esi = in->regs.esi;
	out->Edi = in->regs.edi;
	out->Ebp = in->regs.ebp;
	out->Esp = in->regs.esp;

	out->Dr0 = in->dregs.dr0;
	out->Dr1 = in->dregs.dr1;
	out->Dr2 = in->dregs.dr2;
	out->Dr3 = in->dregs.dr3;
	out->Dr6 = in->dregs.dr6;
	out->Dr7 = in->dregs.dr7;
}

/**
*	Converts Win32 Context descriptor to NDBG context descriptor
*	\param in Win32 CONTEXT descriptor
*	\param out NDBG Context descriptor
*/
void DbgContextFromWin32 (IN CONTEXT* in, OUT dbgContext* out) {

	memset (out, 0, sizeof (dbgContext));

	out->eip = in->Eip;

	out->sregs.cs = (uint16_t) in->SegCs;
	out->sregs.ds = (uint16_t) in->SegDs;
	out->sregs.es = (uint16_t) in->SegEs;
	out->sregs.fs = (uint16_t) in->SegFs;
	out->sregs.gs = (uint16_t) in->SegGs;
	out->sregs.ss = (uint16_t) in->SegSs;

	out->flags = in->EFlags;

	out->regs.eax = in->Eax;
	out->regs.ebx = in->Ebx;
	out->regs.ecx = in->Ecx;
	out->regs.edx = in->Edx;
	out->regs.esi = in->Esi;
	out->regs.edi = in->Edi;
	out->regs.ebp = in->Ebp;
	out->regs.esp = in->Esp;

	out->dregs.dr0 = in->Dr0;
	out->dregs.dr1 = in->Dr1;
	out->dregs.dr2 = in->Dr2;
	out->dregs.dr3 = in->Dr3;
	out->dregs.dr6 = in->Dr6;
	out->dregs.dr7 = in->Dr7;
}

/**
*	Process session request
*
*	This service implements the OS independent API for sending requests to the environment.
*	This session is Windows specific and so will call the operating system. The NDBG executive
*	session manager would send requests over PIPE to NDBG executive debugger server instead.
*
*	\param request Session request
*	\param session Debug session
*	\param addr Optional data address
*	\param data Optional data buffer
*	\param size Optional data buffer size
*	\ret The number of bytes read or written OR TRUE on success, FALSE on failure depending on request
*
*/
unsigned long DbgProcessRequest (IN dbgProcessReq request, IN dbgSession* session,
	IN OPT void* addr, IN OUT OPT void* data, IN OPT size_t size) {

	switch(request) {
		case DBG_REQ_READ: {
			unsigned long bytesRead = 0;
			ReadProcessMemory ((HANDLE)session->process.process,(LPCVOID) addr,data,size, &bytesRead);
			if (bytesRead==0)
				DbgDisplayError("Unable to read process memory. Error code: 0x%x", GetLastError());
			return bytesRead;
		}
		case DBG_REQ_WRITE: {
			unsigned long bytesRead = 0;
			WriteProcessMemory ((HANDLE)session->process.process,(LPCVOID) addr,data,size, &bytesRead);
			if (bytesRead==0)
				DbgDisplayError("Unable to write process memory. Error code: 0x%x", GetLastError());
			return bytesRead;
		}
		case DBG_REQ_GETCONTEXT: {
			CONTEXT context;
			context.ContextFlags = CONTEXT_ALL;

			if (! GetThreadContext ((HANDLE)session->process.thread, &context))
				return FALSE;

			DbgContextFromWin32 (&context, (dbgContext*)data);
			return TRUE;
		}
		case DBG_REQ_SETCONTEXT: {
			return SetThreadContext ((HANDLE)session->process.thread, (LPCONTEXT)data);
		}
		case DBG_REQ_CONTINUE: {
			if (ResumeThread ((HANDLE)session->process.thread) == -1)
				return FALSE;
			return TRUE;
		}
		case DBG_REQ_BREAK: {
			return DebugBreakProcess ((HANDLE)session->process.process);
		}
		case DBG_REQ_STOP:
		default:
			printf ("\nDBG_REQ_STOP Not implemented");
			return 0;
	};
}

/**
*	Process session event
*
*	This service implements the OS independent API for processing equests
*	sent from the environment. Under Windows, this service would process
*	DEBUG_EVENTs when called from the operating system. Under the NDBG
*	executive debugger server, these events would be sent over a PIPE.
*
*	This method calls the environment independent session event procedure.
*
*	\param session Debug session
*	\param e Output DEBUG_EVENT descriptor
*	\ret Session state
*/
dbgSessionState DbgSessionProcessEvent (dbgSession* session, DEBUG_EVENT* e) {

	/* clear event descriptor */
	dbgEventDescr descr;
	memset (&descr,0,sizeof(dbgEventDescr));

	/* process debug event */
	switch (e->dwDebugEventCode) {
		/*
			Create process
		*/
		case CREATE_PROCESS_DEBUG_EVENT: {
			unsigned long long base = 0;

			dbgCreateProcessDescr* record;
			record      = &descr.u.createProcess;
			descr.event = DBG_EVENT_CREATEPROCESS;
			record->entry     = (vaddr_t) e->u.CreateProcessInfo.lpStartAddress;
			record->imageBase = (vaddr_t) e->u.CreateProcessInfo.lpBaseOfImage;
			record->imageName = (vaddr_t) e->u.CreateProcessInfo.lpImageName;
			return session->proc (session, &descr);
		}
		/*
			Create thread
		*/
		case CREATE_THREAD_DEBUG_EVENT: {
			dbgCreateThreadDescr* record;
			record = &descr.u.createThread;
			descr.event = DBG_EVENT_CREATETHREAD;
			record->entry = (vaddr_t) e->u.CreateThread.lpStartAddress;
			return session->proc (session, &descr);
		}
		/*
			Exception
		*/
		case EXCEPTION_DEBUG_EVENT: {
			dbgExceptionDescr* record;
			record = &descr.u.exception;
			record->firstChance = e->u.Exception.dwFirstChance;
			descr.event = DBG_EVENT_EXCEPTION;
			/*
				Converts Windows exception code to internal format
			*/
			switch (e->u.Exception.ExceptionRecord.ExceptionCode) {
				case EXCEPTION_ACCESS_VIOLATION:
					descr.u.exception.code = DBG_EXCEPTION_SEGMENT;
					break;
				case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
					descr.u.exception.code = DBG_EXCEPTION_BOUNDS;
					break;
				case EXCEPTION_BREAKPOINT:
					descr.u.exception.code = DBG_EXCEPTION_BREAKPOINT;
					break;
				case EXCEPTION_DATATYPE_MISALIGNMENT:
					descr.u.exception.code = DBG_EXCEPTION_ALIGNMENT;
					break;
				case EXCEPTION_FLT_DENORMAL_OPERAND:
					descr.u.exception.code = DBG_EXCEPTION_FLT_DENORMAL_OPERAND;
					break;
				case EXCEPTION_FLT_DIVIDE_BY_ZERO:
					descr.u.exception.code = DBG_EXCEPTION_FLT_DIVIDE;
					break;
				case EXCEPTION_FLT_INEXACT_RESULT:
					descr.u.exception.code = DBG_EXCEPTION_FLT_INEXACT_RESULT;
					break;
				case EXCEPTION_FLT_INVALID_OPERATION:
					descr.u.exception.code = DBG_EXCEPTION_FLT_INVALID_OP;
					break;
				case EXCEPTION_FLT_OVERFLOW:
					descr.u.exception.code = DBG_EXCEPTION_FLT_OVERFLOW;
					break;
				case EXCEPTION_FLT_STACK_CHECK:
					descr.u.exception.code = DBG_EXCEPTION_FLT_STACK_CHECK;
					break;
				case EXCEPTION_FLT_UNDERFLOW:
					descr.u.exception.code = DBG_EXCEPTION_FLT_UNDERFLOW;
					break;
				case EXCEPTION_ILLEGAL_INSTRUCTION:
					descr.u.exception.code = DBG_EXCEPTION_INVALID_OPCODE;
					break;
				case EXCEPTION_INT_DIVIDE_BY_ZERO:
					descr.u.exception.code = DBG_EXCEPTION_INT_DIVIDE;
					break;
				case EXCEPTION_INT_OVERFLOW:
					descr.u.exception.code = DBG_EXCEPTION_INT_OVERFLOW;
					break;
				case EXCEPTION_SINGLE_STEP:
					descr.u.exception.code = DBG_EXCEPTION_SINGLE_STEP;
					break;
				case EXCEPTION_STACK_OVERFLOW:
					descr.u.exception.code = DBG_EXCEPTION_STACK;
					break;
				case EXCEPTION_IN_PAGE_ERROR:
				case EXCEPTION_INVALID_DISPOSITION:
				case EXCEPTION_NONCONTINUABLE_EXCEPTION:
				case EXCEPTION_PRIV_INSTRUCTION:
					descr.u.exception.code = DBG_EXCEPTION_GPF;
					break;
				case DBG_CONTROL_C:
					printf ("\n\rCtrl+c event not currently implemented.");
					break;
			}
			record->address = (vaddr_t) e->u.Exception.ExceptionRecord.ExceptionAddress;
			return session->proc (session, &descr);
		}
		/*
			Exit process
		*/
		case EXIT_PROCESS_DEBUG_EVENT: {
			dbgExitProcessDescr* record;
			record = &descr.u.exitProcess;
			descr.event = DBG_EVENT_EXITPROCESS;
			record->exitCode = e->u.ExitProcess.dwExitCode;
			return session->proc (session, &descr);
		}
		/*
			Exit thread
		*/
		case EXIT_THREAD_DEBUG_EVENT: {
			dbgExitThreadDescr* record;
			record = &descr.u.exitThread;
			descr.event = DBG_EVENT_EXITTHREAD;
			record->exitCode = e->u.ExitThread.dwExitCode;
			return session->proc (session, &descr);
		}
		/*
			Output string
		*/
		case OUTPUT_DEBUG_STRING_EVENT: {
			dbgDebugStringDescr* record;		
			char* str = 0;
			dbgSessionState state = DBG_STATE_CONTINUE;

			/* create record */
			record = &descr.u.debugString;
			descr.event = DBG_EVENT_PRINT;
			record->length = e->u.DebugString.nDebugStringLength;

			/* string located in process address space so we buffer it */
			str = (char*)malloc(record->length);
			DbgProcessRequest (DBG_REQ_READ,session, e->u.DebugString.lpDebugStringData,str,record->length);
			record->string = (vaddr_t) str;
			str[record->length-1] = 0; /* 0 terminate */

			/* process event and release memory */
			state = session->proc (session, &descr);
			free(str);
			return state;
		}

		/*
			The following events are processed internally and are not passed
			to the core debugger. They are Windows specific events.
		*/

		/*
			Load DLL
		*/
		case LOAD_DLL_DEBUG_EVENT: {
			dbgSharedLibrary* libraryDescr;
			char path[64];
			memset (path,0,64);
			/*
				lpImageName is a pointer in the process address space
				that contains a pointer to the real string in that address space.
			*/
			if (e->u.LoadDll.lpImageName) {
				vaddr_t imageName = 0;
				DbgProcessRequest (DBG_REQ_READ,session, e->u.LoadDll.lpImageName,&imageName,4);
				if (imageName) {
					wchar_t image[128];
					memset (image,128,0);
					DbgProcessRequest (DBG_REQ_READ,session,(void*)imageName,image,127);
#ifdef _MSC_VER
					{
						size_t numConverted = 0;
						wcstombs_s (&numConverted, path, 64, image,63);
					}
#else
					wcstombs (path,image,63);
#endif
				}
			}

			/* there is no debug event for this since its Windows specific */
			DbgDisplayMessage ("(%i) Loaded '%s'", session->process.id.pid, path);
			break;
		}
		/*
			RIP Event
		*/
		case RIP_EVENT: {
			/* there is no debug event for this since its Windows specific */
			DbgDisplayMessage ("RIP Event");
			break;
		}
		/*
			Unload DLL
		*/
		case UNLOAD_DLL_DEBUG_EVENT: {
			/*
				Locate library object
			*/
			dbgSharedLibrary* descr = 0;
			listNode* cur = session->process.libraryList.first;
			size_t c;
			for (c = 0; c < session->process.libraryList.count; c++) {
				descr = (dbgSharedLibrary*) cur->data;
				if (descr->base == (vaddr_t) e->u.UnloadDll.lpBaseOfDll)
					break;
				cur = cur->next;
			}
			/*
				Display library and unload it
			*/
			if (cur) {
				DbgDisplayMessage("(%i) Unloaded '%s'",
					session->process.id.pid,
					descr->name);
				free (descr->name);
				listRemoveElement(c, &session->process.libraryList);
			}
			else
				DbgDisplayMessage ("(%i) Unloaded unknown DLL", session->process.id.pid);

			/* there is no debug event for this since its Windows specific */
			break;
		}
	}

	/* we just continue this session for all events that dont need processing */
	return DBG_STATE_CONTINUE;
}

/**
*	Send event to session
*	\param in Debug Session
*	\param request Session request
*	\param source Requesting source
*/
void DbgSessionSendEvent (IN dbgSession* in, IN dbgSessionEvent request, IN dbgSessionEventSource source) {
	dbgEventDescr descr;
	switch (request) {
		case DBG_SESSION_QUIT: {
			descr.event = DBG_EVENT_QUIT;
			descr.u.quitDescr.code = 0;
			descr.u.quitDescr.source = source;
			if (in->proc (in, &descr) == DBG_STATE_QUIT) {
				in->state = DBG_STATE_QUIT;
			}
			break;
		}
		case DBG_SESSION_BREAK: {
			in->state = DBG_STATE_SUSPEND;
			break;
		}
		case DBG_SESSION_CONTINUE: {
			in->state = DBG_SESSION_CONTINUE;
			break;
		}
	};
}

/**
*	Session entry point
*	\param command Command line
*	\ret Error code
*/
int __stdcall DbgSessionThreadEntry (char* command) {
	dbgSession*         session;
	DEBUG_EVENT         dbgEvent;
	PROCESS_INFORMATION process;
	STARTUPINFO         startup;

	/* start process */
	memset (&process, 0, sizeof(PROCESS_INFORMATION));
	memset (&startup, 0, sizeof(STARTUPINFO));

	if (!CreateProcess (0,command,0,0,FALSE,
		CREATE_SUSPENDED|CREATE_NEW_CONSOLE|DEBUG_PROCESS,
		0,0,&startup,&process)) {
		fprintf(stderr, "Error: Unable to create process.\n\r");
	}

	/* create debug session */
	session = DbgSessionNew (command,process.dwProcessId,process.dwThreadId,(handle_t)process.hProcess,(handle_t)process.hThread);
	if (!session) {
		fprintf(stderr, "Error: Unable to create session.\n\r");
		CloseHandle (process.hThread);
		CloseHandle (process.hProcess);
		return EXIT_FAILURE;
	}

	/* attempt to enumerate symbol information */
	if (!DbgSymbolEnumerate (session))
		DbgDisplayError ("*** Unable to load symbols");
	else
		DbgDisplayMessage ("Symbols loaded");

	/* this is the current session */
	DbgSetCurrentSession (session);

	/* session thread event loop */
	while (TRUE) {

		if (session->state == DBG_STATE_QUIT)
			break;

		if (session->state == DBG_STATE_CONTINUE) {

			/* wait for debug event from process */
	//		if (WaitForDebugEvent (&dbgEvent, INFINITE)) {
			if (WaitForDebugEvent (&dbgEvent, 1000)) {

				/* process event */
				session->state = DbgSessionProcessEvent (session, &dbgEvent);

				/* continue execution */
				ContinueDebugEvent (dbgEvent.dwProcessId,dbgEvent.dwThreadId, DBG_CONTINUE);
			}
		}
	}

	/* free session */
	DbgSessionDelete (session);

	/* free symbols */
	DbgSymbolFree (session);

	/* clear current session, release resources and return */
	if (DbgGetCurrentSession() == session)
		DbgSetCurrentSession (0);

	free (session);
	return EXIT_SUCCESS;
}

/**
*	Create session
*	\param path Command line
*/
void DbgCreateSession (char* path) {   
	if (DbgGetCurrentSession()) {
		printf ("\nAttempt to create more then one debug session");
		return;
	}
	/* create new thread for debug session */
	CreateThread (0, 0, (LPTHREAD_START_ROUTINE)DbgSessionThreadEntry, path, 0,0);
}

/* flush instruction cache. Should this be a SESSION message? */
void DbgFlushInstructionCache (dbgSession* in, vaddr_t addr, uint32_t size) {

	FlushInstructionCache(in->process.process,addr,size);
}
