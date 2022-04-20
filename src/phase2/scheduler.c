#include "scheduler.h"

void scheduler(){
    klog_print("entro nello scheduler...\n");
    if(!emptyProcQ(&high_priority_queue)){  // la coda ad alta priorita' non e' vuota
        currentProcess = removeProcQ(&high_priority_queue);
        STCK(startTime);
        klog_print("primo caso scheduler\n");
        //processor_state->pc_epc += WORDLEN;
        LDST(&(currentProcess->p_s));
    }else if (!emptyProcQ(&low_priority_queue)){   // la coda ad alta priorita' e' vuota, quella a bassa priorita' no
        currentProcess =  removeProcQ(&low_priority_queue);
        //TODO:Load 5 milliseconds on the PLT. [Section 4.1.4-pops], maybe done
        setTIMER(TIMESLICE);
        STCK(startTime);
        klog_print("secondo caso scheduler\n");
        //processor_state->pc_epc += WORDLEN;
        LDST(&(currentProcess->p_s));
    } else{   // sono entrambe vuote

        if (prCount == 0) //spegni la macchina
            HALT();     
        else if (prCount > 0 && sbCount > 0){   // aspetta...
            //TODO: set STATUS reg for enabling interrupts and disable PLT, maybe done
            setTIMER(-1);  
            setSTATUS(IECON | IMON);   //interrupts on and PLT off
            WAIT();

        }else if (prCount > 0 && sbCount == 0){   // Deadlock
            PANIC();
        }
    }
    klog_print("esco dallo scheduler...\n");
}