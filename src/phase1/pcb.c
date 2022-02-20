#include "pcb.h"

static struct list_head pcbFree_h;

static pcb_t pcbFree_table[MAXPROC];

/*
Inizializza la lista pcbFree in modo da
contenere tutti gli elementi della
pcbFree_table. Questo metodo deve
essere chiamato una volta sola in fase di
inizializzazione della struttura dati.
*/

void initPcbs(){ //TODO
    int i = 0;
    INIT_LIST_HEAD(&pcbFree_h);
    while(i < MAXPROC){
        list_add_tail(&(pcbFree_table[i].p_list), &pcbFree_h);   //aggiunge in coda
        INIT_LIST_HEAD(&(pcbFree_table[i].p_child));
        i++;
    }
}

/*
nserisce il PCB puntato da p nella lista
dei PCB liberi (pcbFree_h)
*/

void freePcb(pcb_PTR p){
    list_add_tail(&(p->p_list), &pcbFree_h);
}

/*
Restituisce NULL se la pcbFree_h è vuota.
Altrimenti rimuove un elemento dalla
pcbFree, inizializza tutti i campi (NULL/0)
e restituisce l’elemento rimosso.
*/

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
    INIT_LIST_HEAD(&tmp->p_sib);
    return tmp;
}

/*
Crea una lista di PCB, inizializzandola
come lista vuota.
*/

void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

/*
Restituisce TRUE se la lista puntata da
head è vuota, FALSE altrimenti.
*/

bool emptyProcQ(struct list_head *head){
    return list_empty(head);
}

/*
Restituisce l’elemento di testa della coda
dei processi da head, SENZA
RIMUOVERLO. Ritorna NULL se la coda
non ha elementi.
*/

pcb_PTR headProcQ(struct list_head *head){
    if(list_empty(head)){
        return NULL;
    }
    pcb_PTR ret_val = container_of(head->prev, pcb_t, p_list);
    return ret_val;
}

/*
Inserisce l’elemento puntato da p nella
coda dei processi puntata da head.
*/

void insertProcQ(struct list_head* head, pcb_PTR p){
    list_add(&(p->p_list),head);
}

/* 
Rimuove il primo elemento dalla coda dei
processi puntata da head. Ritorna NULL se la
coda è vuota. Altrimenti ritorna il puntatore
all’elemento rimosso dalla lista.
*/

pcb_PTR removeProcQ(struct list_head* head){
    if(list_empty(head)){
        return NULL;
    }
    pcb_PTR tmp = container_of(head->prev, pcb_t, p_list);
    list_del(head->prev);
    return tmp;
}

/*
Rimuove il PCB puntato da p dalla coda dei
processi puntata da head. Se p non è presente
nella coda, restituisce NULL. (NOTA: p può
trovarsi in una posizione arbitraria della coda).
*/

pcb_PTR outProcQ(struct list_head* head, pcb_t *p){
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

/*
Restituisce TRUE se il PCB puntato da p
non ha figli, FALSE altrimenti.
*/

bool emptyChild(pcb_PTR p){
    if(list_empty(&p->p_child))
        return TRUE;
    else 
        return FALSE;
}

/*
Inserisce il PCB puntato da p come figlio
del PCB puntato da prnt.
*/

void insertChild(pcb_PTR pnrt, pcb_PTR p){
    if(list_empty(&pnrt->p_child)){
        pnrt->p_child.next = &p->p_sib;
        p->p_parent = pnrt;
    }else{
        list_add_tail(&p->p_sib, &pnrt->p_child);
        p->p_parent = pnrt;
    }
}

/*
Rimuove il primo figlio del PCB puntato
da p. Se p non ha figli, restituisce NULL.
*/

pcb_PTR removeChild(pcb_PTR p){
    if(list_empty(&p->p_child)){
        return NULL;
    }
    pcb_PTR son = container_of(p->p_child.next, pcb_t, p_sib);
    list_del(p->p_child.next);
    p->p_parent = NULL;
    return son;
}

/*
Rimuove il PCB puntato da p dalla lista
dei figli del padre. Se il PCB puntato da
p non ha un padre, restituisce NULL,
altrimenti restituisce l’elemento
rimosso (cioè p). A differenza della
removeChild, p può trovarsi in una
posizione arbitraria (ossia non è
necessariamente il primo figlio del
padre).
*/

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