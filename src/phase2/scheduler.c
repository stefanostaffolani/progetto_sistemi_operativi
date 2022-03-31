#include "scheduler.h"

// just a skeleton

void scheduler(){
    if(!emptyProcQ(&high_priority_queue)){  // la coda ad alta priorita' non e' vuota
        removeProcQ(&high_priority_queue);
        //TODO:store the pointer of the pcb to currentprocess field
        //TODO:LDST(currentprocess->p_s)
    }else if (!emptyProcQ(&low_priority_queue)){   // la coda ad alta priorita' e' vuota, quella a bassa priorita' no
        removeProcQ(&low_priority_queue);
        //TODO:store the pointer of the pcb to currentprocess field
        //TODO:Load 5 milliseconds on the PLT. [Section 4.1.4-pops]
        //TODO:LDST(currentprocess->p_s)
    } else{   // sono entrambe vuote
        if (ProcessCount == 0) //spegni la macchina
            HALT();     
        else if (ProcessCount > 0 && SoftBlockedCount > 0){   // aspetta...
            //TODO: set STATUS reg for enabling interrupts and disable PLT
            WAIT();
        }else if (ProcessCount > 0 && SoftBlockedCount == 0){   // Deadlock
            PANIC();
        } 
    }

}