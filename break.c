/********************************************
*
*	break.c - Breakpoint and Watchpoints
*
********************************************/

/*
	This component implements the breakpoint and watchpoint
	services.
*/

#include <string.h>
#include "defs.h"

/* unique ID counters */
static unsigned int _breakPointUniqueID = 0;
static unsigned int _watchPointUniqueID = 0;

BOOL DbgSetBreakpointHitCount (IN dbgSession* session, unsigned int hits) {

	listNode*      current;
	size_t         c;

	current = session->process.breakPointList.first;
	for (c=0; c<session->process.breakPointList.count; c++) {
		dbgBreakpoint* breakpoint = (dbgBreakpoint*) current->data;
		breakpoint->hits = hits;
		current = current->next;
	}
	return TRUE;
}

BOOL DbgSetBreakpointInternal (IN dbgSession* session, dbgBreakpoint* in) {

	void* addr = (void*)in->address;
	unsigned long result = 0;
	unsigned char bp = 0xcc;

	/* save byte at address and attempt to write breakpoint. */
	result = DbgProcessRequest(DBG_REQ_READ,session,addr, &in->opcode,1);
	result = DbgProcessRequest(DBG_REQ_WRITE,session,addr, (void*) &bp, 1);
	DbgFlushInstructionCache (session,addr,1);
	return TRUE;
}

BOOL DbgSetBreakpoint (IN dbgSession* session, IN vaddr_t address, dbgBreakpoingType type) {

	listNode*      current;
	dbgBreakpoint  breakpoint;
	size_t         c;

	current = session->process.breakPointList.first;
	for (c=0; c<session->process.breakPointList.count; c++) {
		dbgBreakpoint* breakpoint = (dbgBreakpoint*) current->data;
		if (breakpoint->address == address) {
			DbgDisplayMessage ("Breakpoint at [0x%x] already set", address);
			return FALSE;
		}
		current = current->next;
	}

	breakpoint.id      = _breakPointUniqueID++;
	breakpoint.address = address;
	breakpoint.once    = FALSE;
	breakpoint.opcode  = 0;
	breakpoint.set     = TRUE;
	breakpoint.type    = type;

	if (!DbgSetBreakpointInternal(session, &breakpoint)) {
		DbgDisplayError ("Unable to set breakpoint at [0x%x]", address);
		return FALSE;
	}

	if (listAddElement (&breakpoint, sizeof(dbgBreakpoint), &session->process.breakPointList)) {
		DbgDisplayMessage ("Added breakpoint at [0x%x]", address);
		return TRUE;
	}
	return FALSE;
}

BOOL DbgGetBreakpoint (IN dbgSession* session, IN vaddr_t address, OUT dbgBreakpoint* out) {
	listNode*      current;
	size_t         c;

	current = session->process.breakPointList.first;
	for (c=0; c<session->process.breakPointList.count; c++) {

		dbgBreakpoint* breakpoint = (dbgBreakpoint*) current->data;

		/* if we found the breakpoint, copy it to out and return success. */
		if (breakpoint->address == address) {
			memcpy(out,breakpoint,sizeof(dbgBreakpoint));
			return TRUE;
		}
		current = current->next;
	}

	return FALSE;
}

BOOL DbgRemoveBreakpointInternal (IN dbgSession* session, IN dbgBreakpoint* breakpoint) {

	void* addr = (void*) breakpoint->address;
	unsigned char byte = 0;

	/* save byte at address. */
	DbgProcessRequest(DBG_REQ_READ,session,addr, &byte,1);
	if (byte != 0xcc)
		return FALSE; /* this should never happen. */

	/* write back original byte and return success. */
	DbgProcessRequest(DBG_REQ_WRITE,session,addr, (void*) breakpoint->opcode,1);
	DbgFlushInstructionCache (session,addr,1);
	return TRUE;
}

BOOL DbgRemoveBreakpoint (IN dbgSession* session, IN dbgBreakpoint* breakpoint) {

	listNode*      current;
	size_t         c;

	current = session->process.breakPointList.first;
	for (c=0; c<session->process.breakPointList.count; c++) {

		dbgBreakpoint* breakpoint2 = (dbgBreakpoint*) current->data;

		/* if we found the breakpoint, remove it. */
		if (breakpoint2 == breakpoint) {
			DbgRemoveBreakpointInternal (session, breakpoint);
			DbgDisplayMessage ("Breakpoint at [0x%x] removed", breakpoint->address);
			listRemoveElement (c, &session->process.breakPointList);
			return TRUE;
		}
		current = current->next;
	}

	return FALSE;
}

BOOL DbgClearBreakpoints (IN dbgSession* session) {
	return FALSE;
}

/* not supported. */

BOOL DbgSetWatchpoint (IN dbgSession* session, IN vaddr_t address,
					   IN dbgWatchpointType type, IN unsigned long value) {
	return FALSE;
}

BOOL DbgGetWatchpoint (IN dbgSession* session, IN vaddr_t address, OUT dbgWatchpoint* out) {
	return FALSE;
}

BOOL DbgRemoveWatchpoint (IN dbgSession* session, IN dbgWatchpoint* breakpoint) {
	return FALSE;
}

BOOL DbgClearWatchpoints (IN dbgSession* session) {
	return FALSE;
}
