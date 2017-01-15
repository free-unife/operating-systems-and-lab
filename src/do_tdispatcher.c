/*
 * do_tdispatcher.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * do_tdispatcher.c
 *
 * Il file contiene la funzione do_tdispatcher eseguita dal thread dispatcher.
 * La funzione invia i messaggi tra client.
 *
 */

#include "main_includes.h"

/*
 * Funzione eseguita dal thread dispatcher.
 * La funzione è chiamata una volta dal thread main. Viene passata la testa della lista utenti, cioè head_usrs_list
 * e la struttura contenente i mutex ed altri elementi, td_mux_struct.
 * All'interno del cliclo while(go) il thread rimane in attesa se non ci sono messaggi sul buffer circolare, grazie alla
 * variabile c -> count. Non appena arriva un messaggio il thread legge il numero di richiesta tokenizzando
 * il messaggio presente nella posizione di lettura del buffer. Viene usato ':' come delimitatore.
 * Il numero della richiesta viene salvato in req_str, poi con sscanf viene trasformato in intero e salvato
 * in req_do_tdispatcher. Attraverso il puntatore tmp viene fatto lo scorrimento della lista degli utenti e vengono
 * cercati tutti gli utenti a cui mandare il messaggio con la variabile tmp -> req (req = request).
 * Viene fatta un'altra toenizzazione per estrarre il messaggio e salvari nella variabile msg.
 * Prima della fine del ciclo while(go) viene decrementato c -> count, il contatore del numero di thread che stanno
 * agendo sul buffer circolare.
 *
 * i = indice generico per scorrere la stringa del buffer circolare.
 * j = indice usato come copia di i e per seplificarne alcune operazioni.
 * k = indice usato per iterare stringa msg.
 */
void *do_tdispatcher (void *thread_dispatcher_args)
{

	struct thread_dispatcher_struct *dispatcher_args; // nome locale
	int req_do_tdispatcher; // variabile locale che rappresenta il numero della richiesta
	char *msg, *req_str; // msg contiene messaggio da mandare agli utenti. req_str contiene il numero di richiesta (come tipo char *)
	td_c_p *c; // nome locale per il punatore alla struttura
	hdata_t *search, *sender_hinfo; // usato per ricerca utenti, usato per riccavare csd sender
	users *head_do_tdispatcher, *tmp; // variabili per scorrere la lista degli utenti
	int ret_thrd_op; // return thread operation
	int i = 0, j = 0, k = 0; // vedi sopra
	int remote_usr_disconn = 0, total_usr_conn = 0, sender_csd = -1, dotdot = 0; // variabili che vengono usate solo se l'utente remoto non è collegato
	char *sender_local; // contiene username del mittente


	dispatcher_args = (struct thread_dispatcher_struct *) thread_dispatcher_args; // istanza locale della struttura con casting da void a struct
	head_do_tdispatcher = dispatcher_args -> head_usrs_list; // riassegazione delle vatiabili
	c = dispatcher_args -> td_mux_struct;

	while (go)
	{

		ret_thrd_op = pthread_mutex_lock (&c -> mux_tdispatcher); // mutua esclusione per accedere alla tabela hash e per le altre operazioni
		if (ret_thrd_op != 0)
			err_handler (E_TDISPATCHER_OP, 'x', 'w');
		while (c -> count == 0) // se non ci sono messaggi non può consumarli...
		{
			ret_thrd_op = pthread_cond_wait (&c -> empty, &c -> mux_tdispatcher); // ...quindi attende una signal sul mux
			if (ret_thrd_op != 0)
				err_handler (E_TDISPATCHER_OP, 'x', 'w');

		}

		req_str = (char *) malloc (sizeof (char) * REQ_NUM); // strlen (req_str) = log10(K) + 1, ma per essere sicuri allochiamo molto più pazio
		if (req_str == NULL)
			err_handler (E_MALLOC, 'x', 'w');

		bzero ((void *) req_str, REQ_NUM); // azzeramento stringa

		i = 0;
		while (c -> buff_r[c -> pos_r][i] != ':') // estrai il numero di richiesta e salvalo in un char *
		{
			req_str[i] = c -> buff_r[c -> pos_r][i];
			i++;
		}

		sscanf (req_str, "%d", &req_do_tdispatcher); // prende l'intero corrispondente alla richiesta
		free (req_str);

		total_usr_conn = 0;
		remote_usr_disconn = 0;

		// cerca nella lista lo/gli utenti che corrsipondono all'intero appena estratto
		tmp = head_do_tdispatcher;
		while (tmp != NULL) // usando la lista posso trattare MSG_SINGLE e MSG_BRDCAST come un unico caso
		{
			j = i + 1; // j è la posizione corrente dopo aver estatto il numero di richiesta e i ':'
			k = 0; // indice della stringa msg

			if (tmp -> req == req_do_tdispatcher) // match numero richiesta
			{
				search = CERCAHASH (tmp -> username, H); // ricerca utente nella tabella hash
				if (search != NULL) // se esiste l'utente è vero
				{
					if (search -> sockid != -1) // se arrivo qui sono sicuro di poter mandare il messaggio
					{
						msg = (char *) malloc (sizeof (char) * strlen (c -> buff_r[c -> pos_r]) - j + 1);
						if (msg == NULL)
							err_handler (E_MALLOC, 'x', 'w');

						bzero ((void *) msg, strlen (c -> buff_r[c -> pos_r]) - j + 1);

						while (c -> buff_r[c -> pos_r][j] != '\0')
						{
							msg[k] = c -> buff_r[c -> pos_r][j]; // copio solo la parte di stringa utile
							j++;
							k++;
						}
						msg [k] = '\0'; // per sicurezza

						write_sock (NULL, msg, search -> sockid); // invio del messaggio al/ai client

						free (msg);

						total_usr_conn++; // viene usata questa variabile per evitare di entrare nel prossimo if se si tratta di MSG_BRDCAST e c'è solo un utente scolegato
					}
					else // utente a cui mandare msg non collegato (ma esistente)
						remote_usr_disconn++; // contatore utenti scollegati
				}
			}
			tmp = tmp -> next; // avanzamento al prossimo elemento della lista
		}

		if (remote_usr_disconn == 1 && total_usr_conn == 0) // se ho mandato un messaggio ad un singolo utente, e questo non è collegato
		{
			sender_local = (char *) malloc (sizeof (char) * 257);
			if (sender_local == NULL)
				err_handler (E_MALLOC, 'x', 'w');

			bzero ((void *) sender_local, 257);

			dotdot = 0; // conta il numero di ':'
			j = 0;
			while (c -> buff_r[c -> pos_r][j] != '\0' && dotdot < 3)
			{
				if (c -> buff_r[c -> pos_r][j] == ':')
					dotdot++;
				j++;
			}

			k = 0;
			while (c -> buff_r[c -> pos_r][j] != ':')
			{
				sender_local[k] = c -> buff_r[c -> pos_r][j]; // ricavo mittente
				j++;
				k++;
			}

			sender_hinfo = CERCAHASH (sender_local, H); // ricerca hash info mittente
			if (sender_hinfo != NULL)
				sender_csd = sender_hinfo -> sockid; // ricavo csd mittente

			write_sock (WR_SOCK_ERR, E_USR_NOT_CONN, sender_csd); // mando messaggio di utente scollegato al mittente
			err_handler_argv (E_USR_NOT_CONN, sender_local, 0, 'w'); // scrittura sul log e stderr del server

			free (sender_local);
		}

		free (c -> buff_r[c -> pos_r]);

		c -> count--; // decremento numero thread che agiscono sul buffer circolare
		c -> pos_r = (c -> pos_r + 1) % K; // avanzo posizione lettura, garantendo circolarità buffer

		ret_thrd_op = pthread_cond_signal (&c -> full);
		if (ret_thrd_op != 0)
			err_handler (E_TDISPATCHER_OP, 'x', 'w');
		ret_thrd_op = pthread_mutex_unlock (&c -> mux_tdispatcher);
		if (ret_thrd_op != 0)
			err_handler (E_TDISPATCHER_OP, 'x', 'w');

	}

	pthread_exit (NULL);

}
