/*
 * do_tworker.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * do_tworker.h
 *
 * Contiene macro per numero max richieste errate client e definizioni strutture fondamentali.
 *
 */

#include "main_includes.h"

#define MAX_BAD_REQ 10 // numero massimo di richieste errate prima della disconnessione forzata


struct _tw_log_c // thread worker log control
{
	pthread_mutex_t mux_log; // semaforo per log file
	pthread_cond_t cond_log; // coda di attesa/condition per log file
	int w_logfile; // numero di thread che stanno agendo sul log file
};

typedef struct _tw_log_c tw_log_c; // thread dispatcher communication protocol
tw_log_c L; // istanza struttura

struct thread_worker_struct // dati thread main
{
	int csd_socket; // client socket descriptor
	users *head_usrs_list; // testa lista utenti
	td_c_p *td_mux_struct; // serve per comunicazione con thread dispatcher
        tw_log_c *tw_mux_struct; // scrittura su log file
};

struct thread_worker_struct thread_worker; // oggetto che contiene i dati da passare al thread main
void *do_tworker (void *thread_worker_args); // funzione eseguita dal thread main

// prototipi funzioni presenti all'interno di do_tworker
int get_and_parse_cmd (msg_t *msg_t_local, int csd_local);
void write_logfile_cmd (msg_t *msg_t_local, char *user_name_local, char *receiver_local, char *time_string, tw_log_c *l);
int demar (char *buff_msg_local, msg_t *msg_t_local);
