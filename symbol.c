/********************************************
*
*	symbol.c - Symbol tables
*
********************************************/

/*
	This component implements the symbol table API.
*/

#include "defs.h"

/*
	NDBG Symbol services
*/

BOOL DbgSymbolFromName (IN dbgSession* in, IN const char* name, OUT dbgSymbol* symbol) {
	return DbgSymbolFromNamePDB (name, symbol);
}

BOOL DbgSymbolFromAddress (IN dbgSession* in, IN vaddr_t address, OUT dbgSymbol* symbol) {
	return DbgSymbolFromAddressPDB (address, symbol);
}

BOOL DbgSymbolEnumerate (IN dbgSession* in) {
	vaddr_t modbase;

	DbgInitializePDB (&in->process);
	DbgDisplayMessage("Loading symbols for : %s", in->process.name);

	modbase = (vaddr_t) DbgLoadSymbolTablePDB (in->process.name, 0);
	in->process.base = (vaddr_t) modbase;

	if (modbase) {
		return DbgLoadSymbolsPDB (&in->process);
	}
	return FALSE;
}

BOOL DbgSymbolFree (IN dbgSession* in) {
	if (!in)
		return FALSE;
	DbgFreePDB (&in->process);
	return TRUE;
}
