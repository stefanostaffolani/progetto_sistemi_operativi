#include "../phase2/pandos_types.h"
#include "../phase2/pandos_const.h"
#include "asl.h"
#include "pcb.h"
#include "/usr/include/umps3/umps/cp0.h"
#include "/usr/include/umps3/umps/libumps.h"
#include "debug.h"

#define size_t unsigned int

extern cpu_t startTime;
extern state_t* processor_state;
extern struct list_head high_priority_queue;
extern struct list_head low_priority_queue;
extern int prCount;
extern int sbCount;
extern int gpid;
extern pcb_PTR currentProcess;
extern passupvector_t* PassUpVector;
extern devregarea_t* memInfo;
extern cpu_t interval_timer;
extern int dSemaphores[MAXPROC];

void breakpoint();
void memcpy(void *, const void *, size_t);
void insert_to_readyq(pcb_t *);
void set_time(pcb_t *, cpu_t);