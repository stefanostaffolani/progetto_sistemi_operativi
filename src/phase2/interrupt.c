#include "interrupt.h"

extern pcb_PTR readyQueue;
extern pcb_PTR currentProcess;
extern int sbCount;

cpu_t interrTime;

void interrupt_exception(unsigned int cause){

    // TODO: pulire questi commenti 
    /*takes
    an unsigned integer as its input parameter and populates it with the value of
    the low-order word of the TOD clock divided by the Time Scale*/
    STCK(interrTime);   //save the interrupt starting time

    /*Which interrupt lines have pending interrupts is set in Cause.IP*/
    unsigned int causeIp = getBits(cause, 0xFF00, 8); //TODO: non sono sicuro funzioni
    int line = 2;

    // for each line manage the interrupt
    for (int i = 1; i < 8; i++) {
        if (causeIp & line)
            manageInterr(i);
        line *= 2;
    }
}

void manageInterr(int line){
    
}

