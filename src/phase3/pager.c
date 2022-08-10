#include "pandos_const.h"
#include "pandos_types.h"
#include "/usr/include/umps3/umps/types.h"
#include "/usr/include/umps3/umps/const.h"

// TODO: controllare tutti gli include onde evitare errori come in phase2!!!

swap_t swap_pool[POOLSIZE];

int sem_swap = 1;   // per mutua esclusione sulla swap pool

void init_swap_pool(){
    for(int i = 0; i < POOLSIZE; i++){
        swap_pool[i].sw_asid = NOPROC;
    }
}

unsigned int get_vpn_index(unsigned int vpn){
    // if (vpn == (KUSEG + GETPAGENO) >> VPNSHIFT)    // serve per arrivare all'indirizzo di inizio della stack area
    //     return MAXPAGES - 1;
    // else 
        return vpn - (KUSEG >> VPNSHIFT);
}

int replace_algo(){
    int i;
    //static int index;  // per il rimpiazzamento
    for (i = 0; i < POOLSIZE; i++)
        if(swap_pool[i].sw_asid == NOPROC)
            break;
    if(i < POOLSIZE)    // ho trovato il frame
        return i;
    else{
        static int index = -1;  // essendo static non lo rifara'
        index = (index + 1) % POOLSIZE;
        return index;
    }
}

/* TLB-Refill Handler */
/* One can place debug calls here, but not calls to print */
void uTLB_RefillHandler() {
    state_t *saved_state = (state_t *)BIOSDATAPAGE;
    unsigned int vpn = saved_state->entry_hi >> 12;  /* prendo campo VPN */                
    size_t index = get_vpn_index(vpn);
    // forse serve un controllo
    pteEntry_t pg = currentProcess->p_supportStruct->sup_privatePgTbl[index];
    setENTRYHI(pg.pte_entryHI);
    setENTRYLO(pg.pte_entryLO);
    TLBWR();
    LDST(saved_state);
}

void rw_flash(int operation, int asid, unsigned int index){
    dtpreg_t *flashdev = (dtpreg_t *) DEV_REG_ADDR(FLASHINT, asid-1);
    size_t blocknumber = get_vpn_index(index);
    //if (operation == FLASHWRITE){
    flashdev->data0 = &(swap_pool[index]);    // swap_pool + index
    size_t cmd = operation | blocknumber;
    SYSCALL(DOIO, &(flashdev->command), operation, 0);
    if (flashdev->status != READY){        // c'e' un errore ==> program trap ==> TERMINATE       
        SYSCALL(TERMPROCESS,0,0,0);
    }
}

void pager(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    int code = CAUSE_GET_EXCODE(sup->sup_exceptState->cause);
    if (code == EXC_MOD)
        SYSCALL(TERMPROCESS,0,0,0);
    else{
        SYSCALL(PASSEREN, (int)&sem_swap, 0, 0);
        unsigned int vpn = sup->sup_exceptState->entry_hi >> 12;
        int index = replace_algo();
        if(swap_pool[index].sw_asid != NOPROC){
            setSTATUS(DISABLEINTS);   // disabilito gli interrupt      
            swap_pool[index].sw_pte->pte_entryLO &= ~VALIDON;    // VALIDON == 512 == 2^9 negando ottengo il registro entryLO con il bit V uguale a 0
            
            pteEntry_t sp = swap_pool[index].sw_pte;
            setENTRYHI(sp.pte_entryHI);
            TLBP();
            if (!(getINDEX() & PRESENTFLAG)) {
                setENTRYHI(sp.pte_entryHI);
                setENTRYLO(sp.pte_entryLO);
                TLBWI();
            }
            setSTATUS(IECON);
            rw_flash(FLASHWRITE, swap_pool[index].sw_asid, index);
        }
        rw_flash(FLASHREAD, sup->sup_asid, sup->sup_exceptState->entry_hi >> VPNSHIFT);
        
    }
    
}
