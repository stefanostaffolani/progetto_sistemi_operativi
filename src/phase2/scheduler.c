#include "scheduler.h"

void memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}


void memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}



void scheduler(){
    if(!emptyProcQ(&high_priority_queue)){  // la coda ad alta priorita' non e' vuota
        currentProcess = removeProcQ(&high_priority_queue);
        LDST(&(currentProcess->p_s));
    }else if (!emptyProcQ(&low_priority_queue)){   // la coda ad alta priorita' e' vuota, quella a bassa priorita' no
        currentProcess =  removeProcQ(&low_priority_queue);
        //TODO:Load 5 milliseconds on the PLT. [Section 4.1.4-pops], maybe done
        setTIMER(TIMESLICE);
        LDST(&(currentProcess->p_s));
    } else{   // sono entrambe vuote
        if (prCount == 0) //spegni la macchina
            HALT();     
        else if (prCount > 0 && sbCount > 0){   // aspetta...
            //TODO: set STATUS reg for enabling interrupts and disable PLT, maybe done
            setTIMER(-1);    // il prof ha detto di metterlo!
            setSTATUS(IECON | IMON);   //interrupts on and PLT off
            WAIT();
        }else if (prCount > 0 && sbCount == 0){   // Deadlock
            PANIC();
        }
    }

}