#include "interrupt.h"
#include "umps3/umps/cp0.h"
#include "umps3/umps/arch.h"
#include "umps3/umps/types.h"

void Create_Process_SYS1();

void Terminate_Process_SYS2();

void Passeren_SYS3();

void Verhogen_SYS4();

void terminateProgeny(pcb_t*);

void terminateSingleProcess(pcb_t*);

void DO_IO_Device_NSYS5();

void NSYS6_Get_CPU_Time();

void NSYS7_Wait_For_Clock();

void NSYS8_Get_SUPPORT_Data();

void NSYS9_Get_Process_ID();

void NSYS10_Yield();

