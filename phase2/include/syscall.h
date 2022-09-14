#include "interrupt.h"
#include "umps3/umps/cp0.h"
#include "umps3/umps/arch.h"
#include "umps3/umps/types.h"

void Create_Process_NSYS1(state_t *);

void Terminate_Process_NSYS2(int, state_t *);

void Passeren_NSYS3(int *, state_t *);

void Verhogen_NSYS4(int *,state_t *);

void terminateProgeny(pcb_t*);

void terminateSingleProcess(pcb_t *);

void DO_IO_Device_NSYS5(state_t *);

void Get_CPU_Time_NSYS6(state_t *);

void Wait_For_Clock_NSYS7(state_t *);

void Get_SUPPORT_Data_NSYS8(state_t *);

void Get_Process_ID_NSYS9(state_t *, int);

void Yield_NSYS10(state_t *);
