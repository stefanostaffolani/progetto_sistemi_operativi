#ifndef ASL_H_INCLUDED
#define ASL_H_INCLUDED
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "pcb.h"

static semd_t semd_table[MAXPROC];

static struct list_head *semdFree_h;

static struct list_head *semd_h;

int insertBlocked(int *, pcb_PTR);

void initASL();

pcb_PTR headBlocked(int *);

pcb_PTR outBlocked(pcb_PTR);

pcb_PTR removeBlocked(int *);