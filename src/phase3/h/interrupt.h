#include "scheduler.h"


void interrupt_exception(unsigned int, state_t *);
void manageInterr(int, state_t *);
void manageNTInt(int, int, state_t *);
