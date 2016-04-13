/********************************************
*
*	pdb.c - Program DataBase (PDB) Support
*
********************************************/

/*
	This component programs services for PDB files.
*/

#include <stdlib.h>
#include <string.h>
#include "defs.h"

#ifdef _WIN32
/*
	Due to the proprietary format of the file, we
	use the provided system library to parse it.
*/
#pragma comment(lib, "dbghelp.lib")

/* note: without this we will get name conflict with handle_t */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <dbghelp.h>

/*

	The following functions convert PDB specific structures to their
	NDBG counterparts. These functions provide the back-end of the PDB API.

*/

/**
*	Converts a PDB source line descriptor to an NDBG descriptor
*	\param pdb PDB descriptor
*	\param sym NDBG descriptor
*	\ret TRUE is success, FALSE otherwise
*/
BOOL DbgSourceCodeFromPDB (IN PSRCCODEINFO LineInfo, OUT dbgSourceLine* out) {

	return FALSE;
}

/**
*	Converts a PDB source file descriptor to an NDBG descriptor
*	\param pdb PDB descriptor
*	\param sym NDBG descriptor
*	\ret TRUE is success, FALSE otherwise
*/
BOOL DbgSourceFileFromPDB (IN PSOURCEFILE pSourceFile, OUT dbgSourceFile* out) {
	if (!out)
		return FALSE;
	if (!pSourceFile)
		return FALSE;

	out->modbase = (vaddr_t) pSourceFile->ModBase;
	out->name    = (char*) malloc (strlen(pSourceFile->FileName)+1);
#ifdef _MSC_VER
	strcpy_s(out->name,strlen(pSourceFile->FileName)+1,pSourceFile->FileName);
#else
	strcpy(out->name,pSourceFile->FileName);
#endif
	listInit (&out->sourceLineList);
	return TRUE;
}

/**
*	Converts a PDB symbol descriptor to an NDBG descriptor
*	\param pdb PDB descriptor
*	\param sym NDBG descriptor
*	\ret TRUE is success, FALSE otherwise
*/
BOOL DbgSymbolFromPDB (IN SYMBOL_INFO* pdb, OUT dbgSymbol* sym) {

	sym->name    = pdb->Name;
	sym->addr    = (vaddr_t) pdb->Address;
	sym->modbase = (vaddr_t) pdb->ModBase;
	sym->value   = pdb->Value;
	sym->flags   = 0;
	sym->type    = 0;
	sym->reg     = 0;
	sym->src     = 0;

	if ((pdb->Flags & SYMFLAG_CLR_TOKEN)==SYMFLAG_CLR_TOKEN)    sym->type |= DBG_SYM_CLR_TOKEN;
	if ((pdb->Flags & SYMFLAG_CONSTANT)==SYMFLAG_CONSTANT)      sym->type |= DBG_SYM_CONSTANT;
	if ((pdb->Flags & SYMFLAG_FORWARDER)==SYMFLAG_FORWARDER)    sym->type |= DBG_SYM_FORWARDER;
	if ((pdb->Flags & SYMFLAG_SLOT )==SYMFLAG_SLOT )            sym->type |= DBG_SYM_SLOT;
	if ((pdb->Flags & SYMFLAG_THUNK)==SYMFLAG_THUNK)            sym->type |= DBG_SYM_THUNK;
	if ((pdb->Flags & SYMFLAG_TLSREL)==SYMFLAG_TLSREL)          sym->type |= DBG_SYM_TLSREL;

	if ((pdb->Flags & SYMFLAG_EXPORT)==SYMFLAG_EXPORT)          sym->src |= DBG_SYM_EXPORT;
	if ((pdb->Flags & SYMFLAG_FUNCTION)==SYMFLAG_FUNCTION)      sym->src |= DBG_SYM_FUNCTION;
	if ((pdb->Flags & SYMFLAG_LOCAL)==SYMFLAG_LOCAL)            sym->src |= DBG_SYM_LOCAL;
	if ((pdb->Flags & SYMFLAG_METADATA)==SYMFLAG_METADATA)      sym->src |= DBG_SYM_METADATA;
	if ((pdb->Flags & SYMFLAG_PARAMETER)==SYMFLAG_PARAMETER)    sym->src |= DBG_SYM_PARAMETER;
	if ((pdb->Flags & SYMFLAG_REGISTER)==SYMFLAG_REGISTER)      sym->src |= DBG_SYM_REGISTER;

	if ((pdb->Flags & SYMFLAG_FRAMEREL)==SYMFLAG_FRAMEREL)      sym->flags |= DBG_SYM_FRAMEREL;
	if ((pdb->Flags & SYMFLAG_ILREL)==SYMFLAG_ILREL)            sym->flags |= DBG_SYM_IREL;
	if ((pdb->Flags & SYMFLAG_REGREL)==SYMFLAG_REGREL)          sym->flags |= DBG_SYM_REGREL;

	return TRUE;
}

/*

	The following are callback procedures called by the PDB library.
	These are all internal and are used during initialization
	to initialize the NDBG session lists.

*/

/**
*	Source line enumeration callback
*	\param LineInfo PDB source line descriptor
*	\param UserContext Valid NDBG source file descriptor for this line
*	\ret TRUE if success, FALSE otherwise
*/
BOOL CALLBACK EnumLinesProcPDB (PSRCCODEINFO LineInfo, PVOID UserContext ) {
	dbgSourceFile* sourceFile;
	dbgSourceLine  sourceLine;
	/*
		Initialize source line
	*/
	sourceLine.modbase    = (vaddr_t) LineInfo->ModBase;
	sourceLine.lineNumber = LineInfo->LineNumber;
	sourceLine.addr       = (vaddr_t)LineInfo->Address;
	sourceLine.objectFile = 0;
	sourceLine.fname      = 0;
	/*
		Add source line to file source line list
	*/
	sourceFile = (dbgSourceFile*) UserContext;
	if (!sourceFile) {
		return FALSE;
	}
	listAddElement (&sourceLine, sizeof(sourceLine), &sourceFile->sourceLineList);
	return TRUE;
}

/**
*	Source file enumeration callback
*	\param pSourceFile PDB source file descriptor
*	\param UserContext dbgProcess descriptor
*	\ret TRUE if success, FALSE otherwise
*/
BOOL CALLBACK EnumSourceFilesProcPDB (PSOURCEFILE pSourceFile,PVOID UserContext) {
	dbgProcess* proc;
	dbgSourceFile sourceFile;
	/*
		Add source file to list in process descriptor
	*/
	DbgSourceFileFromPDB (pSourceFile, &sourceFile);
	listInit (&sourceFile.sourceLineList);
	proc = (dbgProcess*)UserContext;
	listAddElement (&sourceFile, sizeof(dbgSourceFile), &proc->sourceFileList);
	return TRUE;
}

/**
*	Symbol enumeration callback
*	\param pSourceFile PDB symbol descriptor
*	\param UserContext
*	\ret TRUE if success, FALSE otherwise
*/
BOOL CALLBACK EnumSymbolsProcPDB (PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {
	dbgSymbol out;
	dbgSymbol out2;
	vaddr_t addr;

	/*
		FIXME : Need to implement symbol table.
	*/

//	addr =  (vaddr_t) pSymInfo->Address;
//	DbgDisplayMessage("Symbol %s at 0x%x", pSymInfo->Name, pSymInfo->Address);
//	DbgSymbolFromAddress(0, addr, &out);
//	DbgSymbolFromName (0, pSymInfo->Name, &out2);

	return 1;
}

BOOL CALLBACK ReadProcessMemoryProc(HANDLE hProcess,DWORD64 lpBaseAddress,
	PVOID lpBuffer,DWORD nSize, LPDWORD lpNumberOfBytesRead) {
	return 0;
}

PVOID CALLBACK FunctionTableAccessProc(HANDLE hProcess,DWORD64 AddrBase) {
	return 0;
}

DWORD WINAPI GetModuleBaseProc(HANDLE hProcess,DWORD dwAddr) {
	return 0;
}

DWORD CALLBACK TranslateAddressProc (HANDLE hProcess,HANDLE hThread,LPADDRESS lpaddr) {
	return 0;
}

/*

	The following functions provide the core API methods that can be called by
	the NDBG session manager or symbol manager.

*/

/**
*	Loads symbols
*	\param proc NDBG process descriptor
*/
BOOL DbgLoadSymbolsPDB (dbgProcess* proc) {
	IMAGEHLP_MODULE mod;
	listNode*       current;
	size_t          c;
	/*
		Get module information
	*/
	memset(&mod,0,sizeof(IMAGEHLP_MODULE));
	mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
	if (! SymGetModuleInfo(GetCurrentProcess(),proc->base,&mod)) {
		DbgDisplayError ("Unable to get module info : %x", GetLastError());
		return FALSE;
	}
	/*
		Load symbols, currently we only implement PDB support
	*/
	switch (mod.SymType) {
		case SymNone: 
			DbgDisplayMessage( "No symbols available for the module.\n"); 
			return FALSE;
		case SymExport: 
			break; 
		case SymCoff:  
			break; 
		case SymCv: 
			break; 
		case SymSym: 
			break; 
		case SymVirtual: 
			break;
		case SymPdb:

			SymEnumSourceFiles (GetCurrentProcess(), proc->base, 0,    EnumSourceFilesProcPDB, proc);
			SymEnumSymbols     (GetCurrentProcess(), proc->base, 0,    EnumSymbolsProcPDB,     proc);
			SymEnumTypes       (GetCurrentProcess(), proc->base,       EnumSymbolsProcPDB,     proc);

			current = proc->sourceFileList.first;
			for (c = 0;c < proc->sourceFileList.count; c++) {
				dbgSourceFile* currentFile = (dbgSourceFile*) current->data;
				SymEnumLines (GetCurrentProcess(), proc->base, 0, currentFile->name, EnumLinesProcPDB, currentFile);
				current = current->next;
			}

			break; 
		case SymDia: 
			break; 
		case SymDeferred: 
			break; 
		default: 
			DbgDisplayError("Loaded symbols: Unknown format.\n"); 
			return FALSE;
	}
	return TRUE;
}

/**
* Return symbol information from symbol address
* \param address Address of symbol
* \param sym Output symbol descriptor
* \ret TRUE if success, FAIL otherwise
*/
BOOL DbgSymbolFromAddressPDB (IN vaddr_t address, OUT dbgSymbol* sym) {
	SYMBOL_INFO *pSymbol;
	DWORD res;
	/*
		Initialize PDB symbol descriptor
	*/
	pSymbol = (SYMBOL_INFO *)malloc (sizeof(SYMBOL_INFO )+MAX_SYM_NAME);
	memset(pSymbol,0,sizeof(SYMBOL_INFO )+MAX_SYM_NAME);
	pSymbol->SizeOfStruct= sizeof(SYMBOL_INFO );
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	/*
		Lookup symbol from address
	*/
	res = SymFromAddr(GetCurrentProcess(),address,0,pSymbol);
	if (!res) {
		free (pSymbol);
		DbgDisplayError("Unable to load symbol at '0x%x'", address);
		return FALSE;
	}
	/*
		Convert PDB descriptor to NDBG descriptor
	*/
	res = DbgSymbolFromPDB(pSymbol, sym);
	free(pSymbol);
	if (!res)
		return FALSE;
	return TRUE;
}

/**
* Return symbol information from symbol name
* \param address Name of symbol
* \param sym Output symbol descriptor
* \ret TRUE if success, FAIL otherwise
*/
BOOL DbgSymbolFromNamePDB (IN const char* name, OUT dbgSymbol* sym) {
	SYMBOL_INFO *pSymbol;
	DWORD res;
	/*
		Initialize PDB symbol descriptor
	*/
	pSymbol = (SYMBOL_INFO *)malloc (sizeof(SYMBOL_INFO )+MAX_SYM_NAME);
	if (!pSymbol)
		return FALSE;
	memset(pSymbol,0,sizeof(SYMBOL_INFO )+MAX_SYM_NAME);
	pSymbol->SizeOfStruct= sizeof(SYMBOL_INFO );
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	/*
		Lookup symbol by name
	*/
	res = SymFromName(GetCurrentProcess(),name,pSymbol);
	if (!res) {
		DWORD error = GetLastError();
		free (pSymbol);
		DbgDisplayError("Unable to load symbol '%s' : 0x%x", name, error);
		return FALSE;
	}
	/*
		Convert PDB descriptor to NDBG descriptor
	*/
	res = DbgSymbolFromPDB(pSymbol, sym);
	free(pSymbol);
	if (!res)
		return FALSE;
	return TRUE;
}

/**
*	Initialize symbol handler
*	\param proc NDBG process descriptor
*	\ret TRUE if success, FALSE otherwise
*/
BOOL DbgInitializePDB (dbgProcess* proc) {
	DWORD options;
	/*
		Set options
	*/
	options = SymGetOptions();
	options |= SYMOPT_DEBUG;
	SymSetOptions(options);
	/*
		Initialize dbgHelp.dll and load symbols
	*/
	return SymInitialize (GetCurrentProcess(), 0 ,FALSE);
}

/**
*	Free symbol handler
*	\param proc NDBG process descriptor
*/
void DbgFreePDB (dbgProcess* proc) {
	SymCleanup((HANDLE) proc->process);
}

/**
*	Load symbol table
*	\param module Handle to module
*	\param name Module name
*	\param base Base address
*	\ret Base address of loaded module or 0 on error
*/
unsigned long long DbgLoadSymbolTablePDB (char* name, vaddr_t base) {
	return SymLoadModule (GetCurrentProcess(), 0, name, 0, base, 0);
}

#endif
