#include "interrupt.h"
#include "umps3/umps/cp0.h"
#include "umps3/umps/arch.h"
#include "umps3/umps/types.h"

void Create_Process_NSYS1(state_t *);

void Terminate_Process_NSYS2(int, state_t *);

void Passeren_NSYS3(int *, state_t *);

void Verhogen_NSYS4(int *,state_t *);

void terminateprogeny(pcb_t*);
void terminateProgeny(pcb_t*);

void terminateSingleProcess(pcb_t *);

void DO_IO_Device_NSYS5(state_t *);

void NSYS6_Get_CPU_Time(state_t *);

void NSYS7_Wait_For_Clock(state_t *);

void NSYS8_Get_SUPPORT_Data(state_t *);

void NSYS9_Get_Process_ID(state_t *, int);

void NSYS10_Yield(state_t *);