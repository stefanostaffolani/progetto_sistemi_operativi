#include "h/exception_support.h"

/**
 * @brief support level general exception handler
 * 
 */
void exception_handler_support(){
    support_t *support = (support_t *) SYSCALL(GETSUPPORTPTR,0,0,0);
    const int cause = CAUSE_GET_EXCCODE(support->sup_exceptState[GENERALEXCEPT].cause);
    switch (cause){
    case SYSEXCEPTION:
        syscall_exception_handler_support(support);
        break;
    default:
        program_trap_exception_handler(support);
        break;
    }
    LDST(&support->sup_exceptState[GENERALEXCEPT]);
}

/**
 * @brief Support level syscall exception handler,
 *  takes syscode from reg_a0 and increase program counter
 * 
 * @param support 
 */
void syscall_exception_handler_support(support_t *support){
    state_t *except_state = &support->sup_exceptState[GENERALEXCEPT];
    except_state->pc_epc += WORDLEN;
    except_state->reg_t9 += WORDLEN;
    const int a0 = except_state->reg_a0;
    int retval;
    switch (a0){
    case GETTOD:
        Get_TOD_SYS1(except_state);
        break;
    case TERMINATE:
        Terminate_SYS2(support);
        break;
    case WRITETOPRINTER: 
        retval = Write_to_Printer_SYS3(support);
        except_state->reg_v0 = retval;
        break;
    case WRITETOTERMINAL:
        retval = Write_to_Terminal_SYS4(support);
        except_state->reg_v0 = retval;    
        break;
    case READFROMTERMINAL:                            
        retval = Read_from_Terminal_SYS5(support);
        except_state->reg_v0 = retval;
        break;
    default:                            // kill process ==> this is an error
        program_trap_exception_handler(support);
        break;
    }
    
}
/**
 * @brief Support level program trap exception handler,
 *  Terminate process with SYS2 (do V op in sem_swap)
 * 
 * @param support 
 */
void program_trap_exception_handler(support_t *support){   
    Terminate_SYS2(support);
}

/**
 * @brief Syscall number 1, save the current TOD in reg_v0
 * 
 * @param except_state 
 */
void Get_TOD_SYS1(state_t *except_state){
    cpu_t TOD;
    STCK(TOD);
    except_state->reg_v0 = TOD;
}

/**
 * @brief Wrappper for NSY2, it update the swap_asid[] and swap_pool
 *  before terminate the process. It performs a V operation on master semaphore
 *  signaling test process about termination.   
 * 
 * @param support 
 */
void Terminate_SYS2(support_t *support){
    if(get_swap_asid(support->sup_asid)){
        update_swap_asid(0,support->sup_asid);
        SYSCALL(VERHOGEN, (int)&sem_swap, 0, 0);    
        for(int i = 0; i < POOLSIZE; i++){
            if(swap_pool[i].sw_asid == support->sup_asid)   
                swap_pool[i].sw_asid = NOPROC;
        }
    }
    SYSCALL(VERHOGEN, (int)&master_semaphore, 0, 0);
    SYSCALL(TERMPROCESS,0,0,0);
}

/**
 * @brief Takes length of the string from reg_a2 and the first char from reg_a1,
 * perform a P operation on sem_write_printer for mutex, save the char in device field data0
 * and perform a NSYS5 DOIO on device field command with command PRINTCHR(2), if the status of 
 * the DOIO is not READY release mutex and return the negative of status, else continue and return
 * the length of the string. The device number is (asid-1).
 * 
 * @param support 
 * @return int 
 */
int Write_to_Printer_SYS3(support_t *support){
    size_t len = (size_t)support->sup_exceptState[GENERALEXCEPT].reg_a2;
    char *c = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a1;      // stringa da scrivere
    dtpreg_t *device = (dtpreg_t *)DEV_REG_ADDR(IL_PRINTER, support->sup_asid-1);     // il device e' asid-1 perche' i device sono 8 (da 0 a 7) e i processi sono 8 (da 1 a 8)
    SYSCALL(PASSEREN,(int)&sem_write_printer,0,0);                         // mutua esclusione per chiamare la DOIO
    for(size_t i = 0; i < len; i++){
        device->data0 = c[i];
        int status = SYSCALL(DOIO, (int)&(device->command), PRINTCHR, 0);
        if (status != READY){
            SYSCALL(VERHOGEN, (int)&sem_write_printer, 0, 0);
            return -status;
        }
    }
    SYSCALL(VERHOGEN,(int)&sem_write_printer,0,0);
    return len;
}

/**
 * @brief Takes the length of the string from reg_a2 and the first char from reg_a1,
 * perform a P operation on sem_write_terminal for mutex, the value is a memaddr with
 * first byte is PRINTCHR and second byte the char to be trasmitted. Perform a NSYS5 DOIO
 * on device transm_command with value, if status is not RECVCHAR(5) release mutex and return 
 * the neagtive of status, else continue and at the end of the string release mutex and return 
 * the length of the string. The device is (asid-1)
 *  
 * @param support 
 * @return int 
 */
int Write_to_Terminal_SYS4(support_t *support){  
    size_t len = (size_t)support->sup_exceptState[GENERALEXCEPT].reg_a2;
    char *c = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a1;
    dtpreg_t *device = (dtpreg_t *)DEV_REG_ADDR(IL_TERMINAL, support->sup_asid-1);
    SYSCALL(PASSEREN, (int)&sem_write_terminal, 0, 0);
    for(size_t i = 0; i < len; i++){
        memaddr value = PRINTCHR | (memaddr)(c[i] << 8);       // pops 5.7 value = |xxxxxxxx|xxxxxxxx|char|PRINTCHR|
        int status = SYSCALL(DOIO, (int)&((termreg_t *)device)->transm_command, (int)value, 0);
        if((status & 0x0000000F) != 5){         // 0x0000000F mi servono solo da 0 a 5
            SYSCALL(VERHOGEN,(int)&sem_write_terminal,0,0);
            return -status;
        }
    }
    SYSCALL(VERHOGEN,(int)&sem_write_terminal,0,0);
    return len;
}

/**
 * @brief When requested, this service causes the requesting U-proc to be suspended 
 * until a line of input has been transmitted from the terminal device associated with the U-proc (asid-1).
 * Perform a P operation on sem_read_terminal for mutex and start reading while char newline is received, if
 * received status is different from RECVCHAR(5) release mutex and return negative status else save the char
 * in buffer, when newline is received release mutex and return the size of the string.
 * 
 * 
 * @param support 
 * @return size_t 
 */
size_t Read_from_Terminal_SYS5(support_t *support){
    char *buffer = (char *)support->sup_exceptState[GENERALEXCEPT].reg_a1;
    termreg_t *terminal = (termreg_t *) DEV_REG_ADDR(IL_TERMINAL, support->sup_asid-1);
    SYSCALL(PASSEREN, (int)&sem_read_terminal, 0, 0);
    size_t i = 0;
    char c = '\0';
    while(c != '\n'){
        int status = SYSCALL(DOIO, (int)&(terminal->recv_command), RECVCHAR, 0);   // cfr 5.7 pops
        if((status & 0x0000000F) != 5){     
            SYSCALL(VERHOGEN, (int)&sem_read_terminal, 0, 0);
            return -(status & 0x0000000F);
        }
        c = (char) (0x000000FF & (status >> 8));   // status = |xxxxxxxx|xxxxxxxx|recvchar|xxxxxxxx|
        *(buffer + i) = c;
        i++;
    }
    SYSCALL(VERHOGEN, (int)&sem_read_terminal, 0, 0);
    *(buffer + i) = '\0';                               // ending string
    return i;
}