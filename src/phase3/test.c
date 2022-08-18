#include "exception.c"


int main(){
    // inizializzare le strutture dati
    init_swap_pool();
    init_swap_asid();
    
    state_t state;
    support_t sup;
    memaddr ramtop;
    RAMTOP(ramtop);
    
    state.reg_sp = (memaddr) USERSTACKTOP;
    state.pc_epc = (memaddr) UPROCSTARTADDR;
    state.reg_t9 = (memaddr) UPROCSTARTADDR;
    setSTATUS(IECON | TEBITON);    // deve essere in usermode
    
    for(int i=0;i<UPROCMAX;i++){       // l'asid va da 1 a 8 inclusi
        state.entry_hi = (i+1) << ASIDSHIFT;
        sup.sup_asid = i+1;
        sup.sup_exceptContext[0].pc = (memaddr) uTLB_RefillHandler;
        sup.sup_exceptContext[0].stackPtr = (memaddr) ramtop - (sup.sup_asid * PAGESIZE * 2) + PAGESIZE;
        sup.sup_exceptContext[0].status = state;
        sup.sup_exceptContext[1].pc = (memaddr) exception_handler;
        sup.sup_exceptContext[1].stackPtr = (memaddr) ramtop - (sup.sup_asid * PAGESIZE * 2);
        sup.sup_exceptContext[1].status = state;
        //sup.sup_exceptState[PGFAULTEXCEPT] = 
        SYSCALL(CREATEPROCESS, (int)&state, PROCESS_PRIO_LOW, (int)&sup);
    }
}