#include "globals.h"

cpu_t startTime;
state_t* processor_state;
struct list_head high_priority_queue;
struct list_head low_priority_queue;
cpu_t interval_timer;
int dSemaphores[MAXSEM];

/*integer indicating the number of started, but not
yet terminated processes.*/
int prCount;  // Process Count

/*number of started, but not terminated processes that in are
the “blocked” state due to an I/O or timer request*/
int sbCount; // soft-block Count

int gpid; /* global pid */

/*
Pointer to the pcb that is in the “running” state,
i.e. the current executing process.*/
pcb_PTR currentProcess; 

// inizializzare i vari semafori
passupvector_t* PassUpVector;

/* memory info to calculate ramtop*/
devregarea_t* memInfo;

swap_t swap_pool[POOLSIZE];
int sem_swap = 1;   // per mutua esclusione sulla swap pool
int swap_asid[8];
int sem_write_printer = 1;   // semafori per le syscall write (SYS3 e SYS4)
int sem_write_terminal = 1;
int sem_read_terminal = 1;   // semaforo per la syscall read (SYS5)
int master_semaphore = 0;
memaddr swap_pool_address = 0x20020000;   

void memcpy(void *dest, const void *src, size_t n){
    for (size_t i = 0; i < n; i++){
        ((unsigned char*)dest)[i] = ((unsigned char*)src)[i];
    }
}

void insert_to_readyq(pcb_PTR proc){
    if(proc->p_prio == PROCESS_PRIO_LOW)
        insertProcQ(&low_priority_queue, proc); //and is placed in the Ready Queue
    else
        insertProcQ(&high_priority_queue, proc); //and is placed in the Ready Queue
}

void set_time(pcb_PTR proc, cpu_t stime){
    cpu_t endTime;
    STCK(endTime);
    proc->p_time += endTime - stime;
}