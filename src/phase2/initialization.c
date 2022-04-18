#include "scheduler.h"

/*integer indicating the number of started, but not
yet terminated processes.*/
int prCount;  // Process Count

/*number of started, but not terminated processes that in are
the “blocked” state due to an I/O or timer request*/
int sbCount; // soft-block Count

/* 
Tail pointer to a queue of pcbs that are in the
“ready” state.*/
struct list_head low_priority_queue;
struct list_head high_priority_queue;

/*
Pointer to the pcb that is in the “running” state,
i.e. the current executing process.*/
pcb_PTR currentProcess; 

// inizializzare i vari semafori
passupvector_t* PassUpVector;

/* memory info to calculate ramtop*/
devregarea_t* memInfo;

extern void uTLB_RefillHandler();

extern void exceptionHandler();   // TODO!!

extern void test(); /* test function in p2test*/

int dSemaphores[MAXSEM];

int main(){

    //unsigned int ramtop;
    devregarea_t* ramInfo = (devregarea_t *) RAMBASEADDR;
    unsigned int ramtop = (ramInfo -> rambase) + (ramInfo -> ramsize);

    PassUpVector = (passupvector_t*) PASSUPVECTOR;
    PassUpVector->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    PassUpVector->exception_stackPtr = (memaddr) KERNELSTACK;
    PassUpVector->exception_handler = (memaddr) exceptionHandler;
    PassUpVector->tlb_refill_stackPtr = (memaddr) KERNELSTACK;

    initPcbs();
    initASL();

    for(int i = 0; i<MAXSEM; i++)
        dSemaphores[i] = 0;
    prCount = 0;
    sbCount = 0;
    mkEmptyProcQ(&low_priority_queue);
    mkEmptyProcQ(&high_priority_queue);

    currentProcess = NULL;

    LDIT(PSECOND);

    pcb_PTR initPcb = allocPcb();

    initPcb->p_time = 0;
    initPcb->p_semAdd = NULL;
    initPcb->p_supportStruct = NULL;
    mkEmptyProcQ(&initPcb->p_child);
    mkEmptyProcQ(&initPcb->p_sib);
    prCount++;
    
    STST(&initPcb);
    initPcb->p_s.reg_sp = ramtop;
    initPcb->p_s.pc_epc = (memaddr) test; /* test function in p2test*/
    initPcb->p_s.reg_t9 = (memaddr) test;
    initPcb->p_s.status = IEPON | IMON | TEBITON;
    
    
    //scheduler();

    return 0;
}