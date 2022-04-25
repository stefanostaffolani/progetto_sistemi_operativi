#include "syscall.h"

void exceptionHandler();

void syscall_exception(state_t *);

void interrupt_exception();

void pass_up_or_die(int, state_t *);   