#include "h/pager.h"
   
//TODO: controllare gli include

/**
 * @brief swap_asid used for track process that works in the swap pool
 * 
 */
void init_swap_asid(){
    for (int i = 0; i < 8; i++)
        swap_asid[i] = 0;
}

/**
 * @brief value is 1 or 0 (1 if sem is used, 0 else)
 * 
 * @param value 
 * @param asid 
 */
void update_swap_asid(int value, int asid){
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
 * @brief sw_asid is -1 at the start
 * 
 */
void init_swap_pool(){
    for(int i = 0; i < POOLSIZE; i++){
        swap_pool[i].sw_asid = NOPROC;
    }
}

/**
 * @brief the replacement algorithm search for the asid inside the swap pool and if it found
 * an entry it returns the entry index, else the index is the previous increased.
 * 
 * @return int 
 */
int replace_algo(){
    int i;
    for (i = 0; i < POOLSIZE; i++)                 
        if(swap_pool[i].sw_asid == NOPROC)
            break;
    if(i < POOLSIZE)    // I forund the frame
        return i;
    else{
        static int index = -1;  // static will declare only once
        index = (index + 1) % POOLSIZE;
        return index;
    }
}

/**
 * @brief This is the uTLB_RefillHandler it takes the page from the BIOSDATAPAGE and then
 * write the entry into the TLB using the TLBWR instruction with setENTRYHI and setENTRYLO.
 * 
 */
void uTLB_RefillHandler() {
    state_t *saved_state = (state_t *)BIOSDATAPAGE;
    unsigned int index = (saved_state->entry_hi & GETPAGENO) >> VPNSHIFT;  // take the index of page 
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
 * @brief This function perform a NSYS5 DOIO on flash device for read and write, 
 * blocknumber is the missing page number. The operation may be FLASHWRITE or FLASHREAD.
 * 
 * @param operation 
 * @param asid 
 * @param blocknumber 
 * @param frame_addr 
 */
void rw_flash(int operation, int asid, size_t blocknumber, memaddr frame_addr){
    dtpreg_t *flashdev = (dtpreg_t *) DEV_REG_ADDR(FLASHINT, asid-1);
    flashdev->data0 = frame_addr;                                               // swap_pool + index
    size_t cmd = operation | (blocknumber << 8);                                // |xxxxxxxx|xxxxxxxx|blocknumber|operation|
    SYSCALL(DOIO, (int)&(flashdev->command), cmd, 0);
    if (flashdev->status != READY){                                             // error ==> program trap ==> TERMINATE       
        SYSCALL(TERMPROCESS,0,0,0);
    }
}

/**
 * @brief This function update the swap pool, it is used in the pager.
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
 * @brief This is the pager, when called it update the swap pool and take care of read and write in the 
 * flash device. The access to the swap pool works in mutex so interrups are disabled before operations 
 * and then abled.
 * 
 */
void pager(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    int code = CAUSE_GET_EXCCODE(sup->sup_exceptState->cause);
    if (code == EXC_MOD)
        SYSCALL(TERMPROCESS,0,0,0);
    else{
        update_swap_asid(1,sup->sup_asid);                                                      // there is a process in the swap pool
        SYSCALL(PASSEREN, (int)&sem_swap, 0, 0);
        unsigned int missing_pg_vpn = sup->sup_exceptState->entry_hi >> VPNSHIFT;
        unsigned int missing_pg_no = (sup->sup_exceptState->entry_hi & GETPAGENO) >> VPNSHIFT;
        if(missing_pg_no == 0x3FFFF){
            missing_pg_no = MAXPAGES-1;   // STACK VPN
        }
        int index_swap = replace_algo();                                                        // I get the index of the frame
        memaddr frame_addr = swap_pool_address + (index_swap * PAGESIZE);
        if(swap_pool[index_swap].sw_asid != NOPROC){
            setSTATUS(DISABLEINTS & getSTATUS());                                               // interrupts are off      
            swap_pool[index_swap].sw_pte->pte_entryLO &= ~VALIDON;                              // VALIDON == 512 == 2^9, ~VALIDON I get entryLO  V bit equal to 0
            pteEntry_t sp = *swap_pool[index_swap].sw_pte;
            setENTRYHI(sp.pte_entryHI);
            TLBP();
            if (!(getINDEX() & PRESENTFLAG)) {
                setENTRYHI(sp.pte_entryHI);
                setENTRYLO(sp.pte_entryLO);
                TLBWI();
            }
            setSTATUS(IECON | getSTATUS());
            unsigned int page_in_frame = (swap_pool[index_swap].sw_pte->pte_entryHI & GETPAGENO) >> VPNSHIFT;
            rw_flash(FLASHWRITE, swap_pool[index_swap].sw_asid, page_in_frame, frame_addr);
        }
        rw_flash(FLASHREAD, sup->sup_asid, (size_t)missing_pg_no, frame_addr);
        update_swap_pool(index_swap, missing_pg_vpn, missing_pg_no, sup);    
        setSTATUS(DISABLEINTS & getSTATUS());                                                   // interrupts are now off
        sup->sup_privatePgTbl[missing_pg_no].pte_entryLO = (VALIDON | DIRTYON | frame_addr);    // page is valid V is 1
        swap_pool[index_swap].sw_pte->pte_entryLO = (VALIDON | DIRTYON | frame_addr);           // perche' non lo settiamo in swap pool update
        setENTRYHI(sup->sup_privatePgTbl[missing_pg_no].pte_entryHI);
        /* 
           Probe the TLB (TLBP) to see if the newly updated TLB entry is
           indeed cached in the TLB. If so (Index.P is 0), rewrite (update) that
           entry (TLBWI) to match the entry in the Page Table.
        */
        TLBP();
        if (!(getINDEX() & PRESENTFLAG)) {
            setENTRYHI(sup->sup_privatePgTbl[missing_pg_no].pte_entryHI);
            setENTRYLO(sup->sup_privatePgTbl[missing_pg_no].pte_entryLO);
            TLBWI();
        }
        setSTATUS(IECON | getSTATUS());                                                         // interuupts are now on
        update_swap_asid(0,sup->sup_asid);
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);
        LDST(sup->sup_exceptState);
    }
       
}
