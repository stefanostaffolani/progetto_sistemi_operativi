#include "pandos_const.h"
#include "pandos_types.h"
#include "/usr/include/umps3/umps/types.h"
#include "/usr/include/umps3/umps/const.h"
#include "globals.h"

swap_t swap_pool[POOLSIZE];

int sem_swap;   // per mutua esclusione sulla swap pool

int swap_asid[8];

int sem_write_printer;   // semafori per le syscall write (SYS3 e SYS4)
int sem_write_terminal;
int sem_read_terminal;   // semaforo per la syscall read (SYS5)

void init_swap_asid();
void update_swap_asid(int,int);
int get_swap_asid(int);
void init_swap_pool();
unsigned int get_vpn_index(unsigned int);