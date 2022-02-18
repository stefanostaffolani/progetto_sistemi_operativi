#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "pandos_types.h"

// implementazione veloce, da controllare e commentare!!!

//pcb_t pcbFree_table[MAXPROC];
static pcb_PTR pcbFree_h;

static pcb_PTR pcbFree_table[MAXPROC];

void initPcbs();

void freePcb(pcb_PTR);

pcb_PTR allocPcb();

int emptyProcQ(struct list_head *);

void mkEmptyProcQ(struct list_head *);

pcb_t* headProcQ(struct list_head*);

pcb_t* outProcQ(struct list_head *, pcb_t *);

pcb_PTR removeProcQ(struct list_head *);

int emptyChild(pcb_PTR);

void insertChild(pcb_PTR, pcb_PTR);

pcb_PTR removeChild(pcb_PTR);

pcb_PTR outChild(pcb_PTR);