#include "asl.h"

static semd_t semd_table[MAXPROC];

static struct list_head semdFree_h;

struct list_head semd_h;

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
        if(iter->s_key == semAdd){              // se trovo il semaforo
            p->p_semAdd = semAdd;               // identificatore del semaforo del processo
            insertProcQ(&iter->s_procq, p);     // inserisco nella coda dei processi
            return FALSE;
        }
    }
    if(list_empty(&semdFree_h)) return TRUE;   
    struct list_head *tmp = semdFree_h.next;            // puntatore al primo elemento 
    list_del(tmp);                                      // rimuovo il primo elemento dalla coda di quelli liberi
    list_add_tail(tmp, &semd_h);                        // lo aggiungo in coda a ASL (semd_h)
    semd_PTR sm = container_of(tmp, semd_t, s_link);    // puntatore al semaforo     
    INIT_LIST_HEAD(&sm->s_procq);                       // inizializzo la coda dei processi
    sm->s_key = semAdd;                                 
    p->p_semAdd = semAdd;           
    insertProcQ(&sm->s_procq, p);                       // inserisco nella coda dei processi        
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
    INIT_LIST_HEAD(&semdFree_h);                               // inizializzo semdFree
    INIT_LIST_HEAD(&semd_h);                                   // inizializzo semd_h
    while(i < MAXPROC){             
        list_add_tail(&(semd_table[i].s_link), &semdFree_h);   //aggiunge in coda
        i++;
    }
}

/*
Restituisce (senza rimuovere) il puntatore al PCB che
si trova in testa alla coda dei processi associata al
SEMD con chiave semAdd. Ritorna NULL se il SEMD
non compare nella ASL oppure se compare ma la suas_lin
coda dei processi è vuota.
*/

pcb_PTR headBlocked(int *semAdd){
    semd_PTR iter;
    list_for_each_entry(iter, &semd_h, s_link){     // cicla semd_h
        if(iter->s_key == semAdd){                  // se trovo il semaforo
            if(list_empty(&iter->s_procq))          // se la coda dei processi è vuota (NULL)
                return NULL;
            else
                return headProcQ(&iter->s_procq);   // atrimenti restituisco il primo processo bloccato (l'ultimo della coda)
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
    list_for_each_entry(iter, &semd_h, s_link){                      // cicla semd_h
        if(iter->s_key == p->p_semAdd){                              // se trovo il semaforo
            list_for_each_entry(iter2, &iter->s_procq, p_list){      // cicla s_procq
                if(iter2 == p){                                      // se trovo il processo   
                    list_del(&iter2->p_list);                        // lo rimuovo   
                    if(list_empty(&iter->s_procq)){                  // se la coda dei processi blocatti diventa vuota   
                        list_del(&iter->s_link);                     // rimuovo il semaforo da semd_h
                        list_add_tail(&iter->s_link, &semdFree_h);   // lo aggiungo nella semdFree_h
                    }
                    return iter2;                                    // ritorno il puntatore al processo
                }
            }
        }
    }
    return NULL;                                                     // NULL se non trovo il processo o il semaforo
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
    list_for_each_entry(iter, &semd_h, s_link){                     // ciclo semd_h
        if(iter->s_key == semAdd){                                  // se trovo il semaforo
            pcb_PTR ret =  removeProcQ(&iter->s_procq);             // rimuovo il primo processo bloccato
            if(list_empty(&iter->s_procq)){                         // se la coda dei processi diventa vuota
                list_del(&iter->s_link);                            // rimuovo il semaforo da semd_h
                list_add_tail(&iter->s_link, &semdFree_h);          // lo aggiungo nella semdFree_h
            }
        return ret;                                                 // restituisco il puntatore al processo
        }
    }
    return NULL;                                                    // altrimenti restituisco NULL
}