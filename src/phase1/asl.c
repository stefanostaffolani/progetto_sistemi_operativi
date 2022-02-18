#include "asl.h"

int insertBlocked(int *semAdd, pcb_PTR p){
    struct list_head *iter;
    list_for_each(iter, semd_h){
        if(*container_of(iter, semd_t, s_link)->s_key == *semAdd){
            list_add(p->p_semAdd, &(container_of(iter, semd_t, s_link)->s_procq));
            return FALSE;
        }
    }
    if(list_empty(semdFree_h)) return TRUE;
    iter = semdFree_h->next;
    list_del(iter);
    semd_PTR sm = container_of(iter, semd_t, s_link);
    sm->s_key = *semAdd;
    sm->s_procq = p->p_list;
    return FALSE;
}

void initASL(){ 
    int i = 0;
    //struct list_head *iter = semdFree_h;
    INIT_LIST_HEAD(&semdFree_h);
    while(i < MAXPROC){
        list_add_tail(&(semd_table[i].s_link), &semdFree_h);   //aggiunge in coda
        //tmp1->p_list.prev = pcbFree_h;
        i++;
    }
}

pcb_PTR headBlocked(int *semAdd){
    struct list_head *iter;
    list_for_each(iter, semd_h){
        if(*container_of(iter, semd_t, s_link)->s_key == *semAdd){
            if(list_empty(&(container_of(iter, semd_t, s_link)->s_procq))) return NULL;
            return container_of(iter, semd_t, s_link)->s_procq.next;
        }
    }
    return NULL;
}

pcb_PTR outBlocked(pcb_PTR p){
    int sem = p->p_semAdd;
    struct  list_head *iter;
    list_for_each(iter, semd_h){
        if(container_of(iter, semd_t, s_link)->s_key == sem){
            list_del(&p->p_list);
            if(list_empty(semd_h)){
                list_del(&(container_of(iter, semd_t, s_link)->s_link));
                list_add(&(container_of(iter, semd_t, s_link)->s_link), semdFree_h);
            }
            return p;
        }
    }
    return NULL;   // condizione di errore
}

pcb_PTR removeBlocked(int *semAdd){
    struct list_head *iter;
    list_for_each(iter, semd_h){
        if(container_of(iter, semd_t, s_link)->s_key == *semAdd){
            pcb_PTR retp = container_of(iter, semd_t, s_link)->s_procq.next;
            list_del(container_of(iter, semd_t, s_link)->s_procq.next);
            if(list_empty(&(container_of(iter, semd_t, s_link)->s_procq))){
                list_del(&(container_of(iter, semd_t, s_link)->s_link));
                list_add(&(container_of(iter, semd_t, s_link)->s_link), semdFree_h);
            }
            return retp;
        }
    }
    return NULL;  // condizione di errore
}