/*
 * hash.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * hash.h
 *
 * Contiene strutture lista utenti e tabella hash.
 *
 */

#define SL 256
#define HL 997 // numero primo per calcolare hash

struct _users // struttura lista utenti
{
	char *username; // nome utente
	int req; // numero della richiesta da usare per la comunicazione tra tworker e tdispatcher
	struct _users *next; // puntatore all'elemento successivo
};

typedef struct _users users; // ridefinizione struttura
users usrs; // dichiarazione instanza globale struttura

struct struct_hdata_t { // struttura hash
  char * uname;     // username
  char * fullname;  // full name
  char * email;     // email address
  int  sockid;      // socket identifier; -1 = not connected
};

typedef struct struct_hdata_t hdata_t;
typedef struct cella * lista;
typedef lista          posizione;
typedef lista * hash_t;

struct cella {
  posizione precedente;
  void *    elemento;
  posizione successivo;
};

// prototipi
lista CREALISTA ();
int LISTAVUOTA (lista L);
posizione PRIMOLISTA (lista L);
posizione ULTIMOLISTA (lista L);
posizione SUCCLISTA (posizione p);
posizione PREDLISTA (posizione p);
int FINELISTA (posizione p, lista L);
void INSLISTA (void * data, posizione * p);
void CANCLISTA (posizione * p);

int hashfunc(char * k);
hdata_t * CERCALISTA ( char * key, lista L );
hash_t CREAHASH ();
void * CERCAHASH(char * key, hash_t H);
void INSERISCIHASH (char * key, hdata_t * elem, hash_t H);
users *ADD_USR_TO_LIST (char *username_local, users *head_local);
void GET_CONN_USR (hash_t H_local, char *buffer_local, users *head_local);
