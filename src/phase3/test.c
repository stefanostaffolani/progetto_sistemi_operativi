#include "h/test.h"

void init_pagtable(unsigned int asid, support_t *sup){
    int i;
    for(i=0;i<MAXPAGES-1;i++){
        sup->sup_privatePgTbl[i].pte_entryHI = ((KUSEG | (i << VPNSHIFT)) | (asid << ASIDSHIFT));
        sup->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
    }
    sup->sup_privatePgTbl[i].pte_entryHI  = (0xBFFFF000 | (asid << ASIDSHIFT) );      // ((USERSTACKTOP-1) & 0xFFFFF000) | asid
    sup->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
}


int test(){
    // inizializzare le strutture dati
    init_swap_pool();
    init_swap_asid();
    
    state_t state;
    memaddr ramtop;
    RAMTOP(ramtop);
    static support_t support_array[UPROCMAX];

    state.reg_sp = (memaddr) USERSTACKTOP;
    state.pc_epc = (memaddr) UPROCSTARTADDR;
    state.reg_t9 = (memaddr) UPROCSTARTADDR;
    state.status = IEPON | IMON | TEBITON | USERPON;
    for(int i=0;i < UPROCMAX;i++){       // l'asid va da 1 a 8 inclusi
        state.entry_hi = (i+1) << ASIDSHIFT;
        support_array[i].sup_asid = i+1;
        support_array[i].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
        support_array[i].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr) ramtop - ((i+1) * PAGESIZE * 2) + PAGESIZE;
        support_array[i].sup_exceptContext[PGFAULTEXCEPT].status = IEPON | IMON | TEBITON;
        support_array[i].sup_exceptContext[GENERALEXCEPT].pc = (memaddr) exception_handler_support;
        support_array[i].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr) ramtop - ((i+1) * PAGESIZE * 2);
        support_array[i].sup_exceptContext[GENERALEXCEPT].status = IEPON | IMON | TEBITON;

        // funzione che inizializza la pagetable
        init_pagtable(i+1, &support_array[i]);
        SYSCALL(CREATEPROCESS, (int)&state, PROCESS_PRIO_LOW, (int)&support_array[i]);
    }
    for (int i = 0; i < UPROCMAX; i++){
        SYSCALL(PASSEREN, (int)&master_semaphore, 0, 0);
    }
    SYSCALL(TERMPROCESS,0,0,0);
    return 0;
}
