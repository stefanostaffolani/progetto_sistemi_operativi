#include "syscall.h"

extern state_t* processor_state;
extern struct list_head high_priority_queue;
extern struct list_head low_priority_queue;
extern int dSemaphores[MAXSEM];
extern cpu_t insertTime; 

void Create_Process_NSYS1() {
//alloco un PCB per il nuovo processo
    pcb_PTR newProcess = allocPcb();
if(newProcess == NULL){     //new process cant be created, -1 in caller's v0 register
    processor_state->reg_v0 = -1;
} else {
    //id del processo appena creato e' l'inidirizzo della struttura pcb_t corrispondente
    newProcess->p_pid = (memaddr) newProcess; //e' giusto cosi'?
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
    LDST(processor_state);
}

/* NSYS2 */
void Terminate_Process_NSYS2(int pid) {
    if(pid == 0){
        outChild(currentProcess);
        //recursively terminate all progeny of the process
        terminateProgeny(currentProcess);
        currentProcess = NULL;
    } else { //elimino il processo con il pid indicato
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
        removeProgeny(removeChild(removeMe));
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

void Passeren_NSYS3() {
    //physical address of the semaphore in a1
    int* semAddr = processor_state->reg_a1;
    if (*semAddr == 1/*valore del semaforo non e' bloccante*/){
        *semAddr = 0;
        insertBlocked(semAddr, currentProcess); //Current Process is blocked on the ASL
        scheduler();
    } else {
        LDST(processor_state); //control is returned to the Current Process
    }
}

void Verhogen_NSYS4() {
    //physical address of the semaphore in a1
    int* semAddr = processor_state->reg_a1;
    *semAddr = 1;
    pcb_PTR runningProcess = removeBlocked(semAddr);    //change blocked process to running
    if(runningProcess != NULL){ //if I actually have freed a process
        runningProcess->p_semAdd = NULL;    //that process has no semaphore
        if(runningProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, runningProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, runningProcess); //and is placed in the Ready Queue
    }
    LDST(processor_state); //control is returned to the Current Process
}

void DO_IO_Device_NSYS5() {
    // read the interrupt line number in register a1
    int* cmdAddr = processor_state->reg_a1;
    //read the device number in register a2
    int cmdValue = processor_state->reg_a2;
    *cmdAddr = cmdValue;
    //I need the semaphore that the nucleus maintains for the I/O device indicated by the value in a1
    
    //perform a P operation and always block the Current Process on the ASL
    *(sem) = 0;
    insertBlocked(sem, currentProcess);
    sbCount++;
    currentProcess->p_s = *processor_state;
    //the scheduler is called
    scheduler();
}

void NSYS6_Get_CPU_Time(){
    currentProcess->p_time = currentProcess->p_time + (getTIMER() - insertTime);
    processor_state->reg_v0 = currentProcess->p_time;
    STCK(insertTime);
    LDST(processor_state);
}

void NSYS7_Wait_For_Clock(){
    sbCount++;
    NSYS3_Passeren(&(dSemaphores[MAXSEM-1]));
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
}

void NSYS10_Yield(){
    if(currentProcess->p_prio == PROCESS_PRIO_LOW)
        insertProcQ(&low_priority_queue, currentProcess);
    else
        insertProcQ(&high_priority_queue, currentProcess);    
    scheduler();
}