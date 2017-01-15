/*
 * do-tdispatcher.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * do_tdispatcher.h
 *
 * Contiene definizioni strutture thread dispatcher e macro.
 *
 */

#define K SOMAXCONN / 2 // K è la dimensione del buffer circolare
#define REQ_NUM K / 2 // numero max di richieste

struct _td_c_p
{
	char *buff_r[K]; // buffer delle richieste e i messaggi da tworker a dispatcher
	int pos_r; // buffer read position
	int pos_w; // buffer write position
	int count; // numero di richieste ancora da smaltire
	pthread_mutex_t mux_tdispatcher; // mutex per eseguire il lock
	pthread_cond_t full; // condizione buffer pieno
	pthread_cond_t empty; // condizione buffer vuoto
};

typedef struct _td_c_p td_c_p; // thread dispatcher communication protocol
td_c_p C; // istanza struttura

struct thread_dispatcher_struct // dati da passare al thread dispatcher
{
	users *head_usrs_list; // testa lista utenti
	td_c_p *td_mux_struct; // viene passata l'istanza della struttura sopra
};

struct thread_dispatcher_struct thread_dispatcher; // oggetto che contiene i dati da passare al thread main

void *do_tdispatcher (void *thread_dispatcher_args); // funzione eseguita dal thread dispatcher
