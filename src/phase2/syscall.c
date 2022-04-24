#include "syscall.h"

extern struct list_head semd_h;

//int check = 0;

void Create_Process_NSYS1(state_t *except_state) {
//alloco un PCB per il nuovo processo

    // klog_print("bella vezz\n");
    pcb_PTR newProcess = allocPcb();
    // klog_print("non me loasd\n");

    if(newProcess == NULL){     //new process cant be created, -1 in caller's v0 register
        except_state->reg_v0 = -1;
    } else {
        //id del processo appena creato e' l'inidirizzo della struttura pcb_t corrispondente
        newProcess->p_pid = (memaddr) newProcess;
        klog_print_hex(newProcess->p_pid);
        //breakpoint();
        //to say the new process could be created
        processor_state->reg_v0 = newProcess->p_pid;
        //take the initial state from the a1 register
        memcpy(&(newProcess->p_s), ((state_t *) except_state->reg_a1), sizeof(state_t));
        //newProcess->p_s = *((state_t*) except_state->reg_a1);
        //take the process priority from the a2 register
        newProcess->p_prio = except_state->reg_a2;
        //take the pointer to a structure containing the additional Support Level fields from the a3 register
        //memcpy(&(newProcess->p_supportStruct), ((support_t*) except_state->reg_a3), sizeof(support_t));
        newProcess->p_supportStruct = (support_t*) except_state->reg_a3;
        //newly populated pcb is placed on the Ready Queue...
        insert_to_readyq(newProcess);

        //...and is made a child of the Current Process
        insertChild(currentProcess, newProcess);
        //the new process has yet to accumulate any cpu time
        newProcess->p_time = 0;
        //this process is in the "ready" state
        newProcess->p_semAdd = NULL;
        //Process Count is incremented by one
        klog_print("\ninc prC NSYS1\n");
        breakpoint();
        prCount++;
    }
    //control returned to the Current Process
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

/* NSYS2 */
void Terminate_Process_NSYS2(int pid, state_t *except_state) {
    if(pid == 0){
        //recursively terminate all progeny of the process
        terminateprogeny(currentProcess);
    } else { //elimino il processo con il pid indicato
        //klog_print_hex(pid);
        except_state->pc_epc += WORDLEN;
        currentProcess->p_s = *except_state;
        pcb_PTR proc;
        pcb_PTR currentPcb;
        semd_PTR currentSemd;


        list_for_each_entry(currentPcb, &low_priority_queue, p_list) {
            if (currentPcb->p_pid == pid) {
                proc = currentPcb;
            }
        }

        list_for_each_entry(currentPcb, &high_priority_queue, p_list) {
            if (currentPcb->p_pid == pid) {
                 proc = currentPcb;
            }
        }



        list_for_each_entry(currentSemd, &semd_h, s_link) {
            list_for_each_entry(currentPcb, &currentSemd->s_procq, p_list) {
                if (currentPcb->p_pid == pid) {
                    proc = currentPcb;
                }
            }
        }


        // for(int i = 0; i < MAXSEM-1; i++){
        //     list_for_each_entry(currentPcb, &dSemaphores[i], p_list) {
        //         if (currentPcb->p_pid == pid) {
        //             proc = currentPcb;
        //         }
        //     }
        // }
        terminateprogeny(proc);
    }
    if (currentProcess != NULL){
    set_time(currentProcess, startTime);
        LDST(except_state);
    }
    scheduler();
}
void terminateprogeny(pcb_t* removeMe){
    struct list_head *iterator;
    list_for_each(iterator, &removeMe->p_child){
        pcb_t* child = container_of(iterator, pcb_t, p_sib);
        terminateprogeny(child);
    }
    outChild(removeMe);

    if(currentProcess->p_pid == removeMe->p_pid){
        currentProcess = NULL;
    } else if (removeMe->p_semAdd != 0){
        if((&(dSemaphores[0]) <= removeMe->p_semAdd) && (removeMe->p_semAdd <= &(dSemaphores[0]) + 49 * sizeof(int))){
            sbCount--;
        }
        outBlocked(removeMe);
    } else {
            if(removeMe->p_prio == PROCESS_PRIO_LOW)
        removeMe = outProcQ(&low_priority_queue, removeMe);
    else
        removeMe = outProcQ(&high_priority_queue, removeMe);   
        }
        freePcb(removeMe);
        prCount--;
}
void terminateProgeny(pcb_t* removeMe){
    if (removeMe == NULL) return;
    while (!(emptyChild(removeMe))){
        terminateProgeny(removeChild(removeMe));
    }
    terminateSingleProcess(removeMe);
}

void terminateSingleProcess(pcb_t* removeMe){
    // klog_print("dec prC terminate\n");
    breakpoint();
    // klog_print("\n");
    // klog_print_hex(prCount);
    // klog_print("\n");
    // klog_print_hex(sbCount);
    // klog_print("\n");
    if (currentProcess->p_pid == removeMe->p_pid && currentProcess != NULL){
        currentProcess = NULL;
    }
    prCount--;
    pcb_PTR proc;
    if(removeMe->p_prio == PROCESS_PRIO_LOW)
        proc = outProcQ(&low_priority_queue, removeMe);
    else
        proc = outProcQ(&high_priority_queue, removeMe);
    if (proc == NULL){   // non e' sulla readyqueue
        if ((&(dSemaphores[0]) <= removeMe->p_semAdd) && (removeMe->p_semAdd <= &(dSemaphores[MAXSEM-1]))){
            // klog_print("dec sbC terminate\n");
            // semd_PTR sm = container_of(removeMe->p_semAdd, semd_t, s_key);
            // proc = outProcQ(sm->s_procq, removeMe);
            proc = outBlocked(removeMe);
            //breakpoint();
            if (proc != NULL)
                sbCount--;
        }else
            *(removeMe->p_semAdd)++;
    }
    // klog_print("\n");
    // klog_print_hex(prCount);
    // klog_print("\n");
    // klog_print_hex(sbCount);
    // klog_print("\n");
    freePcb(removeMe);



    // if(*(removeMe->p_semAdd) == 0){
    //     //capire se e' bloccato su un device semaphore o no, se non e' un device semaphore allora
    //     if (&(dSemaphores[0]) <= removeMe->p_semAdd && removeMe->p_semAdd <= &(dSemaphores[MAXSEM-1])){
    //         // klog_print("dec sbC terminate\n");
    //         breakpoint();
    //         sbCount--;
    //     }
    //     else
    //         (*(removeMe->p_semAdd))++;
    //     //altrimenti non faccio niente (cambio solo i soft-blocked count) perche' quando succedera' l'interrupt il semaforo verra' V'ed
    //     outBlocked(removeMe);
    // }
    // freePcb(removeMe);
}

void Passeren_NSYS3(int *semAddr, state_t *except_state) {
    
    //klog_print("entro nella passeren...\n");
    if(*semAddr == 0){
        set_time(currentProcess, startTime);
        except_state->pc_epc += WORD_SIZE;
        memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
        //currentProcess->p_s = *except_state;
        insertBlocked(semAddr, currentProcess);
        currentProcess = NULL;
        //klog_print("passeren scheduler");
        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) {
        (*semAddr)--;
        except_state->pc_epc += WORD_SIZE;
        //except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
    else{
        pcb_PTR proc = removeBlocked(semAddr);
        insert_to_readyq(proc);
        except_state->pc_epc += WORD_SIZE;
        //except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
}

void Verhogen_NSYS4(int *semAddr, state_t *except_state) {
    //physical address of the semaphore in a1
    //klog_print("entro nella veroghen...\n");
    if(*semAddr == 1){
        set_time(currentProcess, startTime);
        insertBlocked(semAddr, currentProcess);
        except_state->pc_epc += WORD_SIZE;
        memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
        //currentProcess->p_s = *except_state;
        currentProcess = NULL;
        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) { 
        (*semAddr)++;
        except_state->pc_epc += WORD_SIZE;
        //except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
    else{
        pcb_PTR runningProcess = removeBlocked(semAddr);
        insert_to_readyq(runningProcess);
        except_state->pc_epc += WORD_SIZE;
        //except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
}

void DO_IO_Device_NSYS5(state_t *except_state) {
    
    // klog_print("entro nella DOIO\n");
    // read the interrupt line number in register a1
    int* cmdAddr = (int *) except_state->reg_a1;
    //read the device number in register a2
    int cmdValue = except_state->reg_a2;
    // klog_print("cmdValue is: ");
    // klog_print_hex(cmdValue);
    *cmdAddr = cmdValue;
    //I need the semaphore that the nucleus maintains for the I/O device indicated by the value in a1

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

    // klog_print("sem_loc: \n");
    // klog_print_hex(sem_loc);
    // klog_print("\n");

    //check = *semAdd;
    //perform a P operation and always block the Current Process on the ASL
    // klog_print("inc sbC DOIO\n");
    sbCount++;
    //except_state->pc_epc += WORD_SIZE;   
    //except_state->reg_t9 += WORD_SIZE;
    //currentProcess->p_s = *except_state;  perche' lo fa la Passeren
    *semAdd = 0;
    // klog_print("arrivo effettivamente alla fine della funzione doio\n");
    Passeren_NSYS3(semAdd, except_state);
}

void NSYS6_Get_CPU_Time(state_t *except_state){
    set_time(currentProcess, startTime);
    except_state->reg_v0 = currentProcess->p_time;
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

void NSYS7_Wait_For_Clock(state_t *except_state){
    // klog_print("entro nella NSYS7...\n");

    dSemaphores[MAXSEM-1] = 0;
    sbCount++;
    klog_print("NSYS7 passeren\n");
    Passeren_NSYS3(&(dSemaphores[MAXSEM-1]), except_state);
    scheduler();
    //scheduler();
    // set_time(currentProcess, startTime);
    // klog_print("inc sbC NSYS7\n");
    // breakpoint();
    // except_state->pc_epc += WORD_SIZE;
    // memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
    // //currentProcess->p_s = *except_state;
    // insertBlocked(&dSemaphores[MAXSEM-1], currentProcess);
    // klog_print("sto per chiamare lo scheduler\n");
    // scheduler();
}

void NSYS8_Get_SUPPORT_Data(state_t *except_state){
    except_state->reg_v0 = (unsigned int) currentProcess->p_supportStruct;
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

void NSYS9_Get_Process_ID(state_t *except_state, int parent){
    if (parent){
        if(currentProcess->p_parent == NULL)
            except_state->reg_v0 = 0;
        else
            except_state->reg_v0 = currentProcess->p_parent->p_pid;
    }else{
        except_state->reg_v0 = currentProcess->p_pid;
    }
    except_state->pc_epc += WORD_SIZE;
    LDST(except_state);
}

void NSYS10_Yield(state_t *except_state){
    except_state->pc_epc += WORD_SIZE;
    memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
    insert_to_readyq(currentProcess);
    set_time(currentProcess, startTime);
    scheduler();
}
