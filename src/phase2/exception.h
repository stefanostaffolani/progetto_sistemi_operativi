#include "syscall.h"

void exceptionHandler();

void syscall_exception();

void interrupt_exception();

void pass_up_or_die(int);   // da fare in fase 3 prendere gia' pronte da qualche parte in p2test..