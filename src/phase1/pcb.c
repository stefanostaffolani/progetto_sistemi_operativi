#include "pcb.h"

// implementazione veloce, da controllare e commentare!!!

void initPcbs(){ //TODO
    int i = 0;
    INIT_LIST_HEAD(&pcbFree_h);
    while(i < MAXPROC){
        list_add_tail(&(pcbFree_table[i].p_list), &pcbFree_h);   //aggiunge in coda
        INIT_LIST_HEAD(&(pcbFree_table[i].p_child));
        i++;
    }
}

void freePcb(pcb_PTR p){
    list_add_tail(&(p->p_list), &pcbFree_h);
}

pcb_PTR allocPcb(){
    if(list_empty(&pcbFree_h)){
        return NULL;
    }
    pcb_PTR tmp =  container_of(pcbFree_h.next, pcb_t, p_list);
    //tmp->p_child.next = NULL;
    list_del(list_next(&pcbFree_h));
    INIT_LIST_HEAD(&tmp->p_child);
    INIT_LIST_HEAD(&tmp->p_list);
    tmp->p_parent = NULL;
    //tmp->p_s.cause = 0;
    //tmp->p_s.entry_hi = 0;
    //tmp->p_s.hi = 0;
    //tmp->p_s.lo = 0;
    //tmp->p_s.pc_epc = 0;
    //tmp->p_s.status = 0;
    //tmp->p_semAdd = NULL;
    INIT_LIST_HEAD(&tmp->p_sib);
    //tmp->p_time = 0;
    //pcbFree_h = *pcbFree_h.next;
    //list_del(list_next(&pcbFree_h));
    return tmp;
}

int emptyProcQ(struct list_head *head){
    return list_empty(head);
}

void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

pcb_PTR headProcQ(struct list_head *head){
    if(list_empty(head)){
        return NULL;
    }
    pcb_PTR ret_val = container_of(head->prev, pcb_t, p_list);
    return ret_val;
}

void insertProcQ(struct list_head* head, pcb_PTR p){
    list_add(&(p->p_list),head);
}

pcb_t* outProcQ(struct list_head* head, pcb_t *p){
    struct list_head *iter;
    int found = FALSE;
    list_for_each(iter, head){
        if(iter == &(p->p_list)){
            list_del(iter);
            found = TRUE;
            break;
        }
    }
    if(!found) return NULL;
    else return container_of(iter,pcb_t, p_list);
}

pcb_PTR removeProcQ(struct list_head* head){
    if(list_empty(head)){
        return NULL;
    }
    pcb_PTR tmp = container_of(head->prev, pcb_t, p_list);
    list_del(head->prev);
    return tmp;
}

int emptyChild(pcb_PTR p){
    if(list_empty(&p->p_child))
        return TRUE;
    else 
        return FALSE;
}

void insertChild(pcb_PTR pnrt, pcb_PTR p){
    pcb_PTR sib = container_of(pnrt->p_child.next, pcb_t, p_sib);
    list_add(&p->p_sib, &sib->p_sib);
    pnrt->p_child.next = &p->p_sib;
    p->p_parent = pnrt;
}

pcb_PTR removeChild(pcb_PTR p){
    if(list_empty(&p->p_child)){
        return NULL;
    }
    pcb_PTR sib = container_of(p->p_child.next, pcb_t, p_sib);
    pcb_PTR son = container_of(p->p_child.next, pcb_t, p_child);
    list_del(p->p_child.next);
    list_del(sib->p_sib.prev);
    p->p_child.next = &sib->p_sib;
    //list_add(&p->p_child, &sib->p_child);
    p->p_parent = NULL;
    return son;
}

pcb_PTR outChild(pcb_PTR p){
    pcb_PTR daddy = p->p_parent;
    if(daddy == NULL) return NULL;
    pcb_PTR sib = container_of(p->p_sib.next, pcb_t, p_sib);
    list_del(&p->p_sib);
    if(daddy->p_child.next == &p->p_child){    //se e' il primo figlio
        list_del(&p->p_child);
        daddy->p_child.next = &sib->p_child;
    }
    return p;
}
