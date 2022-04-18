#include "interrupt.h"

// extern struct list_head high_priority_queue;
// extern struct list_head low_priority_queue;
extern pcb_PTR currentProcess;
extern int sbCount;
extern int dSemaphores[MAXSEM];
// extern cpu_t insertTime;

cpu_t interval_timer;

void interrupt_exception(unsigned int cause){
 
    // TODO: pulire questi commenti 
    // TODO: verificare le operazioni bit a bit
    /*takes
    an unsigned integer as its input parameter and populates it with the value of
    the low-order word of the TOD clock divided by the Time Scale*/
    STCK(interval_timer);   //save the interrupt starting time

    /*Which interrupt lines have pending interrupts is set in Cause.IP*/
    // devo prendere i bit di cause da 2^9 a 2^16
    int bit_check = 512; 
    // for each line manage the interrupt
    for (int i = 1; i < 8; i++) {
        if (cause & bit_check)
            manageInterr(i);
        bit_check *= 2;
    }
}

void manageInterr(int line){
    if(line == 1){  // plt timer interrupt
        
        /* Acknowledge the PLT interrupt by loading the timer with a new value.
        [Section 4.1.4-pops]*/ 
        setTIMER(PSECOND);
        
        /*Save off the complete processor state at the time of the exception in a BIOS
        data structure on the BIOS Data Page. For Processor 0, the address of this
        processor state is 0x0FFF.F000.*/
        currentProcess->p_s = *((state_t*) BIOSDATAPAGE);
        
        currentProcess->p_time = currentProcess->p_time + (getTIMER() - currentProcess->p_time);

        /* Place the Current Process on the Ready Queue */
        if(currentProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, currentProcess);
        else
            insertProcQ(&high_priority_queue, currentProcess);

        scheduler();
    }
    else if(line == 2){  // reload interval timer

        /*Acknowledge the interrupt by loading the Interval Timer with a new
        value: 100 milliseconds. [Section 4.1.3-pops]*/
        LDIT(PSECOND);

        /* Unblock ALL pcbs blocked on the Pseudo-clock semaphore */
        while(headBlocked(&(dSemaphores[MAXSEM - 1])) != NULL){
            pcb_PTR unblockedP = removeBlocked(&dSemaphores[MAXSEM - 1]);
            if(unblockedP != NULL){
                
                unblockedP->p_semAdd = NULL;

                currentProcess->p_time = currentProcess->p_time + (getTIMER() - currentProcess->p_time);

                if(unblockedP->p_prio == PROCESS_PRIO_LOW)  // setting process status to ready
                    insertProcQ(&low_priority_queue, unblockedP);
                else
                    insertProcQ(&high_priority_queue, unblockedP);

                sbCount--;                              // decreasing number of sb processes
            }
        }

        if(currentProcess == NULL)
            scheduler();
        else 
            LDST((state_t*) BIOSDATAPAGE);
    }
    else{   // Non-Timer Interrupts

        devregarea_t *deviceRegs = RAMBASEADDR;     // TODO: problema di casting?
        unsigned int bit_check = 1;
        for(int i = 0; i < DEVPERINT; i++){         // DEVPERINT -> devices per interrupt = 8
            if(deviceRegs->interrupt_dev[line-3] & bit_check)
                manageNTInt(line, i);
            bit_check *= 2;
        }
    }
}

void manageNTInt(int line, int dev){

    /* Calculate the address for this device’s device register. [Section 5.1-pops] */
    /* one can compute the starting address of the device’s device register:
    devAddrBase = 0x1000.0054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10) */
    devreg_t* devAddrBase = (devreg_t*) 0x10000054 + ((line - 3) * 0x80) + (dev * 0x10);

    int receive_interr = 0;     
    unsigned int status;

    if (line == 7){ // if it's a terminal sub device
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
        dev = 2*dev + receive_interr;

    }
    else {
        devAddrBase->dtp.command = ACK;             // Acknowledge the interrupt 
        status = devAddrBase->dtp.status;           // Save off the status code from the device’s device register
    }

    // Semaphore associated with this (sub)device
    int semAdd = (line - 3) * 8 + dev;

    // Perform a V operation on the Nucleus
    dSemaphores[semAdd]++;
    pcb_PTR unblockedProcess = removeBlocked(&dSemaphores[semAdd]);

    if(unblockedProcess !=  NULL){
        // Place the stored off status code in the newly unblocked pcb’s v0 register.
        unblockedProcess->p_s.reg_v0 = status;

        unblockedProcess->p_semAdd = NULL;
        //unblockedProcess->p_time += (CURRENT_TOD - interrTime);
        
        currentProcess->p_time = currentProcess->p_time + (getTIMER() - currentProcess->p_time);

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
    else
        LDST((state_t*) BIOSDATAPAGE);  // load old processor state
}


