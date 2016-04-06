/********************************************
*
*	cmd.c - Command line console
*
********************************************/

/*
	This component implements the command line console.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include "defs.h"
#include "sys.h"

typedef struct _dbgConsole {
	char currentLine[32];
}dbgConsole;
dbgConsole _console;

typedef int (*DbgCommandProc) (int argc, char* argv[]);

typedef struct _dbgCommand {
	char* cmd;
	char* descr;
	DbgCommandProc proc;
}dbgCommand;
static dbgCommand _commandList[32];

NDBG_API dbgCommand* NDBG_CALL DbgConsoleGet (char* name) {
	int i;
	for (i = 0; i<32; i++) {
		if (!_commandList[i].cmd)
			continue;
		if (strcmp (_commandList[i].cmd,name)==0)
			return &_commandList[i];
	}
	return 0;
}

NDBG_API void NDBG_CALL DbgConsoleRegister (char* cmd, char* descr, DbgCommandProc proc) {
	static int current = 0;
	assert (current < 32);
	if (DbgConsoleGet (cmd)) {
		printf ("\n\rDbgConsoleRegister: double register");
		exit (0);
	}
	_commandList [current].cmd = cmd;
	_commandList [current].descr = descr;
	_commandList [current].proc = proc;
	current++;
}

NDBG_API size_t NDBG_CALL DbgConsoleGetArgs (IN char* line, IN char** argv, IN size_t count) {
	int argc = 0;
	char* p2 = strtok(line, " \n");
	while (p2 && argc < count) {
		argv[argc++] = p2;
		p2 = strtok(0, " \n");
	}
	return argc;
}

NDBG_API int NDBG_CALL DbgConsoleDefault (IN int argc, IN char* argv[]) {
	if (argc > 0) {
		printf ("\nUnknown command '%s', ignored.", argv[0]);
	}
	return 0;
}

int DbgConsoleClear (IN int argc, IN char* argv[]) {
	system ("cls");
	return 0;
}

int DbgConsoleHelp (IN int argc, IN char** argv) {
	int c=0;
	printf (NDBG_PRODUCT" "NDBG_VERSION"\n"NDBG_COPY" "NDBG_VENDER);
	printf ("\n\nThe "NDBG_PRODUCT" is a kernel and user mode debugger for the Neptune Software Suite.");

	printf ("\n\nCommand\t| Description\n");
	printf ("------------------------------\n");
	for (c=0; c<32; c++) {
		if (_commandList[c].cmd) {
			printf ("\n%s", _commandList[c].cmd);
			printf ("\t| %s", _commandList[c].descr ? _commandList[c].descr : "<invalid>");
		}
	}
	printf ("\n\nType \"exit\" to close the debugger.\n");
	printf ("Some commands may support '-?' or '-h' operands for further information.\n");
	return 0;
}

/**
*	Implements console CONTINUE command
*	\param argc Argument count
*	\param argv Argument list
*	\ret TRUE if success, FALSE on error
*/
BOOL DbgConsoleContinue (IN int argc, IN char** argv) {
	dbgSession* session = DbgGetCurrentSession ();
	/* call debugger API */
	if (!DbgContinue (session))
		return FALSE;
	session->state = DBG_STATE_CONTINUE;
	return TRUE;
}

void DbgDisplayContext (dbgContext* context) {
	printf ("EAX : 0x%x\n", context->regs.eax);
	printf ("EBX : 0x%x\n", context->regs.ebx);
	printf ("ECX : 0x%x\n", context->regs.ecx);
	printf ("EDX : 0x%x\n", context->regs.edx);
	printf ("ESI : 0x%x\n", context->regs.esi);
	printf ("EDI : 0x%x\n", context->regs.edi);
	printf ("EBP : 0x%x\n", context->regs.ebp);
	printf ("ESP : 0x%x\n", context->regs.esp);
	printf ("CS : 0x%x\n", context->sregs.cs);
	printf ("DS : 0x%x\n", context->sregs.ds);
	printf ("ES : 0x%x\n", context->sregs.es);
	printf ("SS : 0x%x\n", context->sregs.ss);
	printf ("FS : 0x%x\n", context->sregs.fs);
	printf ("GS : 0x%x\n", context->sregs.gs);
	printf ("IP : 0x%x\n", context->eip);
	printf ("FLAGS : 0x%x\n", context->flags);
}

BOOL DbgConsoleSetBreakpoint (IN int argc, IN char** argv) {
	vaddr_t address;

	if (argc!=2) {
		DbgDisplayError ("Syntax : b [address]");
		return FALSE;
	}

	address = (vaddr_t) strtol (argv[1], 0, 10);	
	if (!address)
		address = (vaddr_t) strtol (argv[1], 0, 16);	
	return DbgSetBreakpoint (DbgGetCurrentSession(), address, DBG_BREAK_SOFT);
}

BOOL DbgConsoleRegisters (IN int argc, IN char** argv) {
	dbgContext context;
	if (! DbgGetRegisters (DbgGetCurrentSession(), &context)) {
		DbgDisplayError ("Unable to obtain context");
		return FALSE;
	}
	DbgDisplayContext (&context);
	return TRUE;
}

BOOL DbgConsoleSingleStep (IN int argc, IN char** argv) {
	return FALSE;
}

void DbgConsoleInterrupt (int sig) {
	printf ("\nctrl+c triggered");
//	signal (sig, SIG_IGN);
}

void DbgConsoleInit (void) {

	/* general commands */
	DbgConsoleRegister ("help",   "Display help information", DbgConsoleHelp);
	DbgConsoleRegister ("clear",  "Clear display",            DbgConsoleClear);
	DbgConsoleRegister ("ext",    "Extension library",        0);
	DbgConsoleRegister ("version","Version information",      0);

	/* session commands */
	DbgConsoleRegister ("attach","Attach session to process",   0);
	DbgConsoleRegister ("detach","Detach session from process", 0);
	DbgConsoleRegister ("q", "Quit", 0);
	DbgConsoleRegister ("restart", "Restart session", 0);

	/* execution control */
	DbgConsoleRegister ("c", "Continue",     DbgConsoleContinue);
	DbgConsoleRegister ("s", "Single step",  DbgConsoleSingleStep);

	/* breakpoints */
	DbgConsoleRegister ("b",     "Set breakpoint",     DbgConsoleSetBreakpoint);
	DbgConsoleRegister ("be",    "Breakpoint enable",  0);
	DbgConsoleRegister ("bd",    "Breakpoint disable", 0);
	DbgConsoleRegister ("bc",    "Breakpoint clear",   0);

	/* trace enable */
	DbgConsoleRegister ("t",    "Trace",  0);

	DbgConsoleRegister ("r",     "Display registers", DbgConsoleRegisters);
//	DbgConsoleRegister ("mem",   "Display memory", 0);
//	DbgConsoleRegister ("vb",    "View breakpoint", 0);

	/* trap ctrl+c */
//	signal (SIGINT, DbgConsoleInterrupt);
}

BOOL _DbgCommandConsoleContinueFlag = TRUE;

void DbgConsoleSendEvent (dbgConsoleEvent code) {
	switch(code) {
		case DBG_CON_CONTINUE:
			_DbgCommandConsoleContinueFlag = TRUE;
			break;
		case DBG_CON_BREAK:
			_DbgCommandConsoleContinueFlag = FALSE;
			break;
	};
}

int DbgConsoleEntry (void) {

	DbgConsoleInit ();
	memset (_console.currentLine, 0, 32);

	printf ("\n\rType \"help\" for information and \"q\" to quit.\n");

	while (1) {

		/* command argument list */
		dbgCommand* command = 0;
		size_t      argc    = 0;
		char*       argv[5];

		/* display prompt and get line */
		DbgDisplayMessage (0);
		fgets (_console.currentLine, 32, stdin);

		/* convert line to argument list */
		argc = DbgConsoleGetArgs (_console.currentLine, argv, 5);
		if (argc==0)
			continue; /* nothing was entered */

		/* locate command */
		command = DbgConsoleGet (argv[0]);
		if (!command) {
			DbgConsoleDefault (argc, argv);
		}
		else if (command->cmd) {
			/* special case: watch for exit command */
			if (strcmp (command->cmd, "q") == 0)
				break;
			/* run command */
			if (command->proc)
				command->proc (argc, argv);
		}

		/* clear line and restart */
		memset (_console.currentLine, 0, 32);
	}

	DbgSessionSendEvent (DbgGetCurrentSession(),DBG_SESSION_QUIT, DBG_SOURCE_COMMAND);
	return EXIT_SUCCESS;
}
