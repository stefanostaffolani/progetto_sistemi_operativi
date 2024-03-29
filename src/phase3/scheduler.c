#include "h/scheduler.h"
void scheduler(){
    if(!emptyProcQ(&high_priority_queue)){  // la coda ad alta priorita' non e' vuota
        currentProcess = removeProcQ(&high_priority_queue);
        STCK(startTime);
        LDST(&(currentProcess->p_s));
    }else if (!emptyProcQ(&low_priority_queue)){   // la coda ad alta priorita' e' vuota, quella a bassa priorita' no
        currentProcess =  removeProcQ(&low_priority_queue);
        //klog_print("scheduler lowq\n");
        // Load 5 milliseconds on the PLT. [Section 4.1.4-pops]
        setTIMER(TIMESLICE * (*((cpu_t*) TIMESCALEADDR)));
        STCK(startTime);
        LDST(&(currentProcess->p_s));
    } else{   // sono entrambe vuote
        if (prCount == 0) //spegni la macchina
            HALT();     
        else if (prCount > 0 && sbCount > 0){   // aspetta...
            //klog_print("wait\n");
            //breakpoint();
            setSTATUS(ALLOFF | IECON | IMON);   //interrupts on and PLT off
            WAIT();
        }else if (prCount > 0 && sbCount == 0){   // purtroppo Deadlock...
            klog_print("sto per andare in panic\n");
            PANIC();
        }
    }
}