/********************************************
*
*	defs.h - Base definitions
*
********************************************/

#ifndef DEFS_H
#define DEFS_H

/*
	The following implements an operating system independent
	debugging interface.
*/

#include "list.h"
#include "sys.h"

#define NDBG_NAME "ndbg"
#define NDBG_PRODUCT "Neptune Debugger ("NDBG_NAME")"
#define NDBG_VERSION "0.1"
#define NDBG_VENDER "BrokenThorn Entertainment, Co"
#define NDBG_COPY "Copyright (c)"

#ifndef NDBG_CALL
#define NDBG_CALL __cdecl
#endif

#ifndef NDBG_API
#define NDBG_API __declspec(dllexport)
#endif

#ifdef _MSC_VER
#define INLINE __forceinline
#else
#define INLINE
#endif

#ifndef _WINDOWS_
#ifndef BOOL
#ifdef bool
typedef bool BOOL;
#else
typedef int BOOL;
#endif
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif

#define IN
#define OUT
#define OPT

/* process and thread IDs */
typedef unsigned int pid_t;
typedef unsigned int tid_t;

/* virtual and physical address */
typedef unsigned int  vaddr_t;
typedef unsigned int  paddr_t;

/* basic types */
typedef unsigned int  handle_t;
typedef unsigned int  size_t;
typedef unsigned int  index_t;
typedef unsigned int  pdiff_t;

/* mutual exclusion */
#define dbgMutex CRITICAL_SECTION
#define DbgMutexLock(mutex)    EnterCriticalSection(mutex)
#define DbgMutexLockTry(mutex) TryEnterCriticalSection (mutex)
#define DbgMutexUnlock(mutex)  LeaveCriticalSection(mutex)
#define DbgMutexInit(mutex)    InitializeCriticalSection(mutex)
#define DbgMutexFree(mutex)    DeleteCriticalSection(mutex)

/* command console */

typedef enum _dbgConsoleEvent {
	DBG_CON_BREAK,
	DBG_CON_CONTINUE
}dbgConsoleEvent;

/* process and thread management */

typedef struct _dbgPtid {
	pid_t pid;
	tid_t tid;
}dbgPtid;

typedef struct _dbgConfig {
	char*         path;
}dbgConfig;

typedef struct _dbgSession dbgSession;

/* session request management */

typedef enum _dbgProcessReq {
	DBG_REQ_READ,
	DBG_REQ_WRITE,
	DBG_REQ_GETCONTEXT,
	DBG_REQ_SETCONTEXT,
	DBG_REQ_CONTINUE,
	DBG_REQ_BREAK,
	DBG_REQ_STOP,
	DBG_REQ_ATTACH,
	DBG_REQ_DETATCH,
	/*
		read/write/translate physical/virtual addresses
		may not be supported by all session types
	*/
	DBG_REQ_READPHYS,
	DBG_REQ_WRITEPHYS,
	DBG_REQ_TRANSLATE	/* translates vaddr_t to paddr_t */
}dbgProcessReq;

/* exception management */

typedef enum _dbgException {
	/*
		x86 CPU exceptions
	*/
	DBG_EXCEPTION_INT_DIVIDE,
	DBG_EXCEPTION_SINGLE_STEP,
	DBG_EXCEPTION_NMI,
	DBG_EXCEPTION_BREAKPOINT,
	DBG_EXCEPTION_INT_OVERFLOW,
	DBG_EXCEPTION_BOUNDS,
	DBG_EXCEPTION_INVALID_OPCODE,
	DBG_EXCEPTION_NO_COPROCESSOR,
	DBG_EXCEPTION_DOUBLE_FAULT,
	DBG_EXCEPTION_INVALID_TSS,
	DBG_EXCEPTION_SEGMENT,
	DBG_EXCEPTION_STACK,
	DBG_EXCEPTION_GPF,
	DBG_EXCEPTION_PAGE_FAULT,
	DBG_EXCEPTION_COPROCESSOR,
	DBG_EXCEPTION_ALIGNMENT,
	/*
		x87 FPU exceptions
	*/
	DBG_EXCEPTION_FLT_DIVIDE,
	DBG_EXCEPTION_FLT_OVERFLOW,
	DBG_EXCEPTION_FLT_INEXACT_RESULT,
	DBG_EXCEPTION_FLT_INVALID_OP,
	DBG_EXCEPTION_FLT_STACK_CHECK,
	DBG_EXCEPTION_FLT_DENORMAL_OPERAND,
	DBG_EXCEPTION_FLT_UNDERFLOW
}dbgException;

typedef enum dbgExceptionType {
	DBG_EXCEPTION_TYPE_UNKNOWN = 0,
	DBG_EXCEPTION_TYPE_FAULT,
	DBG_EXCEPTION_TYPE_TRAP,
	DBG_EXCEPTION_TYPE_ABORT
}dbgExceptionType;

typedef struct _dbgExceptionDescr {
	int              firstChance;
	dbgException     code;
	dbgExceptionType type;
	vaddr_t          address;
}dbgExceptionDescr;

typedef struct _dbgCreateThreadDescr {
	vaddr_t      entry;
}dbgCreateThreadDescr;

typedef struct _dbgExitThreadDescr {
	int          exitCode;
}dbgExitThreadDescr;

typedef struct _dbgExitProcessDescr {
	int          exitCode;
}dbgExitProcessDescr;

typedef struct _dbgCreateProcessDescr {
	vaddr_t      imageBase;
	vaddr_t      entry;
	vaddr_t      imageName;
}dbgCreateProcessDescr;

typedef struct _dbgDebugStringDescr {
	vaddr_t      string;
	unsigned int length;
}dbgDebugStringDescr;

typedef struct _dbgQuitDescr {
	unsigned int code;
	unsigned int source;
}dbgQuitDescr;

typedef enum _dbgEvent {
	DBG_EVENT_EXCEPTION,
	DBG_EVENT_CREATETHREAD,
	DBG_EVENT_EXITTHREAD,
	DBG_EVENT_CREATEPROCESS,
	DBG_EVENT_EXITPROCESS,
	DBG_EVENT_PRINT,
	DBG_EVENT_QUIT
}dbgEvent;

typedef struct _dbgEventDescr {
	dbgEvent event;
	union {
		dbgExceptionDescr     exception;
		dbgCreateThreadDescr  createThread;
		dbgExitThreadDescr    exitThread;
		dbgCreateProcessDescr createProcess;
		dbgExitProcessDescr   exitProcess;
		dbgDebugStringDescr   debugString;
		dbgQuitDescr          quitDescr;
	}u;
}dbgEventDescr;

/* source file management */

typedef struct _dbgSourceLine {
	vaddr_t      modbase;
	char*        objectFile;
	char*        fname;
	unsigned int lineNumber;
	vaddr_t      addr;
}dbgSourceLine;

typedef struct _dbgSourceFile {
	vaddr_t  modbase;
	char*    name;
	list     sourceLineList;
}dbgSourceFile;

/* symbol information */

typedef enum _dbgSymbolType {
	DBG_SYM_FORWARDER = 1,
	DBG_SYM_SLOT      = 0x10,
	DBG_SYM_THUNK     = 0x100,
	DBG_SYM_CLR_TOKEN = 0x1000,
	DBG_SYM_CONSTANT  = 0x10000,
	DBG_SYM_TLSREL    = 0x100000
}dbgSymbolType;

typedef enum _dbgSymbolSrc {
	DBG_SYM_EXPORT    = 0x1,
	DBG_SYM_FUNCTION  = 0x10,
	DBG_SYM_LOCAL     = 0x100,
	DBG_SYM_METADATA  = 0x1000,
	DBG_SYM_PARAMETER = 0x10000,
	DBG_SYM_REGISTER  = 0x100000
}dbgSymbolSrc;

typedef enum _dbgSymbolFlag {
	DBG_SYM_FRAMEREL  = 0x1,
	DBG_SYM_IREL      = 0x10,
	DBG_SYM_REGREL    = 0x100
}dbgSymbolFlag;

typedef struct _dbgSymbol {
	vaddr_t      modbase;
	unsigned int type;
	unsigned int src;
	unsigned int flags;
	unsigned long long value;			// constants only
	vaddr_t      addr;
	int          reg;
	char*        name;
}dbgSymbol;

/* break points */

typedef enum _dbgBreakpointType {
	DBG_BREAK_HARD,
	DBG_BREAK_SOFT
}dbgBreakpoingType;

typedef struct _dbgBreakpoint {
	unsigned int      id;
	BOOL              set;
	BOOL              once;	   /* 1 time breakpoint. When hit, it should be removed */
	unsigned char     opcode;
	vaddr_t           address;
	dbgBreakpoingType type;
	unsigned int      hits;
}dbgBreakpoint;

/* watch points */

typedef enum dbgWatchpointType {
	DBG_WATCH_READ,
	DBG_WATCH_WRITE,
	DBG_WATCH_ACCESS
}dbgWatchpointType;

typedef struct _dbgWatchpoint {
	unsigned int      id;
	BOOL              set;
	vaddr_t           address;
	size_t            length;
	unsigned long     value;
	dbgWatchpointType type;
}dbgWatchpoint;

/* session management */

typedef enum _dbgSessionEventSource {
	DBG_SOURCE_COMMAND,
	DBG_SOURCE_SESSION,
	DBG_SOURCE_SYMBOL
}dbgSessionEventSource;

typedef enum _dbgSessionEvent {
	DBG_SESSION_BREAK,
	DBG_SESSION_CONTINUE,
	DBG_SESSION_QUIT
}dbgSessionEvent;

typedef enum _dbgSessionState {
	DBG_STATE_CONTINUE,
	DBG_STATE_SUSPEND,
	DBG_STATE_QUIT,
	DBG_STATE_IGNORED
}dbgSessionState;

typedef struct _dbgSharedLibrary {
	char*   name;
	vaddr_t base;
}dbgSharedLibrary;

typedef struct _dbgThread {
	handle_t thread;
	vaddr_t  entry;
/*	void*    threadLocalBase; */
}dbgThread;

typedef struct _dbgProcess {
	char*    name;
	vaddr_t  base;
	handle_t process;
	handle_t thread;
	dbgPtid  id;
	list     libraryList;
	list     threadList;
	list     sourceFileList;
	list     breakPointList;
	list     watchPointList;
}dbgProcess;

/* debug event callback */
typedef dbgSessionState (*DbgSessionEventProc) (IN dbgSession* session, IN dbgEventDescr* descr);

typedef struct _dbgSession {
	dbgSessionState     state;
	dbgProcess          process;
	DbgSessionEventProc proc;
}dbgSession;

/*
	main.c
	Main program services
*/
extern void DbgDisplayMessage (const char* msg, ...);
extern void DbgDisplayError   (const char* msg, ...);

/*
	session.c
	Implements session manager
*/
extern unsigned long DbgProcessRequest     (IN dbgProcessReq request, IN dbgSession* session,
                                            IN OPT void* addr, IN OUT OPT void* data, IN OPT size_t size) ;
extern dbgSession* DbgGetCurrentSession     (void);
extern void        DbgSetCurrentSession     (IN dbgSession* session);
extern void        DbgCreateSession         (IN char* path);
extern void        DbgRegisterEventProc     (IN dbgSession* session, IN DbgSessionEventProc proc);
extern char*       DbgSessionGetProcessName (IN dbgSession* session);
extern dbgPtid*    DbgSessionGetPtid        (IN dbgSession* session);
extern void        DbgSessionSendEvent      (IN dbgSession* in, IN dbgSessionEvent request,
                                             IN dbgSessionEventSource source);
extern void DbgFlushInstructionCache (dbgSession* in, vaddr_t addr, uint32_t size);

/*
	symbol.c
	Implements symbol manager. Should ONLY be used by session manager or debug core
*/
extern BOOL DbgSymbolFromName    (IN dbgSession* in, IN const char* name, OUT dbgSymbol* symbol);
extern BOOL DbgSymbolFromAddress (IN dbgSession* in, IN vaddr_t address,  OUT dbgSymbol* symbol);
extern BOOL DbgSymbolEnumerate   (IN dbgSession* in);
extern BOOL DbgSymbolFree        (IN dbgSession* in);

/*
	cmd.c
	Implements command console entry point
*/
extern int  DbgConsoleEntry     (void);
extern void DbgConsoleSendEvent (dbgConsoleEvent code);

/*
	pdb.c
	Implements PDB API. Should ONLY be used by session or symbol manager
*/
extern BOOL DbgInitializePDB                    (IN dbgProcess* proc);
extern void DbgFreePDB                          (IN dbgProcess* proc);
extern BOOL DbgLoadSymbolsPDB                   (IN dbgProcess* proc);
extern BOOL DbgSymbolFromNamePDB                (IN const char* name, OUT dbgSymbol* sym);
extern BOOL DbgSymbolFromAddressPDB             (IN vaddr_t address,  OUT dbgSymbol* sym);
extern unsigned long long DbgLoadSymbolTablePDB (IN char* name, IN vaddr_t base);

/*
	dbg.c
	Implement core Neptune Debugger API
*/
extern BOOL DbgInitialize                       (IN dbgSession* session);
extern BOOL DbgContinue                         (IN dbgSession* session);
extern BOOL DbgGetRegisters                     (IN dbgSession* session, OUT dbgContext* context);
extern BOOL DbgSetRegisters                     (IN dbgSession* session, OUT dbgContext* context);
extern BOOL DbgSingleStep                       (IN dbgSession* session);
extern BOOL DbgStepIn                           (IN dbgSession* session);
extern BOOL DbgStepOver                         (IN dbgSession* session);
extern BOOL DbgStepOut                          (IN dbgSession* session);
extern BOOL DbgContinueUntil                    (IN dbgSession* session, IN vaddr_t address);
extern BOOL DbgSetNext                          (IN dbgSession* session, IN vaddr_t address);

/*
	break.c
	Breakpoint and watchpoint management
*/
extern BOOL DbgSetBreakpoint                    (IN dbgSession* session, IN vaddr_t address, dbgBreakpoingType type);
extern BOOL DbgGetBreakpoint                    (IN dbgSession* session, IN vaddr_t address, OUT dbgBreakpoint* out);
extern BOOL DbgRemoveBreakpoint                 (IN dbgSession* session, IN dbgBreakpoint* breakpoint);
extern BOOL DbgClearBreakpoints                 (IN dbgSession* session);
extern BOOL DbgSetWatchpoint                    (IN dbgSession* session, IN vaddr_t address,
												 IN dbgWatchpointType type, IN unsigned long value);
extern BOOL DbgGetWatchpoint                    (IN dbgSession* session, IN vaddr_t address, OUT dbgWatchpoint* out);
extern BOOL DbgRemoveWatchpoint                 (IN dbgSession* session, IN dbgWatchpoint* breakpoint);
extern BOOL DbgClearWatchpoints                 (IN dbgSession* session);

#endif
