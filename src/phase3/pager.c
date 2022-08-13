#include "pandos_const.h"
#include "pandos_types.h"
#include "/usr/include/umps3/umps/types.h"
#include "/usr/include/umps3/umps/const.h"
#include "phase2/globals.h"

// TODO: controllare tutti gli include onde evitare errori come in phase2!!!

swap_t swap_pool[POOLSIZE];

int sem_swap = 1;   // per mutua esclusione sulla swap pool

int swap_asid[8];

void init_swap_asid(){
    for (int i = 0; i < 8; i++)
        swap_asid[i] = 0;
}

void update_swap_asid(int value, int asid){   // value is 1 or 0 (1 if sem is used, 0 else)
    swap_asid[asid-1] = value;
}

int get_swap_asid(int asid){
    return swap_asid[asid-1];
}

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
    for (i = 0; i < POOLSIZE; i++)                  // ottimizzazione!
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

void rw_flash(int operation, int asid, unsigned int vpn){
    dtpreg_t *flashdev = (dtpreg_t *) DEV_REG_ADDR(FLASHINT, asid-1);
    size_t blocknumber = get_vpn_index(vpn);
    //if (operation == FLASHWRITE){
    flashdev->data0 = &(swap_pool[blocknumber]);    // swap_pool + index
    size_t cmd = operation | blocknumber;
    SYSCALL(DOIO, &(flashdev->command), operation, 0);
    if (flashdev->status != READY){        // c'e' un errore ==> program trap ==> TERMINATE       
        SYSCALL(TERMPROCESS,0,0,0);
    }
}

void update_swap_pool(int index_swap, unsigned int vpn, support_t *sup){
    memaddr vpn_index = get_vpn_index(vpn);
    swap_pool[index_swap].sw_asid = sup->sup_asid;
    swap_pool[index_swap].sw_pageNo = vpn >> VPNSHIFT;
    swap_pool[index_swap].sw_pte = sup->sup_privatePgTbl + vpn_index;   // non serve & perche' e' gia' un array !!
}

void pager(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    int code = CAUSE_GET_EXCODE(sup->sup_exceptState->cause);
    if (code == EXC_MOD)
        SYSCALL(TERMPROCESS,0,0,0);
    else{
        update_swap_asid(1,sup->sup_asid);
        SYSCALL(PASSEREN, (int)&sem_swap, 0, 0);
        unsigned int vpn = sup->sup_exceptState->entry_hi >> VPNSHIFT;
        int index_swap = replace_algo();
        if(swap_pool[index_swap].sw_asid != NOPROC){
            setSTATUS(DISABLEINTS);   // disabilito gli interrupt      
            swap_pool[index_swap].sw_pte->pte_entryLO &= ~VALIDON;    // VALIDON == 512 == 2^9 negando ottengo il registro entryLO con il bit V uguale a 0
            pteEntry_t sp = swap_pool[index_swap].sw_pte;
            setENTRYHI(sp.pte_entryHI);
            TLBP();
            if (!(getINDEX() & PRESENTFLAG)) {
                setENTRYHI(sp.pte_entryHI);
                setENTRYLO(sp.pte_entryLO);
                TLBWI();
            }
            setSTATUS(IECON);
            rw_flash(FLASHWRITE, swap_pool[index_swap].sw_asid, swap_pool[index_swap].sw_pte->pte_entryHI >> VPNSHIFT);
        }
        rw_flash(FLASHREAD, sup->sup_asid, vpn);
        update_swap_pool(index_swap, vpn, sup);     // aggiorna la swap pool
        setSTATUS(DISABLEINTS);   // disabilito gli interrupt
        unsigned int vpn_index = get_vpn_index(vpn);
        sup->sup_privatePgTbl[vpn_index].pte_entryLO = (VALIDON | DIRTYON | (&(swap_pool[index_swap]) << ENTRYLO_PFN_BIT));    // mette il bit V a 1
        //pteEntry_t sp = swap_pool[index_swap].sw_pte;
        setENTRYHI(sup->sup_privatePgTbl[vpn].pte_entryHI);
        TLBP();
        if (!(getINDEX() & PRESENTFLAG)) {
            setENTRYHI(sup->sup_privatePgTbl[vpn].pte_entryHI);
            setENTRYLO(sup->sup_privatePgTbl[vpn].pte_entryLO);
            TLBWI();
        }
        setSTATUS(IECON);
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);
        update_swap_asid(0,sup->sup_asid);
        LDST(sup->sup_exceptState);
    }   
}
