#ifndef ASL_H_INCLUDED
#define ASL_H_INCLUDED
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "pcb.h"

static semd_t semd_table[MAXPROC];

static struct list_head *semdFree_h;

static struct list_head *semd_h;

int insertBlocked(int *semAdd, pcb_PTR p){
    struct list_head *iter;
    list_for_each(iter, semd_h){
        if(container_of(iter, semd_t, s_link)->s_key == *semAdd){
            list_add(p->p_semAdd, &(container_of(iter, semd_t, s_link)->s_procq));
            return FALSE;
        }
    }
    if(list_empty(semdFree_h)) return TRUE;
    struct list_head *iter = semdFree_h->next;
    list_del(iter);
    semd_PTR sm = container_of(iter, semd_t, s_link);
    sm->s_key = *semAdd;
    sm->s_procq = p->p_list;
    return FALSE;
}

void initASL(){ 
    int i = 0;
    //struct list_head *iter = semdFree_h;
    while(i < MAXPROC){
        semd_PTR tmp1 = malloc(sizeof(pcb_t));
        tmp1 = &semd_table[i];
        list_add_tail(&tmp1->s_link, &semdFree_h);   //aggiunge in coda
        //tmp1->p_list.prev = pcbFree_h;
        i++;
    }
}

pcb_PTR headBlocked(int *semAdd){
    struct list_head *iter;
    list_for_each(iter, semd_h){
        if(container_of(iter, semd_t, s_link)->s_key == *semAdd){
            if(list_empty(&(container_of(iter, semd_t, s_link)->s_procq))) return NULL;
            return container_of(iter, semd_t, s_link)->s_procq.next;
        }
    }
    return NULL;
}

