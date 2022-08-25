#include "test.h"
#include "exception.h"
#include "pager.h"

//extern void uTLB_RefillHandler();

//extern void exceptionHandler();  

//extern void test(); /* test function in p2test*/

int main(){

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

    gpid = 1;

    initPcb->p_prio = PROCESS_PRIO_LOW;
    initPcb->p_pid = gpid;
    initPcb->p_time = 0;
    initPcb->p_semAdd = NULL;
    initPcb->p_supportStruct = NULL;
    mkEmptyProcQ(&initPcb->p_child);
    mkEmptyProcQ(&initPcb->p_sib);
    prCount++;
    
    initPcb->p_s.pc_epc = (memaddr) test; /* test function in p2test*/
    
    initPcb->p_s.reg_t9 = (memaddr) test;
    initPcb->p_s.status = IEPON | IMON | TEBITON;
    
    RAMTOP(initPcb->p_s.reg_sp);
    
    insertProcQ(&low_priority_queue, initPcb);

    scheduler();

    return 0;
}