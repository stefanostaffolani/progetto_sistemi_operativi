#include "exception.h"

void exceptionHandler(){
    //klog_print("entro in exception handler..\n");
    processor_state = (state_t*) BIOSDATAPAGE;
    const unsigned int CAUSE_CODE = CAUSE_GET_EXCCODE(processor_state->cause);
    processor_state->pc_epc += WORD_SIZE;
    unsigned int cause = processor_state->cause & CAUSE_IP_MASK;
    // klog_print_hex(CAUSE_CODE);
    switch (CAUSE_CODE){
    case EXC_SYS:
        syscall_exception();
        break;
    case EXC_INT:
        interrupt_exception(cause);
        break;
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
        pass_up_or_die(PGFAULTEXCEPT);   // TLBexc...
        break;
    default:
        pass_up_or_die(GENERALEXCEPT);   // program trap exc...
        break;
    }
    LDST(processor_state);
}


void syscall_exception(){
    unsigned int a0 = processor_state->reg_a0;
    unsigned int a1 = processor_state->reg_a1;
    unsigned int a2 = processor_state->reg_a2;
    unsigned int a3 = processor_state->reg_a3;
    // processor_state->pc_epc += WORD_SIZE;

    if ((processor_state->status & USERPON) != ALLOFF){ //il processo non e' in kernel mode
        setCAUSE(EXC_RI);
        pass_up_or_die(GENERALEXCEPT);
    } 

    switch (a0){
    case CREATEPROCESS:
        Create_Process_NSYS1();
        break;
    case TERMPROCESS:
        Terminate_Process_NSYS2();
        break;
    case PASSEREN:
        Passeren_NSYS3(a1);
        break;
    case VERHOGEN:
        Verhogen_NSYS4(a1);
        break;
    case DOIO:
        DO_IO_Device_NSYS5();
        break;
    case GETTIME:
        NSYS6_Get_CPU_Time();
        break;
    case CLOCKWAIT:
        NSYS7_Wait_For_Clock();
        break;
    case GETSUPPORTPTR:
        NSYS8_Get_SUPPORT_Data();
        break;
    case GETPROCESSID:
        NSYS9_Get_Process_ID();
        break;
    case YIELD:
        NSYS10_Yield();
        break;
    default:
        break;
    }
}

void pass_up_or_die(int except_type){    // check if similar to trap
    if (currentProcess->p_supportStruct == NULL){
        Terminate_Process_NSYS2();
    }else{
        // Copy the saved exception state from the BIOS Data Page to the correct sup exceptState field of the Current Process
        currentProcess->p_supportStruct->sup_exceptState[except_type] = *processor_state;  
        // Perform a LDCXT using the fields from the correct sup exceptContext field of the Current Process.
        context_t support = currentProcess->p_supportStruct->sup_exceptContext[except_type];
        LDCXT(support.stackPtr, support.status, support.pc);
    }
}

