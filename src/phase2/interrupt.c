#include "interrupt.h"

void interrupt_exception(unsigned int cause, state_t *exception_state){
    //klog_print("exc start int\n");
    //klog_print_hex(exception_state);
    // klog_print("entro interrupt \n");
    // TODO: verificare le operazioni bit a bit
    /*takes
    an unsigned integer as its input parameter and populates it with the value of
    the low-order word of the TOD clock divided by the Time Scale*/
    STCK(interval_timer);   //save the interrupt starting time

    /*Which interrupt lines have pending interrupts is set in Cause.IP*/
    // devo prendere i bit di cause da 2^9 a 2^15

    // for each line manage the interrupt
    for (int i = 1; i < 8; i++) {
        if (cause & CAUSE_IP(i))
            manageInterr(i, exception_state);
    }
}

void manageInterr(int line, state_t *exception_state){
    // klog_print("mannaggio un interrupt..\n");

    if(line == 1){  // plt processor local timer interrupt
        // klog_print("plt_timer\n");
        // klog_print("si tratta di un plt timer\n");
        /* Acknowledge the PLT interrupt by loading the timer with a new value.
        [Section 4.1.4-pops]*/ 
        setTIMER(0xFFFFFFFF);  //TODO: aggiusta
        
        /*Save off the complete processor state at the time of the exception in a BIOS
        data structure on the BIOS Data Page. For Processor 0, the address of this
        processor state is 0x0FFF.F000.*/
        if (currentProcess != NULL){
            //currentProcess->p_s = *exception_state;
            memcpy(&(currentProcess->p_s), exception_state, sizeof(state_t));
            set_time(currentProcess, startTime);
            insert_to_readyq(currentProcess);
            /* Place the Current Process on the Ready Queue */
            currentProcess = NULL;    // controllare questa cosa >>> dovrebbe essere per lo scheduler...
        }
        scheduler();
    }
    else if(line == 2){  // reload interval timer
        klog_print("reload interval timer\n");
        // klog_print("si tratta di un reload interval timer\n");

        /*Acknowledge the interrupt by loading the Interval Timer with a new
        value: 100 milliseconds. [Section 4.1.3-pops]*/
        
        LDIT(PSECOND);
        
        /* Unblock ALL pcbs blocked on the Pseudo-clock semaphore */
        while(headBlocked(&(dSemaphores[MAXSEM - 1])) != NULL){
            klog_print("\ncosa accade?\n");
            pcb_PTR unblockedP = removeBlocked(&dSemaphores[MAXSEM - 1]);
            // klog_print("dec sbC while\n");
            //breakpoint();
            sbCount--;                              // decreasing number of sb processes
            insert_to_readyq(unblockedP);
        }

        dSemaphores[MAXSEM-1] = 0;

        if(currentProcess == NULL){
            klog_print("chiamo scheduler\n");
            breakpoint();
            scheduler();
        }
            
        else{
        // klog_print("LDST sbrago\n");
        // klog_print_hex(exception_state);
        // klog_print("ho finito interval timer\n");
        klog_print("sto per fare LDST\n");
        breakpoint();
        LDST(exception_state);  // load old processor state
        }
    }
    else{   // Non-Timer Interrupts
        // klog_print("devicessss\n");

        devregarea_t *deviceRegs = (devregarea_t*) RAMBASEADDR;   
        unsigned int bit_check = 1;
        for(int i = 0; i < DEVPERINT; i++){         // DEVPERINT -> devices per interrupt = 8
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

    if (line == 7){ // if it's a terminal sub device
        // klog_print("it's a terminal\n");
        termreg_t* terminalRegister = (termreg_t*) devAddrBase;
        // klog_print("superato il casting\n");

        if((terminalRegister->recv_status != READY) && (terminalRegister->recv_status != BUSY)){             // in caso mettere 0xff
            status = terminalRegister->recv_status;     // Save off the status code from the device’s device register
            terminalRegister->recv_command = ACK;               // Acknowledge the interrupt    
            receive_interr = 1;
            // klog_print("primo if\n");
        }                       

        if((terminalRegister->transm_status != BUSY) && (terminalRegister->transm_status != READY)){
           status = terminalRegister->transm_status;
           terminalRegister->transm_command = ACK; 
        //    klog_print("secondo if\n");
        }

        // klog_print("superati gli if\n");
        if (receive_interr == 1)
            dev += 8;

    } 
    else {
        status = devAddrBase->dtp.status;           // Save off the status code from the device’s device register
        devAddrBase->dtp.command = ACK;                     // Acknowledge the interrupt 
    }

    // Semaphore associated with this (sub)device
    int sem_loc = dev + (DEVPERINT * (line-3));
    int *semAdd = &dSemaphores[sem_loc];


    // klog_print("sem_loc: \n");
    // klog_print_hex(sem_loc);
    // klog_print("\n");

    // if(headBlocked(semAdd) == NULL) { 
    //     if(currentProcess != NULL){  //nel dubbio
    //         // klog_print("exc st\n");
    //         // klog_print_hex(exception_state);
    //         LDST(exception_state);
    //     }
    //     else
    //         scheduler();
    // }
    // else{
    // klog_print("dec sbC NTINT\n");

    // klog_print("arrivo alla fine di devices\n");

    breakpoint();
    pcb_PTR unblockedProcess = removeBlocked(semAdd);

    // klog_print("unblockedProcess:\n");
    // klog_print_hex(unblockedProcess->p_pid);
    // klog_print("\n");

    // klog_print("currentProcess:\n");
    // klog_print_hex(currentProcess->p_pid);
    // klog_print("\n");
    // breakpoint();
    if(unblockedProcess != NULL){
        // klog_print("aggiorno sbCount...\n");
        unblockedProcess->p_s.reg_v0 = status;
        sbCount--;
        insert_to_readyq(unblockedProcess);
        //currentProcess = NULL;
    }
   // }
    if(currentProcess == NULL){          // if there was no process running
        // klog_print("current proc is NULL");
        scheduler();
    }
    else{
        // klog_print("sto per fare LDST dei devices\n");
        // klog_print("NTINT LDST\n");
        breakpoint();
        LDST(exception_state);  // load old processor state
    }
}