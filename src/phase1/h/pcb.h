#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED
#endif

#ifndef bool
#define bool int
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "pandos_types.h"

void initPcbs();

void insertProcQ(struct list_head *, pcb_PTR);

void freePcb(pcb_PTR);

pcb_PTR allocPcb();

bool emptyProcQ(struct list_head *);

void mkEmptyProcQ(struct list_head *);

pcb_t* headProcQ(struct list_head*);

pcb_t* outProcQ(struct list_head *, pcb_t *);

pcb_PTR removeProcQ(struct list_head *);

bool emptyChild(pcb_PTR);

void insertChild(pcb_PTR, pcb_PTR);

pcb_PTR removeChild(pcb_PTR);

pcb_PTR outChild(pcb_PTR);