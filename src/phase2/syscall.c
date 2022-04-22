#include "syscall.h"

void Create_Process_NSYS1() {
//alloco un PCB per il nuovo processo
    pcb_PTR newProcess = allocPcb();
    if(newProcess == NULL){     //new process cant be created, -1 in caller's v0 register
        processor_state->reg_v0 = -1;
    } else {
        //id del processo appena creato e' l'inidirizzo della struttura pcb_t corrispondente
        newProcess->p_pid = (memaddr) newProcess;
        klog_print_hex(newProcess->p_pid);
        breakpoint();
        //to say the new process could be created
        processor_state->reg_v0 = newProcess->p_pid;
        //take the initial state from the a1 register
        newProcess->p_s = *((state_t*) processor_state->reg_a1);
        //take the process priority from the a2 register
        newProcess->p_prio = processor_state->reg_a2;
        //take the pointer to a structure containing the additional Support Level fields from the a3 register
        newProcess->p_supportStruct = (support_t*) processor_state->reg_a3;
        //newly populated pcb is placed on the Ready Queue...
        if(newProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, newProcess);
        else
            insertProcQ(&high_priority_queue, newProcess);
        //...and is made a child of the Current Process
        insertChild(currentProcess, newProcess);
        //the new process has yet to accumulate any cpu time
        newProcess->p_time = 0;
        //this process is in the "ready" state
        newProcess->p_semAdd = NULL;
        //Process Count is incremented by one
        prCount++;
    }
    //control returned to the Current Process
    LDST(*((state_t *) BIOSDATAPAGE));
}

/* NSYS2 */
void Terminate_Process_NSYS2(int pid) {
    if(pid == 0){
        outChild(currentProcess);
        //recursively terminate all progeny of the process
        terminateProgeny(currentProcess);
        currentProcess = NULL;
    } else { //elimino il processo con il pid indicato
        klog_print_hex(pid);
        breakpoint();
        pcb_PTR proc = (memaddr) pid;
        outChild(proc);
        terminateProgeny(proc);
        proc = NULL;
    }
    scheduler();
}

void terminateProgeny(pcb_t* removeMe){
    if (removeMe == NULL) return;
    while (!(emptyChild(removeMe))){
        terminateProgeny(removeChild(removeMe));
    }
    terminateSingleProcess(removeMe);
}

void terminateSingleProcess(pcb_t* removeMe){
    prCount--;
    if(removeMe->p_prio == PROCESS_PRIO_LOW)
        outProcQ(&low_priority_queue, removeMe);
    else
        outProcQ(&high_priority_queue, removeMe);
    if(*(removeMe->p_semAdd) == 0){
        //capire se e' bloccato su un device semaphore o no, se non e' un device semaphore allora
        if (&(dSemaphores[0]) <= removeMe->p_semAdd && removeMe->p_semAdd <= &(dSemaphores[MAXSEM-1]))
            sbCount--;
        else
            *(removeMe->p_semAdd)++;
        //altrimenti non faccio niente (cambio solo i soft-blocked count) perche' quando succedera' l'interrupt il semaforo verra' V'ed
        outBlocked(removeMe);
    }
    freePcb(removeMe);
}

void Passeren_NSYS3(int *semAddr) {
    
    klog_print("entro nella passeren...\n");

    //int* semAddr = processor_state->reg_a1;

    if(*semAddr == 0){
        /*if(currentProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, currentProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, currentProcess); //and is placed in the Ready Queue

        processor_state->pc_epc += WORD_SIZE;
        LDST(processor_state); //control is returned to the Current Process
        */

        // processor_state->pc_epc += WORD_SIZE;

        cpu_t endTime;
        STCK(endTime);
        currentProcess->p_time = endTime - startTime;

        insertBlocked(semAddr, currentProcess);

        currentProcess->p_s = *processor_state;

        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) { 
        *semAddr--;
        LDST(processor_state);
    }
    else{
        pcb_PTR runningProcess = removeBlocked(semAddr);
        if(runningProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, runningProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, runningProcess); //and is placed in the Ready Queue
        LDST(processor_state);
    }


    // //physical address of the semaphore in a1
    // int* semAddr = processor_state->reg_a1;
    // if (*semAddr == 1/*valore del semaforo non e' bloccante*/){
    //     *semAddr = 0;
    //     insertBlocked(semAddr, currentProcess); //Current Process is blocked on the ASL
    //     scheduler();
    // } else {
    //     LDST(processor_state); //control is returned to the Current Process
    // }
    
    klog_print("esco nella passeren...\n");

}

void Verhogen_NSYS4(int *semAddr) {
    //physical address of the semaphore in a1
    klog_print("entro nella veroghen...\n");
    //int* semAddr = processor_state->reg_a1;

    if(*semAddr == 1){
        
        // currentProcess->p_s = *((state_t *) BIOSDATAPAGE); 

        cpu_t endTime;
        STCK(endTime);
        currentProcess->p_time = endTime - startTime;

        insertBlocked(semAddr, currentProcess);

       /*  if(currentProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, currentProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, currentProcess); //and is placed in the Ready Queue

        processor_state->pc_epc += WORD_SIZE;
        LDST(processor_state ); //control is returned to the Current Process
        */

        currentProcess->p_s = *processor_state;

        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) { 
        *semAddr++;
        LDST(processor_state);
    }
    else{
        pcb_PTR runningProcess = removeBlocked(semAddr);
        if(runningProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, runningProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, runningProcess); //and is placed in the Ready Queue
        LDST(processor_state);
    }
    //klog_print("esco dalla veroghen...\n");
}

void DO_IO_Device_NSYS5() {

    klog_print("entro nella DOIO\n");
    // read the interrupt line number in register a1
    int* cmdAddr = processor_state->reg_a1;
    //read the device number in register a2
    int cmdValue = processor_state->reg_a2;
    klog_print("cmdValue is: ");
    klog_print_hex(cmdValue);
    *cmdAddr = cmdValue;
    //I need the semaphore that the nucleus maintains for the I/O device indicated by the value in a1
    klog_print("sto per prendere il semaforo\n");

    int devNum;
    int intLine;

    devregarea_t *devReg = (devregarea_t*) RAMBASEADDR;     
    for(int i = 0; i < NUMDEV; i++){
        for(int j = 0; j < DEVPERINT; j++){
            if(i == NUMDEV-1){ // terminali 
                if(devReg->devreg[i][j].term.transm_command == *cmdAddr){
                    intLine = i;
                    devNum = j;
                }
                else if(devReg->devreg[i][j].term.recv_command == *cmdAddr){
                    intLine = i;    
                    devNum = j + DEVPERINT;     // to map the dev in dSemaphore
                }
            }
            else{   // altri devices
                if(devReg->devreg[i][j].dtp.command == *cmdAddr){
                    intLine = i;
                    devNum = j;
                }
            }
        }
    }

    int sem_loc = devNum + (DEVPERINT * intLine);
    int *semAdd = &dSemaphores[sem_loc];

    //klog_print("\nSYS LOC: ");
    //klog_print_hex(intLine);
    //klog_print("\nSYS LOC: ");

    //klog_print_hex(devNum);
    //klog_print("\nSYS LOC: ");

    //klog_print_hex(sem_loc);
    //processor_state->reg_a1 = semAdd;
    //Passeren_NSYS3(semAdd);
    // TODO: la faccio sta p operation?

    //perform a P operation and always block the Current Process on the ASL
    sbCount++; 
    Passeren_NSYS3(semAdd);

    // currentProcess->p_s = *processor_state;    
    //state_t *status = cmdAddr - WORDLEN;    // TODO: controllare questo valore poiche' manda in panic la print()
    //processor_state->reg_v0 = status->status;
    //the scheduler is called
    // scheduler();
    klog_print("finita la DOIO\n");
}

void NSYS6_Get_CPU_Time(){
    cpu_t endTime;
    STCK(endTime);
    currentProcess->p_time = endTime - startTime;
    processor_state->reg_v0 = currentProcess->p_time;
    LDST(processor_state);
}

void NSYS7_Wait_For_Clock(){
    klog_print("entro nella NSYS7...\n");

    
    //sbCount++;      // TODO: non ha senso sta cosa
    //Passeren_NSYS3(&(dSemaphores[MAXSEM-1]));

    cpu_t endTime;
    STCK(endTime);
    currentProcess->p_time = endTime - startTime;
    
    insertBlocked(&dSemaphores[MAXSEM-1], currentProcess);
    sbCount++; 
    
    currentProcess->p_s = *processor_state;
    
    klog_print("sto per chiamare lo scheduler\n");
    scheduler();

}

void NSYS8_Get_SUPPORT_Data(){
    processor_state->reg_v0 = (unsigned int) currentProcess->p_supportStruct;
    LDST(processor_state);
}

void NSYS9_Get_Process_ID(){
    if(currentProcess->p_parent == 0){
        processor_state->reg_v0 = currentProcess->p_pid;
    }
    else{
        processor_state->reg_v0 = currentProcess->p_parent->p_pid;
    }
    LDST(processor_state);
}

void NSYS10_Yield(){
    if(currentProcess->p_prio == PROCESS_PRIO_LOW)
        insertProcQ(&low_priority_queue, currentProcess);
    else
        insertProcQ(&high_priority_queue, currentProcess);    
    // currentProcess->p_s = *((state_t *) BIOSDATAPAGE);
    cpu_t endTime;
    STCK(endTime);
    currentProcess->p_time = endTime = startTime;
    scheduler();
}
