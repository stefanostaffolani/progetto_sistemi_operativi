#include "pandos_types.h"
#include "pandos_const.h"
#include "../phase1/asl.h"
#include "../phase1/pcb.h"


#define MAXPROC 20
#define MAXSEM  48

extern pcb_PTR currentProcess;
extern int prCount;
extern int sbCount;
//------- queste variabili vanno definite nell'inizializzazione e chiamate con extern------------------
struct list_head high_priority_queue;
struct list_head low_priority_queue;
// mkEmptyProcQ(&high_priority_queue);
// mkEmptyProcQ(&low_priority_queue);
//------------------------------------------------------------------------------------------------------
void scheduler();
