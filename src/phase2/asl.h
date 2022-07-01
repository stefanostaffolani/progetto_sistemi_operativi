#ifndef ASL_H_INCLUDED
#define ASL_H_INCLUDED
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

#include "pcb.h"

bool insertBlocked(int *, pcb_PTR);

void initASL();

pcb_PTR headBlocked(int *);

pcb_PTR outBlocked(pcb_PTR);

pcb_PTR removeBlocked(int *);