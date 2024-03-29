#include "h/interrupt.h"

void interrupt_exception(unsigned int cause, state_t *exception_state){
 
    /*takes
    an unsigned integer as its input parameter and populates it with the value of
    the low-order word of the TOD clock divided by the Time Scale*/
    STCK(interval_timer);   //save the interrupt starting time

    // for each line manage the interrupt
    breakpoint();
    for (int i = 0; i < 8; i++) {
        if (cause & CAUSE_IP(i))
            manageInterr(i, exception_state);
    }
}

void manageInterr(int line, state_t *exception_state){

    if(line == IL_CPUTIMER){  
        /* Acknowledge the PLT interrupt by loading the timer with a new value.
        [Section 4.1.4-pops]*/ 
        setTIMER(0xFFFFFFFF);  
        
        /*Save off the complete processor state at the time of the exception in a BIOS
        data structure on the BIOS Data Page. For Processor 0, the address of this
        processor state is 0x0FFF.F000.*/
        if (currentProcess != NULL){
            memcpy(&(currentProcess->p_s), exception_state, sizeof(state_t));
            set_time(currentProcess, startTime);
            insert_to_readyq(currentProcess);
            /* Place the Current Process on the Ready Queue */
            currentProcess = NULL;    
        }
        scheduler();
    }
    else if(line == IL_TIMER){  // reload interval timer

        /*Acknowledge the interrupt by loading the Interval Timer with a new
        value: 100 milliseconds. [Section 4.1.3-pops]*/
        LDIT(PSECOND);
        
        /* Unblock ALL pcbs blocked on the Pseudo-clock semaphore */
        while(headBlocked(&(dSemaphores[MAXSEM - 1])) != NULL){
            pcb_PTR unblockedP = removeBlocked(&dSemaphores[MAXSEM - 1]);
            sbCount--;                              // decreasing number of sb processes
            insert_to_readyq(unblockedP);
        }

        dSemaphores[MAXSEM-1] = 0;

        if(currentProcess == NULL){
            scheduler();
        }
            
        else{
            LDST(exception_state);  // load old processor state
        }
    }
    else{   // Non-Timer Interrupts
        devregarea_t *deviceRegs = (devregarea_t*) RAMBASEADDR;   
        unsigned int bit_check = 1;
        for(int i = 0; i < DEVPERINT; i++){         // DEVPERINT -> devices per interrupt = 8
            breakpoint();
            if(deviceRegs->interrupt_dev[line-3] & bit_check)
                manageNTInt(line, i, exception_state);
            bit_check = bit_check << 1;
        }
    }
}

void manageNTInt(int line, int dev, state_t *exception_state){

    /* Calculate the address for this device’s device register. [Section 5.1-pops] */
    /* one can compute the starting address of the device’s device register:
    devAddrBase = 0x1000.0054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10) */
    devreg_t* devAddrBase = (devreg_t*) (0x10000054 + ((line - 3) * 0x80) + (dev * 0x10));

    int receive_interr = 0;     
    unsigned int status;
    if (line == IL_TERMINAL){ // if it's a terminal sub device
        termreg_t* terminalRegister = (termreg_t*) devAddrBase;
        if(((terminalRegister->recv_status & 0x000000FF) != READY) && ((terminalRegister->recv_status & 0x000000FF) != BUSY)){            
            klog_print("recv, mando ACK");
            status = terminalRegister->recv_status;             // Save off the status code from the device’s device register
            terminalRegister->recv_command = ACK;               // Acknowledge the interrupt    
            receive_interr = 1;
        }else if((terminalRegister->transm_status != BUSY) && (terminalRegister->transm_status != READY)){
           status = terminalRegister->transm_status;            
           terminalRegister->transm_command = ACK; 
        }

        if (receive_interr == 1)
            dev += 8;

    } 
    else {
        status = devAddrBase->dtp.status;                   // Save off the status code from the device’s device register
        devAddrBase->dtp.command = ACK;                     // Acknowledge the interrupt 
    }

    // Semaphore associated with this (sub)device
    int sem_loc = dev + (DEVPERINT * (line-3));
    int *semAdd = &dSemaphores[sem_loc];

    pcb_PTR unblockedProcess = removeBlocked(semAdd);

    if(unblockedProcess != NULL){
        unblockedProcess->p_s.reg_v0 = status;
        sbCount--;
        insert_to_readyq(unblockedProcess);
    }

    if(currentProcess == NULL){          // if there was no process running
        scheduler();
    }
    else{
        LDST(exception_state);           // load old processor state
    }
}