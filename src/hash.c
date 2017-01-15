/*
 * hash.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/* WARNING: SOME PARTS OF THIS FILE ARE COPYRIGHTED "schifano 2013" */

/*
 * hash.c
 *
 * Il file contiene tutte le funzioni per gestire la tabella hash.
 * Sono incluse le due funzioni in coda al file, una per aggiungere
 * un utente nella lista utenti, l'altra per ottenere la lista degli
 * utenti collegati.
 */
/*************************************/
/* Library list: schifano 2013       */
/*************************************/
#include "main_includes.h"

lista CREALISTA () {
  lista L;
  L = (lista) malloc(sizeof (struct cella) ); 
  L->successivo = L;
  L->precedente = L;
  return L;
}


int LISTAVUOTA (lista L) {
  int listavuota;
  listavuota = ((L->successivo == L) && (L->precedente == L)) ? 1 : 0;
  return listavuota;
}

  
posizione PRIMOLISTA (lista L) {
  return L->successivo;
}


posizione ULTIMOLISTA (lista L) {
  return L->precedente;
}


posizione SUCCLISTA (posizione p) {
  return p->successivo;
}


posizione PREDLISTA (posizione p) {
  return p->precedente;
}


int FINELISTA (posizione p, lista L) {
  int finelista;
  finelista = (p == L) ? 1 : 0;
  return finelista;
} 


void INSLISTA (void * data, posizione * p) {
  struct cella * tmp;
  
  tmp = (struct cella *) malloc(sizeof(struct cella));
  
  tmp->precedente = (*p)->precedente; 
  tmp->successivo = (*p);
  
  tmp->elemento = data;
  
  (*p)->precedente->successivo = tmp;
  (*p)->precedente             = tmp;
  
  (*p) = tmp;

//  free (tmp); // agginta per evitare memory leaks

}


void CANCLISTA (posizione * p) {
  posizione tmp;
  
  tmp = (*p);
  
  (*p)->precedente->successivo = (*p)->successivo;
  (*p)->successivo->precedente = (*p)->precedente;
  
  (*p) = (*p)->successivo;
  
  free(tmp);  
}

/*************************************/
/* Library hash: schifano fabio 2013 */
/*************************************/

//////////////////////////////////////////////////////////////////////

int hashfunc(char * k) {
  int i = 0;
  int hashval = 0;
  for (i=0; (unsigned int) i < strlen(k); i++ ) {
    hashval = ((hashval*SL) + k[i]) % HL;
  }
  return hashval;
}

//////////////////////////////////////////////////////////////////////
// restituisce la posizione della cella che contiene la chiave key == uname

hdata_t * CERCALISTA ( char * key, lista L ) {
  int found;
  posizione p;
  hdata_t * r;
  char * k;
  r = NULL;
  p = PRIMOLISTA(L);
  found = 0;
  //fprintf(stderr, "looking for key: %s\n", key);
  while ( (!found) && (!FINELISTA(p,L)) ) {
    k = ((hdata_t *)(p->elemento))->uname;
    if ( strcmp (k, key ) == 0 ) {
      r = p->elemento;
    }
    p = SUCCLISTA(p);
  }
  return r;
}

//////////////////////////////////////////////////////////////////////

hash_t CREAHASH () {
  hash_t H;
  int i;
  H = (hash_t) malloc(HL*sizeof(lista));
  for ( i=0; i < HL; i++ ) {
    H[i] = CREALISTA(); 
  }
  return H;
}

//////////////////////////////////////////////////////////////////////
// cerca un elemento nella tabella Hash
// restituisce un "hdata_t *" oppure NULL se non esiste
 
void * CERCAHASH(char * key, hash_t H) {
  int i;
  i = hashfunc(key);
  return CERCALISTA(key, H[i]);
}

//////////////////////////////////////////////////////////////////////
// inserisce l'elemento hdata_t * nella tabella hash H

void INSERISCIHASH (char * key, hdata_t * elem, hash_t H) {
  int i;
  posizione p;
  i = hashfunc(key);
  if ( CERCAHASH(key, H) == NULL ) {
    p = PRIMOLISTA(H[i]);
    INSLISTA((void *)elem, &p); 
  }
}

//////////////////////////////////////////////////////////////////////

/*
 * Funzione per aggiunta di un utente nella lista di tipo users.
 * Alla funzone viene passato il puntatore che si riferice alla stringa del nome utente da aggiungere,
 * username_local. Viene anche passata la testa della lista, perchè il nuovo utente deve essere
 * inserito in coda. Viene alocato un puntatore di struttura, usr, di tipo users. Vengono
 * riempiti i campi della struttua ed il nodo viene posizionato in coda, scorrendo tutta la lista
 * con tmp.
 * Alla fine viene ritornata la testa della lista, perchè questa cambia se si inserisce il
 * primo utente.
 */
users *ADD_USR_TO_LIST (char *username_local, users *head_local)
{

	users *usr, *tmp; // puntatori locali di tipo users


	usr = (users *) malloc (sizeof (users)); // allocazione spazio per nuovo utente
	if (usr == NULL)
		err_handler (E_MALLOC, 'x', 0);
	bzero ((void *) usr, sizeof (users));

	usr -> username = (char *) malloc (sizeof (char) * (strlen (username_local) + 1)); // allocazione spazio per nome utente
	if (usr == NULL)
		err_handler (E_MALLOC, 'x', 0);
	bzero ((void *) usr -> username, strlen (username_local) + 1);

	usr -> username = username_local; // inserimento username
	usr -> req = -1; // inizializzo l'intero che rappresenterà la richiesta. -1 = valore di default, cioè nessuna richiesta
	usr -> next = NULL; // il nodo successivo, non esistendo ancora, deve puntare a NULL

	if (head_local == NULL) // se è il primo elemento dell lista...
		head_local = usr; // ...assegna la testa all'elemento appena creato
	else // altrimenti se c'è almeno un utente...
	{
		tmp = head_local; // ...inizia lo scorrimento della lista dal primo elemento
		while (tmp -> next != NULL) // se esiste un nodo successivo...
			tmp = tmp -> next; // ...scorri fino all'ultimo elemento

		tmp -> next = usr; // salva il nuovo elemento in coda alla lista

	}

	return head_local; // ritorna la testa della lista

}

/*
 * Funzione per generare la lista degli utenti collegati usando lista utenti
 * e tabella hash.
 * La funzione prende in input la tabella hash H_local), un buffer (buffer_local)
 * e testa della lista utenti (head_local).
 * Per ogni utente colleagto viene concatenato lo username (uname) in buffer_local.
 */
void GET_CONN_USR (hash_t H_local, char *buffer_local, users *head_local)
{

	posizione p;
	users *tmp; // puntatore locale
	int i; // intero dove verrà salvato il valore hash


	tmp = head_local;
	if (tmp == NULL) // se la testa è vuota...
		err_handler (E_EMPTY_USR_LIST, 'x', 'w'); // ...fatal error

	while (tmp != NULL) // scorri tutta la lista
	{
		i = hashfunc (tmp -> username); // calcola valore hash per utente corrente
		p = PRIMOLISTA (H_local[i]); // assegnazione testalista trabocco
		while ((!FINELISTA(p, H_local[i]))) // scorri lista trabocco
		{
			if (((hdata_t *) (p -> elemento)) -> sockid != -1) // se l'utente è collegato...
			{
				strncat (buffer_local, ((hdata_t *) (p -> elemento)) -> uname, strlen (((hdata_t *) (p -> elemento)) -> uname)); // ...concatena stringa utente corrente a stringa finale
				strncat (buffer_local, ":", strlen (":")); // concatena anche i ':' per separare gli user name
			}
			p = SUCCLISTA (p); // avanza al prossimo elemento della lista a trabocco
		}
		tmp = tmp -> next; // avanza al prossimo elemento della lista utenti
	}

	buffer_local[strlen (buffer_local) - 1] = '\0'; // aggiungo il terminatore di stringa al posto dell'ultimo ':'

}

