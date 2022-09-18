#include "h/pager.h"
   
//TODO: controllare gli include

/**
 * @brief swap_asid is used for 
 * 
 */
void init_swap_asid(){
    for (int i = 0; i < 8; i++)
        swap_asid[i] = 0;
}

/**
 * @brief 
 * 
 * @param value 
 * @param asid 
 */
void update_swap_asid(int value, int asid){   // value is 1 or 0 (1 if sem is used, 0 else)
    swap_asid[asid-1] = value;
}

/**
 * @brief Get the swap asid object
 * 
 * @param asid 
 * @return int 
 */
int get_swap_asid(int asid){
    return swap_asid[asid-1];
}

/**
 * @brief 
 * 
 */
void init_swap_pool(){
    for(int i = 0; i < POOLSIZE; i++){
        swap_pool[i].sw_asid = NOPROC;
    }
}

/**
 * @brief 
 * 
 * @return int 
 */
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

/**
 * @brief 
 * 
 */
void uTLB_RefillHandler() {
    state_t *saved_state = (state_t *)BIOSDATAPAGE;
    unsigned int index = (saved_state->entry_hi & GETPAGENO) >> VPNSHIFT;  // take VPN 
    if (index == 0x3FFFF){
        index = 31;   // STACK access
    }
    pteEntry_t pg = currentProcess->p_supportStruct->sup_privatePgTbl[index];
    setENTRYHI(pg.pte_entryHI);
    setENTRYLO(pg.pte_entryLO);
    TLBWR();
    LDST(saved_state);
}

/**
 * @brief 
 * 
 * @param operation 
 * @param asid 
 * @param blocknumber 
 * @param frame_addr 
 */
void rw_flash(int operation, int asid, size_t blocknumber, memaddr frame_addr){
    dtpreg_t *flashdev = (dtpreg_t *) DEV_REG_ADDR(FLASHINT, asid-1);
    flashdev->data0 = frame_addr;    // swap_pool + index
    size_t cmd = operation | (blocknumber << 8);
    SYSCALL(DOIO, (int)&(flashdev->command), cmd, 0);
    if (flashdev->status != READY){        // c'e' un errore ==> program trap ==> TERMINATE       
        SYSCALL(TERMPROCESS,0,0,0);
    }
}

/**
 * @brief 
 * 
 * @param index_swap 
 * @param vpn 
 * @param pageno 
 * @param sup 
 */
void update_swap_pool(int index_swap, unsigned int vpn, unsigned int pageno, support_t *sup){
    swap_pool[index_swap].sw_asid = sup->sup_asid;
    swap_pool[index_swap].sw_pageNo = (int)vpn;
    swap_pool[index_swap].sw_pte = sup->sup_privatePgTbl + pageno;
   
}

/**
 * @brief 
 * 
 */
void pager(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    int code = CAUSE_GET_EXCCODE(sup->sup_exceptState->cause);
    if (code == EXC_MOD)
        SYSCALL(TERMPROCESS,0,0,0);
    else{
        update_swap_asid(1,sup->sup_asid);         // value per indicare se c'e' un processo nella swap pool
        SYSCALL(PASSEREN, (int)&sem_swap, 0, 0);
        unsigned int missing_pg_vpn = sup->sup_exceptState->entry_hi >> VPNSHIFT;
        unsigned int missing_pg_no = (sup->sup_exceptState->entry_hi & GETPAGENO) >> VPNSHIFT;
        if(missing_pg_no == 0x3FFFF){
            missing_pg_no = MAXPAGES-1;   // vpn dello STACK
        }
        int index_swap = replace_algo();      // indice di frame
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
            // forse serve if di controllo per 0x3ffff
            unsigned int page_in_frame = (swap_pool[index_swap].sw_pte->pte_entryHI & GETPAGENO) >> VPNSHIFT;
            rw_flash(FLASHWRITE, swap_pool[index_swap].sw_asid, page_in_frame, frame_addr);
        }
        rw_flash(FLASHREAD, sup->sup_asid, (size_t) missing_pg_no, frame_addr);
        update_swap_pool(index_swap, missing_pg_vpn, missing_pg_no, sup);     // aggiorna la swap pool
        setSTATUS(DISABLEINTS & getSTATUS());   // disabilito gli interrupt
        sup->sup_privatePgTbl[missing_pg_no].pte_entryLO = (VALIDON | DIRTYON | frame_addr);    // mette il bit V a 1
        swap_pool[index_swap].sw_pte->pte_entryLO = (VALIDON | DIRTYON | frame_addr);       // perche' non lo settiamo in swap pool update
        setENTRYHI(sup->sup_privatePgTbl[missing_pg_no].pte_entryHI);
        TLBP();                                               //TODO: aggiungere il funzionamento
        if (!(getINDEX() & PRESENTFLAG)) {
            setENTRYHI(sup->sup_privatePgTbl[missing_pg_no].pte_entryHI);
            setENTRYLO(sup->sup_privatePgTbl[missing_pg_no].pte_entryLO);
            TLBWI();
        }
        setSTATUS(IECON | getSTATUS());    //TODO: controllare questo!!!
        update_swap_asid(0,sup->sup_asid);
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);
        LDST(sup->sup_exceptState);
    }
       
}
