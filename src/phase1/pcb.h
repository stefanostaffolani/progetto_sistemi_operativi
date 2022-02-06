#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "pandos_types.h"

// implementazione veloce, da controllare e commentare!!!

//pcb_t pcbFree_table[MAXPROC];
static pcb_PTR pcbFree_h;

static pcb_PTR pcbFree_table[MAXPROC];

void initPcbs(){ //TODO
    int i = 0;
    pcbFree_h->p_list;
    struct list_head iter = pcbFree_h->p_list;
    pcb_PTR tmp = pcbFree_table[0];
    while(i < MAXPROC){
        pcb_PTR tmp1 = malloc(sizeof(pcb_t));
        tmp1 = pcbFree_table[i];
        list_add_tail(&tmp1->p_list, &pcbFree_h->p_list);   //aggiunge in coda
        //tmp1->p_list.prev = pcbFree_h;
        i++;
    }
}

void freePcb(pcb_PTR p){
    list_add_tail(&p->p_list, &pcbFree_h->p_list);
}

pcb_PTR allocPcb(){
    if(list_empty(&pcbFree_h->p_list)){
        return NULL;
    }
    //pcb_PTR tmp = malloc(sizeof(pcb_t));
    pcb_PTR tmp =  pcbFree_h->p_list.next;
    tmp->p_child.next = NULL;
    tmp->p_child.prev = NULL;
    tmp->p_list.next = NULL;
    tmp->p_list.prev = NULL;
    tmp->p_parent = NULL;
    tmp->p_s.cause = NULL;
    tmp->p_s.entry_hi = NULL;
    tmp->p_s.hi = NULL;
    tmp->p_s.lo = NULL;
    tmp->p_s.pc_epc = NULL;
    tmp->p_s.status = NULL;
    tmp->p_semAdd = NULL;
    tmp->p_sib.next = NULL;
    tmp->p_sib.prev = NULL;
    tmp->p_time = NULL;
    list_del(&tmp->p_list);
    return tmp;
}

int emptyProcQ(struct list_head *head){
    return head == NULL;
}

void mkEmptyProcQ(struct list_head *head){
    LIST_HEAD(head);
}

pcb_t* headProcQ(struct list_head* head){
    if(list_empty(head)){
        //pcb_PTR tmp = malloc(sizeof(pcb_t));
        //tmp =  pcbFree_h->p_list.next;
        //tmp->p_child.next = NULL;
        //tmp->p_child.prev = NULL;
        //tmp->p_list.next = NULL;
        //tmp->p_list.prev = NULL;
        //tmp->p_parent = NULL;
        //tmp->p_s.cause = NULL;
        //tmp->p_s.entry_hi = NULL;
        //tmp->p_s.hi = NULL;
        //tmp->p_s.lo = NULL;
        //tmp->p_s.pc_epc = NULL;
        //tmp->p_s.status = NULL;
        //tmp->p_semAdd = NULL;
        //tmp->p_sib.next = NULL;
        //tmp->p_sib.prev = NULL;
        //tmp->p_time = NULL;
        //return *tmp;  //come si fa a ritornare NULL????
        return NULL;
    }
    pcb_PTR ret_val = container_of(head->next, pcb_t, p_list);
    return ret_val;
}

pcb_t* outProcQ(struct list_head* head, pcb_t *p){
    struct list_head *iter = head;
    int found = FALSE;
    while(iter != NULL && (found == FALSE)){
        if(iter == p){
            list_del(iter);
            found = TRUE;
        }else{
            iter = iter->next;
        }
        if(iter == head){
            found = FALSE;
        }
    }
    return NULL;
}

pcb_PTR removeProcQ(struct list_head* head){
    if(list_empty(head)){
        return NULL;
    }
    struct list_head *tmp = head->next;
    list_del(tmp);
    return tmp;
}

int emptyChild(pcb_PTR p){
    if(list_empty(&p->p_child))
        return FALSE;
    else 
        return TRUE;
}

void insertChild(pcb_PTR pnrt, pcb_PTR p){
    pcb_PTR sib = container_of(pnrt->p_child.next, pcb_t, p_sib);
    list_add(&p->p_sib, &sib->p_sib);
    pnrt->p_child.next = &p->p_child;
    p->p_parent = pnrt;
}

pcb_PTR removeChild(pcb_PTR p){
    if(list_empty(p)){
        return NULL;
    }
    pcb_PTR sib = container_of(p->p_child.next, pcb_t, p_sib);
    list_del(&sib->p_sib);
    list_del(p->p_child.next);
    list_add(&p->p_child, &sib->p_child);
    p->p_parent = NULL;
}

pcb_PTR outChild(pcb_PTR p){
    pcb_PTR daddy = p->p_parent;
    if(daddy = NULL) return NULL;
    pcb_PTR sib = container_of(p->p_sib.next, pcb_t, p_sib);
    list_del(&p->p_sib);
    if(daddy->p_child.next == &p->p_child){    //se e' il primo figlio
        list_del(&p->p_child);
        daddy->p_child.next = &sib->p_child;
    }
}