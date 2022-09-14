#include "../include/exception_support.h"

void exception_handler_support(){
    support_t *support = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    state_t *except_state = &support->sup_exceptState[GENERALEXCEPT];
    int cause = CAUSE_GET_EXCCODE(except_state->cause);
    switch (cause){
    case 8:       // syscall exception
        syscall_exception_handler_support(support, except_state);
        break;
    default:
        program_trap_exception_handler(support);
        break;
    }
    except_state->pc_epc += WORDLEN;
    except_state->reg_t9 += WORDLEN;
    LDST(except_state);
}

void syscall_exception_handler_support(support_t *support, state_t *except_state){
    const int a0 = except_state->reg_a0;
    int retval;
    switch (a0){
    case 1:                             // Get_TOD
        Get_TOD_SYS1(except_state);
        break;
    case 2:                             // Terminate
        Terminate_SYS2(support);
        break;
    case 3:                             // Write_to_Printer
        retval = Write_to_Printer_SYS3(support);
        except_state->reg_v0 = retval;
        break;
    case 4:                             // Write_to_Terminal
        retval = Write_to_Terminal_SYS4(support);
        except_state->reg_v0 = retval;    
        break;
    case 5:                             // Read_from_Terminal
        retval = Read_from_Terminal_SYS5(support);
        except_state->reg_v0 = retval;
        break;
    default:                            // kill process ==> this is an error
        program_trap_exception_handler(support);
        break;
    }
    
}

void program_trap_exception_handler(support_t *support){   // Terminate process with SYS2 (do V op in sem_swap)
    Terminate_SYS2(support);
}

void Get_TOD_SYS1(state_t *except_state){
    cpu_t TOD;
    STCK(TOD);
    except_state->reg_v0 = TOD;
}

void Terminate_SYS2(support_t *support){   // capire se va incrementato il PC anche qua (visto che viene fatto dal kernel)
    if(get_swap_asid(support->sup_asid)){
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);    // controllare se aggiusta da solo il semaforo
        for(int i = 0; i < POOLSIZE; i++){
            if(swap_pool[i].sw_asid == support->sup_asid)    // ottimizzazione
                swap_pool[i].sw_asid = NOPROC;
        }
        update_swap_asid(0,support->sup_asid);
    }
    SYSCALL(VERHOGEN, (int)&master_semaphore, 0, 0);
    SYSCALL(TERMPROCESS,0,0,0);
}

int Write_to_Printer_SYS3(support_t *support){
    size_t len = (size_t)support->sup_exceptState[GENERALEXCEPT].reg_a1;
    char *c = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a2;      // stringa da scrivere
    //int retval;
    dtpreg_t *device = (dtpreg_t *)DEV_REG_ADDR(IL_PRINTER, support->sup_asid-1);     // il device e' asid-1 perche' i device sono 8 (da 0 a 7) e i processi sono 8 (da 1 a 8)
    SYSCALL(PASSEREN,(int)&sem_write_printer,0,0);                         // mutua esclusione per chiamare la DOIO
    for(size_t i = 0; i < len; i++){
        device->data0 = c[i];
        int status = SYSCALL(DOIO, (int)&device->command, PRINTCHR, 0);
        if (status != READY){
            SYSCALL(VERHOGEN, (int)&sem_write_printer, 0, 0);
            return -status;
        }
    }
    SYSCALL(VERHOGEN,(int)&sem_write_printer,0,0);
    return len;
}

int Write_to_Terminal_SYS4(support_t *support){
    size_t len = (size_t)support->sup_exceptState[GENERALEXCEPT].reg_a1;
    char *c = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a2;
    dtpreg_t *device = (dtpreg_t *)DEV_REG_ADDR(IL_TERMINAL, support->sup_asid-1);
    SYSCALL(PASSEREN, (int)&sem_write_terminal, 0, 0);
    for(size_t i = 0; i < len; i++){
        memaddr value = PRINTCHR | (memaddr)(c[i] << 8);       // pops 5.7 transmitted char non sono i primi 8 bit ma i secondi da dx
        int status = SYSCALL(DOIO, (int)&((termreg_t *)device)->transm_command, (int)value, 0);
        if((status & 0x000000FF) != READY){         // 0x000000FF mi isola il primo byte
            SYSCALL(VERHOGEN,(int)&sem_write_terminal,0,0);
            return -status;
        }
    }
    SYSCALL(VERHOGEN,(int)&sem_write_terminal,0,0);
    return len;
}

int Read_from_Terminal_SYS5(support_t *support){
    char *buffer = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a1;
    //size_t len = support->sup_exceptState[GENERALEXCEPT].reg_a2;
    termreg_t *terminal = (termreg_t *) DEV_REG_ADDR(IL_TERMINAL, support->sup_asid-1);
    SYSCALL(PASSEREN, (int)&sem_read_terminal, 0, 0);
    size_t i = 0;
    char c = '\0';
    while(c != '\n'){
        int status = SYSCALL(DOIO, (int)&(terminal->recv_command), RECVCHAR, 0);   // cfr capitolo 5.7 pops
        if((status & 0x000000FF) != READY){     // controllare se usare ready o RECVCHAR (5)
            SYSCALL(VERHOGEN, (int)&sem_read_terminal, 0, 0);
            return -status;
        }
        c = (char) (0x0000FF00 & status);
        *(buffer + i) = c;
        i++;
    }
    SYSCALL(VERHOGEN, (int)&sem_read_terminal, 0, 0);
    *(buffer + i) = '\0';                      // controllare se necessario
    return i;
}