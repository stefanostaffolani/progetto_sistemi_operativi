#include "asl.h"

static semd_t semd_table[MAXPROC];

static struct list_head semdFree_h;

static struct list_head semd_h;

/*
Viene inserito il PCB puntato da p nella coda dei
processi bloccati associata al SEMD con chiave
semAdd. Se il semaforo corrispondente non è
presente nella ASL, alloca un nuovo SEMD dalla
lista di quelli liberi (semdFree) e lo inserisce nella
ASL, settando I campi in maniera opportuna (i.e.
key e s_procQ). Se non è possibile allocare un
nuovo SEMD perché la lista di quelli liberi è vuota,
restituisce TRUE. In tutti gli altri casi, restituisce
FALSE.
*/

bool insertBlocked(int *semAdd, pcb_PTR p){
    semd_PTR iter;
    list_for_each_entry(iter, &semd_h, s_link){
        if(iter->s_key == semAdd){
            p->p_semAdd = semAdd;
            insertProcQ(&iter->s_procq, p);
            return FALSE;
        }
    }
    if(list_empty(&semdFree_h)) return TRUE;
    struct list_head *tmp = semdFree_h.next;
    list_del(tmp);
    list_add_tail(tmp, &semd_h);
    semd_PTR sm = container_of(tmp, semd_t, s_link);
    INIT_LIST_HEAD(&sm->s_procq);
    sm->s_key = semAdd;
    p->p_semAdd = semAdd;
    insertProcQ(&sm->s_procq, p);
    return FALSE;
}

/*
Inizializza la lista dei semdFree in
modo da contenere tutti gli elementi
della semdTable. Questo metodo
viene invocato una volta sola durante
l’inizializzazione della struttura dati.
*/

void initASL(){ 
    int i = 0;
    INIT_LIST_HEAD(&semdFree_h);
    INIT_LIST_HEAD(&semd_h);
    while(i < MAXPROC){
        list_add_tail(&(semd_table[i].s_link), &semdFree_h);   //aggiunge in coda
        i++;
    }
}

/*
Restituisce (senza rimuovere) il puntatore al PCB che
si trova in testa alla coda dei processi associata al
SEMD con chiave semAdd. Ritorna NULL se il SEMD
non compare nella ASL oppure se compare ma la sua
coda dei processi è vuota.
*/

pcb_PTR headBlocked(int *semAdd){
    semd_PTR iter;
    list_for_each_entry(iter, &semd_h, s_link){
        if(iter->s_key == semAdd){
            if(list_empty(&iter->s_procq))
                return NULL;
            else
                return headProcQ(&iter->s_procq);
        }
    }
    return NULL;
}

/*
Rimuove il PCB puntato da p dalla coda del semaforo
su cui è bloccato (indicato da p->p_semAdd). Se il PCB
non compare in tale coda, allora restituisce NULL
(condizione di errore). Altrimenti, restituisce p. Se la
coda dei processi bloccati per il semaforo diventa
vuota, rimuove il descrittore corrispondente dalla ASL
e lo inserisce nella coda dei descrittori liberi.
*/

pcb_PTR outBlocked(pcb_PTR p){
    semd_PTR iter;
    pcb_PTR iter2;
    list_for_each_entry(iter, &semd_h, s_link){
        if(iter->s_key == p->p_semAdd){
            list_for_each_entry(iter2, &iter->s_procq, p_list){
                if(iter2 == p){
                    list_del(&iter2->p_list);
                    if(list_empty(&iter->s_procq)){
                        list_del(&iter->s_link);
                        list_add_tail(&iter->s_link, &semdFree_h);
                    }
                    return iter2;
                }
            }
        }
    }
    return NULL;
}

/*
Ritorna il primo PCB dalla coda dei processi
bloccati (s_procq) associata al SEMD della
ASL con chiave semAdd. Se tale descrittore
non esiste nella ASL, restituisce NULL.
Altrimenti, restituisce l’elemento rimosso. Se
la coda dei processi bloccati per il semaforo
diventa vuota, rimuove il descrittore
corrispondente dalla ASL e lo inserisce nella
coda dei descrittori liberi (semdFree_h).
*/

pcb_PTR removeBlocked(int *semAdd){
    semd_PTR iter;
    list_for_each_entry(iter, &semd_h, s_link){
        if(iter->s_key == semAdd){
            pcb_PTR ret =  removeProcQ(&iter->s_procq);
            if(list_empty(&iter->s_procq)){
                list_del(&iter->s_link);
                list_add_tail(&iter->s_link, &semdFree_h);
            }
        return ret;
        }
    }
    return NULL;
}