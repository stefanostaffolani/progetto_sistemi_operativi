#include "interrupt.h"

int bruno = 1;

void interrupt_exception(unsigned int cause){
 
    // klog_print("entro interrupt \n");
    // TODO: verificare le operazioni bit a bit
    /*takes
    an unsigned integer as its input parameter and populates it with the value of
    the low-order word of the TOD clock divided by the Time Scale*/
    STCK(interval_timer);   //save the interrupt starting time

    /*Which interrupt lines have pending interrupts is set in Cause.IP*/
    // devo prendere i bit di cause da 2^9 a 2^15

    unsigned int bit_check = 1 << 9; 
    // for each line manage the interrupt
    
    //klog_print_hex(cause);

    // if(bruno < 2){
    //     klog_print("\n");
    //     klog_print_hex(cause);
    //     bruno++;
    // }

    //klog_print("prima del for\n");

    for (int i = 1; i < 8; i++) {
       // klog_print("sono nel for\n");
        if (cause & bit_check)
            manageInterr(i);
        bit_check = bit_check << 1;
    }
    //klog_print("\n");
    //klog_print("dopo il for\n");

}

void manageInterr(int line){
    klog_print("mannaggio un interrupt..\n");

    if(line == 1){  // plt processor local timer interrupt
        
        klog_print("si tratta di un plt timer\n");
        /* Acknowledge the PLT interrupt by loading the timer with a new value.
        [Section 4.1.4-pops]*/ 
        setTIMER(PSECOND);
        
        /*Save off the complete processor state at the time of the exception in a BIOS
        data structure on the BIOS Data Page. For Processor 0, the address of this
        processor state is 0x0FFF.F000.*/
        currentProcess->p_s = *((state_t*) BIOSDATAPAGE);
        
        cpu_t endTime;
        STCK(endTime);
        currentProcess->p_time = endTime - startTime;

        /* Place the Current Process on the Ready Queue */
        if(currentProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, currentProcess);
        else
            insertProcQ(&high_priority_queue, currentProcess);
        

        scheduler();
    }
    else if(line == 2){  // reload interval timer
        klog_print("si tratta di un reload interval timer\n");

        /*Acknowledge the interrupt by loading the Interval Timer with a new
        value: 100 milliseconds. [Section 4.1.3-pops]*/
        LDIT(PSECOND);

        /* Unblock ALL pcbs blocked on the Pseudo-clock semaphore */
        while(headBlocked(&(dSemaphores[MAXSEM - 1])) != NULL){
            pcb_PTR unblockedP = removeBlocked(&dSemaphores[MAXSEM - 1]);
            if(unblockedP != NULL){
                
                unblockedP->p_semAdd = NULL;

                cpu_t endTime;
                STCK(endTime);
                currentProcess->p_time = endTime - startTime;

                if(unblockedP->p_prio == PROCESS_PRIO_LOW)  // setting process status to ready
                    insertProcQ(&low_priority_queue, unblockedP);
                else
                    insertProcQ(&high_priority_queue, unblockedP);

                sbCount--;                              // decreasing number of sb processes
            }
        }

        if(currentProcess == NULL)
            scheduler();
        else{
            klog_print("sto per fare il LDST\n");
            processor_state->pc_epc += WORDLEN;
            LDST(processor_state);
            klog_print("ho fatto la LDST\n");
        }
    }
    else{   // Non-Timer Interrupts
        klog_print("devicessss\n");

        devregarea_t *deviceRegs = (devregarea_t*) RAMBASEADDR;     // TODO: problema di casting?
        unsigned int bit_check = 1;
        for(int i = 0; i < DEVPERINT; i++){         // DEVPERINT -> devices per interrupt = 8
            if(deviceRegs->interrupt_dev[line-3] & bit_check)
                manageNTInt(line, i);
            bit_check = bit_check << 1;
        }
    }
}

void manageNTInt(int line, int dev){

    /* Calculate the address for this device’s device register. [Section 5.1-pops] */
    /* one can compute the starting address of the device’s device register:
    devAddrBase = 0x1000.0054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10) */
    devreg_t* devAddrBase = (devreg_t*) (0x10000054 + ((line - 3) * 0x80) + (dev * 0x10));

    int receive_interr = 0;     
    unsigned int status;

    if (line == 7){ // if it's a terminal sub device
        klog_print("it's a terminal\n");
        termreg_t* terminalRegister = (termreg_t*) devAddrBase;

        if(terminalRegister->recv_status != READY){     // terminal READ
            status = terminalRegister->recv_status;     // Save off the status code from the device’s device register
            terminalRegister->recv_command = ACK;       // Acknowledge the interrupt    
            receive_interr = 1;
        }                       
        else{                                           // terminal WRITE
            status = terminalRegister->recv_status;     // Save off the status code from the device’s device register
            terminalRegister->transm_command = ACK;     // Acknowledge the interrupt 
        }

        if (receive_interr == 1)
            dev += 8;

    }
    else {
        devAddrBase->dtp.command = ACK;             // Acknowledge the interrupt 
        status = devAddrBase->dtp.status;           // Save off the status code from the device’s device register
    }

    // Semaphore associated with this (sub)device
    int semAdd = dSemaphores[dev];

    // Perform a V operation on the Nucleus
    dSemaphores[semAdd]++;
    pcb_PTR unblockedProcess = removeBlocked(&dSemaphores[semAdd]);

    if(unblockedProcess !=  NULL){
        // Place the stored off status code in the newly unblocked pcb’s v0 register.
        unblockedProcess->p_s.reg_v0 = status;

        unblockedProcess->p_semAdd = NULL;
        //unblockedProcess->p_time += (CURRENT_TOD - interrTime);
        
        cpu_t endTime;
        STCK(endTime);
        currentProcess->p_time = endTime - startTime;

        // decreasing number of sb processes
        sbCount--; 

        // Insert the newly unblocked pcb on the Ready Queue
        if(unblockedProcess->p_prio == PROCESS_PRIO_LOW)  
            insertProcQ(&low_priority_queue, unblockedProcess);
        else
            insertProcQ(&high_priority_queue, unblockedProcess);
    }

    if(currentProcess == NULL)          // if there was no process running
        scheduler();
    else{
        LDST(&(currentProcess->p_s));  // load old processor state
    }
}


