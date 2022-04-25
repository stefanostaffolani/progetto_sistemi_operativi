#include "syscall.h"

extern struct list_head semd_h;

void Create_Process_NSYS1(state_t *except_state) {
//alloco un PCB per il nuovo processo

    pcb_PTR newProcess = allocPcb();

    if(newProcess == NULL){     //new process cant be created, -1 in caller's v0 register
        except_state->reg_v0 = -1;
    } else {
        //id del processo appena creato e' l'inidirizzo della struttura pcb_t corrispondente
        newProcess->p_pid = (memaddr) newProcess;
        //to say the new process could be created
        processor_state->reg_v0 = newProcess->p_pid;
        //take the initial state from the a1 register
        memcpy(&(newProcess->p_s), ((state_t *) except_state->reg_a1), sizeof(state_t));
        //take the process priority from the a2 register
        newProcess->p_prio = except_state->reg_a2;
        //take the pointer to a structure containing the additional Support Level fields from the a3 register
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
        prCount++;
    }

    //control returned to the Current Process
    except_state->pc_epc += WORD_SIZE;
    except_state->reg_t9 += WORD_SIZE;
    LDST(except_state);
}

/* NSYS2 */
void Terminate_Process_NSYS2(int pid, state_t *except_state) {
    except_state->pc_epc += WORD_SIZE;
    except_state->reg_t9 += WORD_SIZE;

    if(pid == 0){
        //recursively terminate all progeny of the process        
        terminateProgeny(currentProcess);
    } else { //elimino il processo con il pid indicato
        
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

        terminateProgeny(proc);
    }    

    currentProcess = NULL;

    scheduler();
}

void terminateProgeny(pcb_t* removeMe){
    if (removeMe == NULL) return;

    pcb_PTR p;
    list_for_each_entry(p, &removeMe->p_child, p_sib){
        terminateProgeny(p);
    }

    terminateSingleProcess(removeMe);
}

void terminateSingleProcess(pcb_t* removeMe){
    outChild(removeMe);
    prCount--;
    int *sem = removeMe->p_semAdd;
    pcb_PTR proc;
    if(removeMe->p_prio == PROCESS_PRIO_LOW)
        proc = outProcQ(&low_priority_queue, removeMe);
    else
        proc = outProcQ(&high_priority_queue, removeMe);
    if (proc == NULL){   // non e' sulla readyqueue
        proc = outBlocked(removeMe);
        if (proc != NULL){
            if ((&(dSemaphores[0]) <= sem) && (sem <= &(dSemaphores[MAXSEM-1]))){
                sbCount--;
            }else
                *(sem)++;
        }
    }
    freePcb(removeMe);
}

void Passeren_NSYS3(int *semAddr, state_t *except_state) {
    
    // se il semaforo è 0 blocca
    if(*semAddr == 0){
        set_time(currentProcess, startTime);
        except_state->pc_epc += WORD_SIZE;
        except_state->reg_t9 += WORD_SIZE;
        memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
        insertBlocked(semAddr, currentProcess);
        currentProcess = NULL;
        scheduler();
    } // altrimenti non ci sono processi bloccati
    else if(headBlocked(semAddr) == NULL) {
        (*semAddr)--;
        except_state->pc_epc += WORD_SIZE;
        except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
    else{   // ci sono processi bloccati (dalla verhogen)
        pcb_PTR proc = removeBlocked(semAddr);
        insert_to_readyq(proc);
        except_state->pc_epc += WORD_SIZE;
        except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
}

void Verhogen_NSYS4(int *semAddr, state_t *except_state) {

    // speculare alla passeren
    if(*semAddr == 1){
        set_time(currentProcess, startTime);
        insertBlocked(semAddr, currentProcess);
        except_state->pc_epc += WORD_SIZE;
        except_state->reg_t9 += WORD_SIZE;
        memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
        currentProcess = NULL;
        scheduler();
    }
    else if(headBlocked(semAddr) == NULL) { 
        (*semAddr)++;
        except_state->pc_epc += WORD_SIZE;
        except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
    else{
        pcb_PTR runningProcess = removeBlocked(semAddr);
        insert_to_readyq(runningProcess);
        except_state->pc_epc += WORD_SIZE;
        except_state->reg_t9 += WORD_SIZE;
        LDST(except_state);
    }
}

void DO_IO_Device_NSYS5(state_t *except_state) {
    // read the interrupt line number in register a1
    int* cmdAddr = (int *) except_state->reg_a1;
    //read the device number in register a2
    int cmdValue = except_state->reg_a2;
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

    // trovo il semaforo
    int sem_loc = devNum + (DEVPERINT * intLine);
    int *semAdd = &dSemaphores[sem_loc];

    //perform a P operation and always block the Current Process on the ASL
    sbCount++;

    *semAdd = 0;
    Passeren_NSYS3(semAdd, except_state);
}

void Get_CPU_Time_NSYS6(state_t *except_state){
    set_time(currentProcess, startTime);
    except_state->reg_v0 = currentProcess->p_time;
    except_state->pc_epc += WORD_SIZE;
    except_state->reg_t9 += WORD_SIZE;
    LDST(except_state);
}

void Wait_For_Clock_NSYS7(state_t *except_state){
    dSemaphores[MAXSEM-1] = 0;
    sbCount++;
    Passeren_NSYS3(&(dSemaphores[MAXSEM-1]), except_state);
    scheduler();
}

void Get_SUPPORT_Data_NSYS8(state_t *except_state){
    except_state->reg_v0 = (unsigned int) currentProcess->p_supportStruct;
    except_state->pc_epc += WORD_SIZE;
    except_state->reg_t9 += WORD_SIZE;
    LDST(except_state);
}

void Get_Process_ID_NSYS9(state_t *except_state, int parent){
    if (parent){
        if(currentProcess->p_parent == NULL)
            except_state->reg_v0 = 0;
        else
            except_state->reg_v0 = currentProcess->p_parent->p_pid;
    }else{
        except_state->reg_v0 = currentProcess->p_pid;
    }
    except_state->pc_epc += WORD_SIZE;
    except_state->reg_t9 += WORD_SIZE;
    LDST(except_state);
}

void Yield_NSYS10(state_t *except_state){
    except_state->pc_epc += WORD_SIZE;
    except_state->reg_t9 += WORD_SIZE;
    memcpy(&(currentProcess->p_s), except_state, sizeof(state_t));
    insert_to_readyq(currentProcess);
    set_time(currentProcess, startTime);
    scheduler();
}
