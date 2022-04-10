#include "exception.h"

static state_t* processor_state;   // stato del processore

extern readyQueue;


void exceptionHandler(){
    processor_state = (state_t*) BIOSDATAPAGE;
    const unsigned int CAUSE_CODE = CAUSE_GET_EXCODE(processor_state->cause);

    switch (CAUSE_CODE){
    case EXC_SYS:
        syscall_exception();
        break;
    case EXC_INT:
        interrupt_exception();
        break;
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
        TLBtrap_exception();
        break;
    default:
        program_trap_exception();    // altri casi... da scrivere meglio
        break;
    }
}


void syscall_exception(){
    unsigned int a0 = processor_state->reg_a0;
    unsigned int a1 = processor_state->reg_a1;
    unsigned int a2 = processor_state->reg_a2;
    unsigned int a3 = processor_state->reg_a3;
    switch (a0){
    case CREATEPROCESS:
        Create_Process_SYS1();
        break;
    case TERMPROCESS:
        Terminate_Process_SYS2();
        break;
    case PASSEREN:
        Passeren_SYS3();
        break;
    case VERHOGEN:
        Verhogen_SYS4();
        break;
    case DOIO:
        doio();
        break;
    case GETTIME:
        get_time();
        break;
    case CLOCKWAIT:
        clock_wait();
        break;
    case GETSUPPORTPTR:
        get_support_ptr();
        break;
    case GETPROCESSID:
        get_process_id();
        break;
    case YIELD:
        yield();
        break;
    default:
        break;
    }
}

void interrupt_exception(){
    return;   // TODO
}

