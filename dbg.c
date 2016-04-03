/********************************************

	dbg.c - Debugger front end.

********************************************/

#include "defs.h"

/**
*	Process exception
*	\param session Debug session
*	\param descr Exception descriptor
*/
int DbgProcessException (IN dbgSession* session, IN dbgExceptionDescr* descr) {

	/* display exception type */
	if (descr->firstChance)
		DbgDisplayError ("First chance exception (0x%x) at (0x%x)", descr->code, descr->address);
	else
		DbgDisplayError ("Second chance exception (0x%x) at (0x%x)", descr->code, descr->address);

	/* tell session manager to halt which in turn enables the console */
	DbgSessionSendEvent (session, DBG_SESSION_BREAK, DBG_SOURCE_COMMAND);
	return 0;
}

/**
*	Process event
*	\param session Debug session
*	\param descr Event descriptor
*/
dbgSessionState DbgEventProc (IN dbgSession* session, IN dbgEventDescr* descr) {
	switch (descr->event) {
		case DBG_EVENT_QUIT: {
			DbgDisplayError ("QUIT Command recieved; quitting...");
			return DBG_STATE_QUIT;
		}
		case DBG_EVENT_EXITTHREAD: {
			DbgDisplayMessage ("Thread '%s' (%i) terminated with exit code %i (0x%x)",
				"Win32 Thread", DbgSessionGetPtid(session)->tid,
				descr->u.exitThread.exitCode, descr->u.exitThread.exitCode);
			break;
		}
		case DBG_EVENT_EXITPROCESS: {
			DbgDisplayMessage ("Process '%s' (%i) terminated with exit code %i (0x%x)",
				DbgSessionGetProcessName(session), DbgSessionGetPtid(session)->pid,
				descr->u.exitProcess.exitCode, descr->u.exitProcess.exitCode);
			break;
		}
		case DBG_EVENT_EXCEPTION: {
			DbgProcessException (session, &descr->u.exception);
			return DBG_STATE_SUSPEND;
		}
		case DBG_EVENT_PRINT: {
			DbgDisplayDebugOut ("%s", descr->u.debugString.string);
			break;
		}
		case DBG_EVENT_CREATEPROCESS:
		case DBG_EVENT_CREATETHREAD:
		default:
			break;
	}
	return DBG_STATE_CONTINUE;
}

/*
	The following implement the core debugger services
*/

/**
*	Implements CONTINUE debug command
*	\param session Debug session
*	\ret TRUE if success, FALSE otherwise
*/
BOOL DbgContinue (IN dbgSession* session) {
	if (!session)
		return FALSE;
	/* call session manager to initiate request */
	if (DbgProcessRequest (DBG_REQ_CONTINUE, session, 0, 0, 0) == FALSE)
		return FALSE;
	return TRUE;
}

/**
*	Implements INFO REGISTER debug command
*	\param session Debug session
*	\param context OUT context
*	\ret TRUE if success, FALSE otherwise
*/
BOOL DbgGetRegisters (IN dbgSession* session, OUT dbgContext* context) {
	if (!session)
		return FALSE;
	if (!DbgProcessRequest (DBG_REQ_GETCONTEXT, session,0, context,sizeof(dbgContext)))
		return FALSE;
	return TRUE;
}

BOOL DbgSetRegisters (IN dbgSession* session, OUT dbgContext* context) {
	return FALSE;
}

BOOL DbgSingleStep (IN dbgSession* session) {
	return FALSE;
}

BOOL DbgStepIn (IN dbgSession* session) {
	return FALSE;
}

BOOL DbgStepOver (IN dbgSession* session) {
	return FALSE;
}

BOOL DbgStepOut (IN dbgSession* session) {
	return FALSE;
}

BOOL DbgContinueUntil (IN dbgSession* session, IN vaddr_t address) {
	return FALSE;
}

BOOL DbgSetNext (IN dbgSession* session, IN vaddr_t address) {
	return FALSE;
}

/**
*	Initialize debugger component
*/
BOOL DbgInitialize (IN dbgSession* session) {
	DbgRegisterEventProc (session, DbgEventProc);
	return TRUE;
}
