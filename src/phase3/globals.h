//#include "../phase2/pandos_types.h"
#include "../phase2/pandos_const.h"
#include "asl.h"
#include "pcb.h"
#include "/usr/include/umps3/umps/cp0.h"
#include "/usr/include/umps3/umps/libumps.h"
#include "debug.h"
#include "/usr/include/umps3/umps/arch.h"
#include "pandos_const.h"
#include "pandos_types.h"
#include "/usr/include/umps3/umps/types.h"
#include "/usr/include/umps3/umps/const.h"

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
extern int dSemaphores[MAXSEM];

extern swap_t swap_pool[POOLSIZE];
extern int sem_swap;   // per mutua esclusione sulla swap pool
extern int swap_asid[8];
extern int sem_write_printer;   // semafori per le syscall write (SYS3 e SYS4)
extern int sem_write_terminal;
extern int sem_read_terminal;   // semaforo per la syscall read (SYS5)
extern memaddr swap_pool_address;

void breakpoint();
void memcpy(void *, const void *, size_t);
void insert_to_readyq(pcb_t *);
void set_time(pcb_t *, cpu_t);