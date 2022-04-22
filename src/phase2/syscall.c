#include "syscall.h"

void Create_Process_NSYS1(state_t *except_state) {
//alloco un PCB per il nuovo processo
    pcb_PTR newProcess = allocPcb();
    if(newProcess == NULL){     //new process cant be created, -1 in caller's v0 register
        except_state->reg_v0 = -1;
    } else {
        //id del processo appena creato e' l'inidirizzo della struttura pcb_t corrispondente
        newProcess->p_pid = (memaddr) newProcess;
        klog_print_hex(newProcess->p_pid);
        breakpoint();
        //to say the new process could be created
        processor_state->reg_v0 = newProcess->p_pid;
        //take the initial state from the a1 register
        newProcess->p_s = *((state_t*) except_state->reg_a1);
        //take the process priority from the a2 register
        newProcess->p_prio = except_state->reg_a2;
        //take the pointer to a structure containing the additional Support Level fields from the a3 register
        newProcess->p_supportStruct = (support_t*) except_state->reg_a3;
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
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

/* NSYS2 */
void Terminate_Process_NSYS2(int pid, state_t *except_state) {
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

void Passeren_NSYS3(int *semAddr, state_t *except_state) {
    
    klog_print("entro nella passeren...\n");
    if(*semAddr == 0){
        cpu_t endTime;
        STCK(endTime);
        currentProcess->p_time = endTime - startTime;
        insertBlocked(semAddr, currentProcess);
        currentProcess->p_s = *except_state;
        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) { 
        *semAddr--;
        except_state->pc_epc += WORD_SIZE;
        LDST(except_state);
    }
    else{
        pcb_PTR runningProcess = removeBlocked(semAddr);
        if(runningProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, runningProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, runningProcess); //and is placed in the Ready Queue
        except_state->pc_epc += WORD_SIZE;
        LDST(except_state);
    }
}

void Verhogen_NSYS4(int *semAddr, state_t *except_state) {
    //physical address of the semaphore in a1
    klog_print("entro nella veroghen...\n");
    if(*semAddr == 1){
        cpu_t endTime;
        STCK(endTime);
        currentProcess->p_time = endTime - startTime;
        insertBlocked(semAddr, currentProcess);
        currentProcess->p_s = *except_state;
        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) { 
        *semAddr++;
        except_state->pc_epc += WORD_SIZE;
        LDST(except_state);
    }
    else{
        pcb_PTR runningProcess = removeBlocked(semAddr);
        if(runningProcess->p_prio == PROCESS_PRIO_LOW)
            insertProcQ(&low_priority_queue, runningProcess); //and is placed in the Ready Queue
        else
            insertProcQ(&high_priority_queue, runningProcess); //and is placed in the Ready Queue
        except_state->pc_epc += WORD_SIZE;
        LDST(except_state);
    }
}

void DO_IO_Device_NSYS5(state_t *except_state) {
    except_state->pc_epc += WORD_SIZE;
    currentProcess->p_s = *except_state;
    klog_print("entro nella DOIO\n");
    // read the interrupt line number in register a1
    int* cmdAddr = except_state->reg_a1;
    //read the device number in register a2
    int cmdValue = except_state->reg_a2;
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
    //perform a P operation and always block the Current Process on the ASL
    sbCount++; 
    Passeren_NSYS3(semAdd, except_state);
    klog_print("finita la DOIO\n");
}

void NSYS6_Get_CPU_Time(state_t *except_state){
    cpu_t endTime;
    STCK(endTime);
    currentProcess->p_time = endTime - startTime;
    except_state->reg_v0 = currentProcess->p_time;
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

void NSYS7_Wait_For_Clock(state_t *except_state){
    klog_print("entro nella NSYS7...\n");
    cpu_t endTime;
    STCK(endTime);
    currentProcess->p_time = endTime - startTime;
    insertBlocked(&dSemaphores[MAXSEM-1], currentProcess);
    sbCount++;
    except_state->pc_epc += WORD_SIZE; 
    currentProcess->p_s = *except_state;
    klog_print("sto per chiamare lo scheduler\n");
    scheduler();

}

void NSYS8_Get_SUPPORT_Data(state_t *except_state){
    except_state->reg_v0 = (unsigned int) currentProcess->p_supportStruct;
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

void NSYS9_Get_Process_ID(state_t *except_state){
    if(currentProcess->p_parent == 0){
        except_state->reg_v0 = currentProcess->p_pid;
    }
    else{
        except_state->reg_v0 = currentProcess->p_parent->p_pid;
    }
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

void NSYS10_Yield(state_t *except_state){
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
