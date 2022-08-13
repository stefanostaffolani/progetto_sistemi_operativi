#include "pager.c"

void exception_handler(){
    support_t *support = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    state_t *except_state = &support->sup_exceptState[GENERALEXCEPT];
    int cause = CAUSE_GET_EXCCODE(except_state->cause);
    switch (cause){
    case 8:       // syscall exception
        syscall_exception_handler(support, except_state);
        break;
    default:
        program_trap_exception_handler(support);
        break;
    }
    except_state->pc_epc += WORDLEN;
    except_state->reg_t9 += WORDLEN;
    LDST(except_state);
}


void syscall_exception_handler(support_t *support, state_t *except_state){
    const int a0 = except_state->reg_a0;
    switch (a0){
    case 1:                             // Get_TOD
        Get_TOD_SYS1(except_state);
        break;
    case 2:                             // Terminate
        Terminate_SYS2(support);
        break;
    case 3:                             // Write_to_Printer
        break;
    case 4:                             // Write_to_Terminal
        break;
    case 5:                             // Read_from_Terminal
        break;
    default:                            // kill process ==> this is an error
        program_trap_exception_handler(support);
        break;
    }
}


void program_trap_exception_handler(support_t *support){   // Terminate process with SYS2 (do V op in sem_swap)
    Terminate_SYS2();
}

void Get_TOD_SYS1(state_t except_state){
    cpu_t TOD;
    STCK(&TOD);
    except_state->reg_v0 = TOD;
}


void Terminate_SYS2(support_t *support){   // capire se va incrementato il PC anche qua (visto che viene fatto dal kernel)
    if(get_swap_asid(support->sup_asid)){
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);    // controllare se aggiusta da solo il semaforo
        for(int i = 0; i < POOLSIZE; i++){
            if(swap_pool[i] == support->sup_asid)    // ottimizzazione
                swap_pool[i] = -1;
        }
        update_swap_asid(0,support->sup_asid);
    }
    SYSCALL(TERMPROCESS,0,0,0);
}