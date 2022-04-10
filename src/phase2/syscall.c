#include "syscall.h"

void Create_Process_SYS1() {
//alloco un PCB per il nuovo processo
    pcb_PTR newProcess = allocPcb();
//initial state in a1 register

if(newProcess == NULL){     //new process cant be created, -1 in caller's v0 register
    exceptState->reg_v0 = -1;
} else {
    //newly populated pcb is placed on the Ready Queue...
    insertProcQ(&readyQueue, newProcess);
    //...and is made a child of the Current Process
    insertChild(currentProcess, newProcess);
    //Process Count is incremented by one
    prCount++;
    //control returned to the Current Process
    LDST(exceptState);
}

}

void Terminate_Process_SYS2() {
    outChild(currentProcess);
    //recursively terminate all progeny of the process

    currentProcess = NULL;
    scheduler();
}

void Passeren_SYS3() {
    if (/*valore del semaforo non e' bloccante*/){
        *semAddr = ;//valore bloccante
        insertBlocked(semAddr, currentProcess); //Current Process is blocked on the ASL
        scheduler();
    } else {
        LDST(exceptState); //control is returned to the Current Process
    }
}

void Verhogen_SYS4() {
    *semAddr = ;//valore non bloccante
    pcb_PTR runningProcess = removeBlocked(semAddr);    //change blocked process to running
    if(runningProcess != NULL){ //if I actually have freed a process
        runningProcess->p_semAdd = NULL;    //that process has no semaphore
        insertProcQ(&readyQueue, runningProcess); //and is placed in the Ready Queue
    }
    LDST(exceptState); //control is returned to the Current Process
}