#include "exception.c"


int main(){
    // inizializzare le strutture dati
    init_swap_pool();
    init_swap_asid();
    
    state_t state;
    support_t sup;
    memaddr ramtop;
    RAMTOP(memaddr);
    
    state.reg_sp = (memaddr) USERSTACKTOP;
    state.pc_epc = (memaddr) UPROCSTARTADDR;
    state.reg_t9 = (memaddr) UPROCSTARTADDR;
    setSTATUS(IECON | TEBITON);
    
    for(int i=0;i<UPROCMAX;i++){       // l'asid va da 1 a 8 inclusi
        state.entry_hi = (i+1) << ASIDSHIFT;
        sup.sup_asid = i+1;
        //sup.sup_exceptState[PGFAULTEXCEPT] = 
        SYSCALL(CREATEPROCESS, (int)&state, PROCESS_PRIO_LOW, (int)&sup);
    }

}