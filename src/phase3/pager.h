#include "globals.h"

void init_swap_asid();
void update_swap_asid(int,int);
int get_swap_asid(int);
void init_swap_pool();
unsigned int get_vpn_index(unsigned int);
int replace_algo();
void uTLB_RefillHandler();
void rw_flash(int, int, unsigned int);
void update_swap_pool(int, unsigned int, support_t *);
void pager();
