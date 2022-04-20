#include "interrupt.h"
#include "umps3/umps/cp0.h"
#include "umps3/umps/arch.h"
#include "umps3/umps/types.h"

void Create_Process_NSYS1();

void Terminate_Process_NSYS2();

void Passeren_NSYS3(int *);

void Verhogen_NSYS4(int *);

void terminateProgeny(pcb_t*);

void terminateSingleProcess(pcb_t*);

void DO_IO_Device_NSYS5();

void NSYS6_Get_CPU_Time();

void NSYS7_Wait_For_Clock();

void NSYS8_Get_SUPPORT_Data();

void NSYS9_Get_Process_ID();

void NSYS10_Yield();

