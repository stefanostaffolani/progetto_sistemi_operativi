#include "exception.h"

void exceptionHandler(){
    //klog_print("entro in exception handler..\n");
    processor_state = (state_t*) BIOSDATAPAGE;
    const unsigned int CAUSE_CODE = CAUSE_GET_EXCCODE(processor_state->cause);
    // processor_state->pc_epc += WORD_SIZE;
    unsigned int cause = processor_state->cause & CAUSE_IP_MASK;
    breakpoint();
    // klog_print_hex(CAUSE_CODE);
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
        klog_print("TLBS PASSUP\n");
        breakpoint();
        pass_up_or_die(PGFAULTEXCEPT, processor_state);   // TLBexc...
        break;
    default:
        klog_print("def1 PASSUP\n");
        klog_print_hex(CAUSE_CODE);
        breakpoint();
        pass_up_or_die(GENERALEXCEPT, processor_state);   // program trap exc...
        break;
    }
}


void syscall_exception(state_t *exception_state){
    unsigned int a0 = exception_state->reg_a0;
    unsigned int a1 = exception_state->reg_a1;
    //unsigned int a2 = processor_state->reg_a2;
    //unsigned int a3 = processor_state->reg_a3;
    // processor_state->pc_epc += WORD_SIZE;
    
    if ((exception_state->status & STATUS_KUp) != ALLOFF){ //il processo non e' in kernel mode
        klog_print("CAUSE PASSUP\n");
        setCAUSE(EXC_RI);
        breakpoint();
        pass_up_or_die(GENERALEXCEPT, exception_state);
    } 

    //processor_state->pc_epc += WORD_SIZE;
    //processor_state->reg_t9 = processor_state->pc_epc;

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
        NSYS6_Get_CPU_Time(exception_state);
        break;
    case CLOCKWAIT:
        NSYS7_Wait_For_Clock(exception_state);
        break;
    case GETSUPPORTPTR:
        NSYS8_Get_SUPPORT_Data(exception_state);
        break;
    case GETPROCESSID:
        NSYS9_Get_Process_ID(exception_state);
        break;
    case YIELD:
        NSYS10_Yield(exception_state);
        break;
    default:
        klog_print("def2 PASSUP\n");
        exception_state->cause = (exception_state->cause & ~CAUSE_EXCCODE_MASK) | (EXC_RI << CAUSE_EXCCODE_BIT);
        breakpoint();
        pass_up_or_die(GENERALEXCEPT, exception_state);
    }
}

void pass_up_or_die(int except_type, state_t *exceptio_state){    // check if similar to trap
    klog_print("pass UP\n");
    breakpoint();
    if (currentProcess->p_supportStruct == NULL){
        Terminate_Process_NSYS2(currentProcess->p_pid, processor_state);
    }else{
        // Copy the saved exception state from the BIOS Data Page to the correct sup exceptState field of the Current Process
        currentProcess->p_supportStruct->sup_exceptState[except_type] = *exceptio_state;  
        // Perform a LDCXT using the fields from the correct sup exceptContext field of the Current Process.
        context_t support = currentProcess->p_supportStruct->sup_exceptContext[except_type];
        breakpoint();
        klog_print("sto per fare la LDXCT\n");
        LDCXT(support.stackPtr, support.status, support.pc);
    }
}

