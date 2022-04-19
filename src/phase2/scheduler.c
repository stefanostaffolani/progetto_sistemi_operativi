#include "scheduler.h"

void memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}


void scheduler(){
    klog_print("ENTRO NELL SCHED...\n");
    if(!emptyProcQ(&high_priority_queue)){  // la coda ad alta priorita' non e' vuota
            klog_print("la coda ad alta priorita' non e' vuota\n");
        currentProcess = removeProcQ(&high_priority_queue);
        klog_print("si rompe mica qua?\n");
        LDST(&(currentProcess->p_s));
            klog_print("non è vuota e faccio cose\n");
    }else if (!emptyProcQ(&low_priority_queue)){   // la coda ad alta priorita' e' vuota, quella a bassa priorita' no
            klog_print("la coda ad alta priorita' e' vuota, quella a bassa priorita' no\n");
        currentProcess =  removeProcQ(&low_priority_queue);
          klog_print("si rompe mica qua?\n");
        //TODO:Load 5 milliseconds on the PLT. [Section 4.1.4-pops], maybe done
        setTIMER(TIMESLICE);
          klog_print("oppure si rompe mica qua?\n");
        LDST(&(currentProcess->p_s));
                 klog_print("la coda ad bassa priorità non è vuota e faccio cose\n");
    } else{   // sono entrambe vuote
        klog_print("sono entrambe stra che vuote\n");

        if (prCount == 0) //spegni la macchina
            HALT();     
        else if (prCount > 0 && sbCount > 0){   // aspetta...
            //TODO: set STATUS reg for enabling interrupts and disable PLT, maybe done
            setTIMER(-1);    // il prof ha detto di metterlo!
            setSTATUS(IECON | IMON);   //interrupts on and PLT off
            WAIT();
                    klog_print("oddio entro qua\n");

        }else if (prCount > 0 && sbCount == 0){   // Deadlock
            PANIC();
        }
    }

}