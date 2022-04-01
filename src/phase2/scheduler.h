#include "pandos_types.h"
#include "pandos_const.h"
#include "../phase1/asl.h"
#include "../phase1/pcb.h"

//------- queste variabili vanno definite nell'inizializzazione e chiamate con extern------------------
struct list_head high_priority_queue;
struct list_head low_priority_queue;
// mkEmptyProcQ(&high_priority_queue);
// mkEmptyProcQ(&low_priority_queue);
int ProcessCount;
int SoftBlockedCount;
//------------------------------------------------------------------------------------------------------
void scheduler();
