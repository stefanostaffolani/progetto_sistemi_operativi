#include "pager.h"
   
//TODO: controllare gli include

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
    klog_print("Utlb Refill\n");
    state_t *saved_state = (state_t *)BIOSDATAPAGE;
    unsigned int index = (saved_state->entry_hi & GETPAGENO) >> VPNSHIFT;  /* prendo campo VPN */
    
    klog_print("saved state entry hi: ");
    klog_print_hex(saved_state->entry_hi);
    klog_print("\n"); 
    
    klog_print("index: ");
    klog_print_hex(index);
    klog_print("\n");                
    //size_t index = get_vpn_index(vpn);
    klog_print("sup except utlb: \n" );
    klog_print_hex((memaddr)saved_state);
    klog_print("\n");

    // support_t *sup = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    // size_t index = get_vpn_index(sup->sup_exceptState->entry_hi >> VPNSHIFT);
    // forse serve un controllo
    klog_print("prima di accedere alla support\n");
    //klog_print_hex(index);
    //pteEntry_t *p = currentProcess->p_supportStruct->sup_privatePgTbl;
    pteEntry_t pg = currentProcess->p_supportStruct->sup_privatePgTbl[index];
    klog_print("\nEntryHi : ");
    klog_print_hex(pg.pte_entryHI);
    klog_print("\n");
    breakpoint();
    setENTRYHI(pg.pte_entryHI);
    setENTRYLO(pg.pte_entryLO);
    TLBWR();
    LDST(saved_state);
}

void rw_flash(int operation, int asid, unsigned int vpn, memaddr frame_addr){
    dtpreg_t *flashdev = (dtpreg_t *) DEV_REG_ADDR(FLASHINT, asid-1);
    size_t blocknumber = get_vpn_index(vpn);
    //if (operation == FLASHWRITE){
    flashdev->data0 = frame_addr;    // swap_pool + index
    size_t cmd = operation | blocknumber;
    unsigned int status = SYSCALL(DOIO, (int)&(flashdev->command), cmd, 0);
    klog_print("frame address: ");
    klog_print_hex(frame_addr);
    klog_print("\n");
    klog_print("lo status della DOIO :\n");
    klog_print_hex(status);
    breakpoint();
    if (flashdev->status != READY){        // c'e' un errore ==> program trap ==> TERMINATE       
        SYSCALL(TERMPROCESS,0,0,0);
    }
}

void update_swap_pool(int index_swap, unsigned int vpn, support_t *sup){
    memaddr vpn_index = get_vpn_index(vpn);
    swap_pool[index_swap].sw_asid = sup->sup_asid;
    swap_pool[index_swap].sw_pageNo = (int)vpn;
    // klog_print("VPN UPDATE\n");
    // klog_print_hex(vpn_index);
    // klog_print("\n");
    swap_pool[index_swap].sw_pte = sup->sup_privatePgTbl + vpn_index;   // non serve & perche' e' gia' un array !!
    klog_print("sw_asid: ");
    klog_print_hex(swap_pool[index_swap].sw_asid);
    klog_print("\nsw_pageNo: ");
    klog_print_hex(swap_pool[index_swap].sw_pageNo);
    klog_print("\nsw_pte_entry HI: ");
    klog_print_hex(swap_pool[index_swap].sw_pte->pte_entryHI);
    klog_print("\n");
    breakpoint();
}

void pager(){
    klog_print("inizio pager\n");
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    int code = CAUSE_GET_EXCCODE(sup->sup_exceptState->cause);
    if (code == EXC_MOD)
        SYSCALL(TERMPROCESS,0,0,0);
    else{
        SYSCALL(PASSEREN, (int)&sem_swap, 0, 0);
        update_swap_asid(1,sup->sup_asid);         // value per indicare se c'e' un processo nella swap pool
        unsigned int vpn = sup->sup_exceptState->entry_hi >> VPNSHIFT;
        int index_swap = replace_algo();      // indice di frame
        klog_print("ho fatto replace algo\n");
        memaddr frame_addr = swap_pool_address + (index_swap * PAGESIZE);
        if(swap_pool[index_swap].sw_asid != NOPROC){
            setSTATUS(DISABLEINTS & getSTATUS());   // disabilito gli interrupt      
            swap_pool[index_swap].sw_pte->pte_entryLO &= ~VALIDON;    // VALIDON == 512 == 2^9 negando ottengo il registro entryLO con il bit V uguale a 0
            pteEntry_t sp = *swap_pool[index_swap].sw_pte;
            setENTRYHI(sp.pte_entryHI);
            TLBP();
            if (!(getINDEX() & PRESENTFLAG)) {
                setENTRYHI(sp.pte_entryHI);
                setENTRYLO(sp.pte_entryLO);
                TLBWI();
            }
            setSTATUS(IECON | getSTATUS());
            klog_print("sto per fare rw_flash dentro if\n");
            rw_flash(FLASHWRITE, swap_pool[index_swap].sw_asid, swap_pool[index_swap].sw_pte->pte_entryHI >> VPNSHIFT, frame_addr);
        }
        //breakpoint();
        rw_flash(FLASHREAD, sup->sup_asid, vpn, frame_addr);
        klog_print("ho fatto rw_flash read\n");
        update_swap_pool(index_swap, vpn, sup);     // aggiorna la swap pool
        klog_print("updated swap pool\n");
        //breakpoint();
        setSTATUS(DISABLEINTS & getSTATUS());   // disabilito gli interrupt
        //klog_print("setstatus done\n");
        unsigned int vpn_index = get_vpn_index(vpn);
        sup->sup_privatePgTbl[vpn_index].pte_entryLO = (VALIDON | DIRTYON | frame_addr);    // mette il bit V a 1
        swap_pool[index_swap].sw_pte->pte_entryLO = (VALIDON | DIRTYON | frame_addr);       // perche' non lo settiamo in swap pool update
        //pteEntry_t sp = swap_pool[index_swap].sw_pte;
        setENTRYHI(sup->sup_privatePgTbl[vpn_index].pte_entryHI);
        TLBP();                                               //TODO: aggiungere il funzionamento
        if (!(getINDEX() & PRESENTFLAG)) {
            setENTRYHI(sup->sup_privatePgTbl[vpn_index].pte_entryHI);
            setENTRYLO(sup->sup_privatePgTbl[vpn_index].pte_entryLO);
            TLBWI();
        }
        setSTATUS(IECON | getSTATUS());    //TODO: controllare questo!!!
        klog_print("riabilitati gli interrupt\n");
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);
        update_swap_asid(0,sup->sup_asid);
        klog_print("end pager\n");
        breakpoint();
        klog_print("sup except: \n" );
        klog_print_hex((memaddr) &(sup->sup_exceptState[0]));
        klog_print("\n");
        klog_print("entryHI prima LDST :");
        klog_print_hex(sup->sup_privatePgTbl[vpn_index].pte_entryHI);
        klog_print("\n");
        LDST(sup->sup_exceptState);
    }   
}
