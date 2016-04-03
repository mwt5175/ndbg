/********************************************
*
*	sys.c - Architecture structures
*
********************************************/

#ifndef SYS_H
#define SYS_H

#include "defs.h"

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct _x86_sreg {
	uint16_t ss;
	uint16_t es;
	uint16_t ds;
	uint16_t cs;
	uint16_t gs;
	uint16_t fs;
}x86_sreg;

typedef struct _x86_reg {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t esp;
}x86_reg;

typedef struct _x86_creg {
	uint16_t cr0;
	uint16_t cr1;
	uint16_t cr2;
	uint16_t cr3;
}x86_creg;

typedef struct _x86_dreg {
	uint32_t dr0;
	uint32_t dr1;
	uint32_t dr2;
	uint32_t dr3;
	uint32_t dr6;
	uint32_t dr7;
}x86_dreg;

/**
*	Standard dbgContext structure is defined
*	but the contents are architecture dependent
*/
typedef struct _dbgContext {
	uint32_t eip;
	x86_creg cregs;
	x86_sreg sregs;
	x86_dreg dregs;
	x86_reg  regs;
	unsigned long flags;
}dbgContext;

#endif
