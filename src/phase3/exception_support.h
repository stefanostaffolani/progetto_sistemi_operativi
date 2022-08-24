#include "pager.h"

#define PRINTCHR 2    // come definito in pops a pagina 39
#define SENDCHAR 2    // pops capitolo 5.7
#define RECVCHAR 2    // pops capitolo 5.7


void exception_handler();

void syscall_exception_handler(support_t*, state_t*);

void program_trap_exception_handler(support_t*);

void Get_TOD_SYS1(state_t*);

void Terminate_SYS2(support_t*);

int Write_to_Printer_SYS3(support_t*);

int Write_to_Terminal_SYS4(support_t*);

int Read_from_Terminal_SYS5(support_t*);



