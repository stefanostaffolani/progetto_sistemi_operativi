#include "scheduler.h"

#define MAXPROC 20
#define MAXSEM  MAXPROC

/*integer indicating the number of started, but not
yet terminated processes.*/
int prCount;  // Process Count

/*number of started, but not terminated processes that in are
the “blocked” state due to an I/O or timer request*/
int sbCount; // soft-block Count

/*
Tail pointer to a queue of pcbs that are in the
“ready” state.*/
struct list_head readyQueue;

/*
Pointer to the pcb that is in the “running” state,
i.e. the current executing process.*/
pcb_PTR currentProcess; 

state_t initState;

// inizializzare i vari semafori

passupvector_t* PassUpVector;

extern void uTLB_RefillHandler();

extern void exeptionHandler();   // TODO!!

int dSemaphores[MAXSEM];

int main(){

    PassUpVector = PASSUPVECTOR;
    PassUpVector->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    PassUpVector->exception_stackPtr = (memaddr) KERNELSTACK;
    PassUpVector->exception_handler = (memaddr) exeptionHandler;
    PassUpVector->tlb_refill_stackPtr = (memaddr) KERNELSTACK;

    initPcbs();
    initASL();

    for(int i = 0; i<MAXSEM; i++)
        dSemaphores[i] = 0;
    prCount = 0;
    sbCount = 0;
    mkEmptyProcQ(&readyQueue);
    currentProcess = NULL;

    LDIT(PSECOND);

    pcb_PTR initPcb = allocPcb();

    initPcb->p_time = 0;
    initPcb->p_semAdd = NULL;
    initPcb->p_supportStruct = NULL;
    mkEmptyProcQ(&initPcb->p_child);
    mkEmptyProcQ(&initPcb->p_sib);
    prCount++;
    
    STST(&initState);
    
    

    return 0;
}