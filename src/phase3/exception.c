#include "pager.c"
#include "/usr/include/umps3/umps/arch.h"

#define PRINTCHR 2    // come definito in pops a pagina 39


void exception_handler(){
    support_t *support = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    state_t *except_state = &support->sup_exceptState[GENERALEXCEPT];
    int cause = CAUSE_GET_EXCCODE(except_state->cause);
    switch (cause){
    case 8:       // syscall exception
        syscall_exception_handler(support, except_state);
        break;
    default:
        program_trap_exception_handler(support);
        break;
    }
    except_state->pc_epc += WORDLEN;
    except_state->reg_t9 += WORDLEN;
    LDST(except_state);
}


void syscall_exception_handler(support_t *support, state_t *except_state){
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
        break;
    case 5:                             // Read_from_Terminal
        break;
    default:                            // kill process ==> this is an error
        program_trap_exception_handler(support);
        break;
    }
    
}


void program_trap_exception_handler(support_t *support){   // Terminate process with SYS2 (do V op in sem_swap)
    Terminate_SYS2();
}

void Get_TOD_SYS1(state_t except_state){
    cpu_t TOD;
    STCK(&TOD);
    except_state->reg_v0 = TOD;
}


void Terminate_SYS2(support_t *support){   // capire se va incrementato il PC anche qua (visto che viene fatto dal kernel)
    if(get_swap_asid(support->sup_asid)){
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);    // controllare se aggiusta da solo il semaforo
        for(int i = 0; i < POOLSIZE; i++){
            if(swap_pool[i] == support->sup_asid)    // ottimizzazione
                swap_pool[i] = -1;
        }
        update_swap_asid(0,support->sup_asid);
    }
    SYSCALL(TERMPROCESS,0,0,0);
}

int Write_to_Printer_SYS3(support_t *support){
    size_t len = (size_t)support->sup_exceptState[GENERALEXCEPT].reg_a1;
    char *c = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a2;      // stringa da scrivere
    int retval;
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