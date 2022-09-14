#include "../include/exception.h"

void exceptionHandler(){
    processor_state = (state_t*) BIOSDATAPAGE;
    const unsigned int CAUSE_CODE = CAUSE_GET_EXCCODE(processor_state->cause);
    unsigned int cause = processor_state->cause & CAUSE_IP_MASK;
    switch (CAUSE_CODE){
    case EXC_SYS:
        syscall_exception(processor_state);
        break;
    case EXC_INT:
        interrupt_exception(cause, processor_state);
        break;
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
        pass_up_or_die(PGFAULTEXCEPT, processor_state);   // TLBexc...
        break;
    default:
        pass_up_or_die(GENERALEXCEPT, processor_state);   // program trap exc...
        break;
    }
}

void syscall_exception(state_t *exception_state){
    unsigned int a0 = exception_state->reg_a0;
    unsigned int a1 = exception_state->reg_a1;
    
    if ((exception_state->status & STATUS_KUp) == STATUS_KUp){ //il processo non e' in kernel mode
        exception_state->cause = (exception_state->cause & ~CAUSE_EXCCODE_MASK) | (EXC_RI << CAUSE_EXCCODE_BIT);
        pass_up_or_die(GENERALEXCEPT, exception_state);
    }

    switch (a0){
    case CREATEPROCESS:
        Create_Process_NSYS1(exception_state);
        break;
    case TERMPROCESS:
        Terminate_Process_NSYS2(a1,exception_state);
        break;
    case PASSEREN:
        Passeren_NSYS3((int *) a1,exception_state);
        break;
    case VERHOGEN:
        Verhogen_NSYS4((int *) a1,exception_state);
        break;
    case DOIO:
        DO_IO_Device_NSYS5(exception_state);
        break;
    case GETTIME:
        Get_CPU_Time_NSYS6(exception_state);
        break;
    case CLOCKWAIT:
        Wait_For_Clock_NSYS7(exception_state);
        break;
    case GETSUPPORTPTR:
        Get_SUPPORT_Data_NSYS8(exception_state);
        break;
    case GETPROCESSID:
        Get_Process_ID_NSYS9(exception_state, a1);
        break;
    case YIELD:
        Yield_NSYS10(exception_state);
        break;
    default:
        pass_up_or_die(GENERALEXCEPT, exception_state);
    }
}

void pass_up_or_die(int except_type, state_t *exception_state){    
    if (currentProcess->p_supportStruct == NULL){
        Terminate_Process_NSYS2(0, processor_state);
    }else{
        // Copy the saved exception state from the BIOS Data Page to the correct sup exceptState field of the Current Process
        memcpy(&currentProcess->p_supportStruct->sup_exceptState[except_type], exception_state, sizeof(state_t));
        context_t support = currentProcess->p_supportStruct->sup_exceptContext[except_type];
        LDCXT(support.stackPtr, support.status, support.pc);
    }
}