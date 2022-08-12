#include "pager.c"

void exception_handler(){
    const unsigned int CAUSE_CODE = CAUSE_GET_EXCCODE(currentProcess->p_supportStruct->sup_exceptState->cause);
    state_t *except_state = (state_t *)currentProcess->p_supportStruct->sup_exceptState;
    switch (CAUSE_CODE){
    case EXC_ADEL:     // program trap
    case EXC_ADES:
    case EXC_IBE:
    case EXC_DBE:
    case EXC_BP:
    case EXC_RI:
    case EXC_CPU:
    case EXC_OV:
        program_trap_exception_handler(except_state);
        break;
    case EXC_SYS:       // syscall exception
        syscall_exception_handler(except_state);
        break;
    default:
        uTLB_RefillHandler();   // TLB exception
        break;
    }
}


void syscall_exception_handler(state_t *except_state){
    const int a0 = except_state->reg_a0;
    switch (a0){
    case 1:
        Get_TOD_SYS1(except_state);
        break;
    case 2:
        Terminate_SYS2(except_state);
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    default:
        break;
    }

}


void program_trap_exception_handler(state_t *except_state){   // Terminate process with SYS2 (do V op in sem_swap)
    if (*sem_swap != 1)
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);
    Terminate_SYS2();
}

void Get_TOD_SYS1(state_t except_state){
    unsigned int TOD;
    STCK(TOD);
    except_state->reg_v0 = TOD;
    except_state->pc_epc += WORDLEN;
}


void Terminate_SYS2(){   // capire se va incrementato il PC anche qua (visto che viene fatto dal kernel)
    SYSCALL(TERMPROCESS,0,0,0);
}