/*
 * do_tworker.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * do_tworker.c
 *
 * Il file include la funzione eseguita dal thread_worker, write_logfile_cmd e demar.
 *
 */

#include "main_includes.h"

/*
 * Funzione eseguta dal thread worker
 * La funzione esegue un ciclo while controllato dalla variabile go e immediatamente chiama la funzione
 * get_and_parse_cmd per leggere messaggi in arrivo sul socket client. La funzione ritorna un valore intero
 * che viene salvato in ret_get. Se ret_get == 1 significa che il client si è scollegato per qualche motivo,
 * allora bisogna controllare se è già loggato oppure no. In caso affermativo bisogna impostare il suo
 * sockid = -1, logged_in = 0 e closed_csd = 1.
 * Se ret_get = 0 allora si tratta di un messaggio valido, quindi viene
 * controlato se l'utente è già loggato oppure no con la variabile logged_in. A seconda del valore di
 * questa variabile l'utente può fare o meno alcune operazioni.
 * La variabile closed_csd indica se è ancora presente una connessione con il client, dove 0 = si, 1 = no.
 * bad_requests identifica il numero di comandi errati nviati dal client al server. Se questo numero supera
 * quello definito dalla macro MAX_BAD_REQ l'utente viene immediatamente scollegato.
 * Dopo la lettura del comando, il programma genera un timestamp per poi gestire il comando.
 */

int r = -1; // contatore globale delle richieste (dei messaggi) per il buffer circolare e per la lista utenti.

void *do_tworker (void *thread_worker_args) // procedura eseguita dal thread worker
{

	int csd; // client socket descriptor (variabile locale)

	struct thread_worker_struct *worker_args; // nome locale

	msg_t msg_t_l; // dichiaro istanza struttura di tipo msg_t
	msg_t *msg_t_do_tworker; //dicharo puntatore di tipo msg_t
	msg_t_do_tworker = &msg_t_l; // in questo modo ottengo l'indirizzo di memoria della struttura msg_t_l. Questo sarà usato anche in altre funzioni

	hdata_t *insert, *search; // hash per ricerca/inserimento
	int ret_get, ret_str_token_read; // la prima variabile che serve a controllare se chiudere csd, l'altra è usata nel caso REGLOG, che = 0 se è andato a buon fie str_token_read

	char time_string[25];

	int logged_in = 0; // variabile che indica se l'utente ha già fatto il login. se la variabile rimane a zero significa che non può essere abilitato al servizio
	int closed_csd = 0; // variabile per controllare se il socket è stato chiuso, serve anche per fare break del while

	users *head_do_tworker, *tmp; // con tmp si scorre la lista degli utenti, head_do_tworker è la testa locale.

	td_c_p *c; // dihiarazione puntatori locali per le strutture dei mutex (e altro) del thread dispatcher e thread worker. td_c_p = thread dispatcher communication protocol
	tw_log_c *l; // tw_log_c = thread worker log control

	char *local_user; // stringa che ontiene username dell'utente che ha fatto login o reglog

	int user_count = 0; // conta il numero totale di utenti a cui mandare un messaggio cioè conta numero utenti totali collegati

	char *msg_to_client; // usata per mandare alcuni messaggi al client
	size_t len_msg_to_client;

	char *users_string; // stringa da usare per MSG_LIST. Qui vengono salvati tutti gli uname degli utenti, ognuno separato da ':'

	int ret_thrd_op; // altre variabili di ritorno
	ssize_t ret_io_bytes;
	int ret_close;

	int bad_requests = 0; // variabile che definisce il numero di volte in cui l'utente ha inviato comandi errati

	// -- FINE DICHIARAZIONI --

	worker_args = (struct thread_worker_struct *) thread_worker_args; // istanza locale della struttura con casting da void a struct
	csd = worker_args -> csd_socket; // riempimento variabili locali
	head_do_tworker = worker_args -> head_usrs_list;
	c = worker_args -> td_mux_struct;
	l = worker_args -> tw_mux_struct;

	while (go)
	{

		if (bad_requests >= MAX_BAD_REQ && closed_csd == 0) // se l'utente manda troppi comandi errati...
		{
			if (logged_in == 1) // ...se l'utente è loggato...
			{
				search = CERCAHASH (local_user, H); // ...ricerca utente nella tabella hash per impostare sockid = -1
				if (search != NULL)
					search -> sockid = -1;
				else // se non c'è l'utente in tabella hash...
				{
					write_sock (WR_SOCK_ERR, E_NOT_THAT_USR_IN_TABLE, csd);
					err_handler (E_NOT_THAT_USR_IN_TABLE, 0, 'w');
				}
			}
			err_handler (E_TOO_MANY_INVALID_CMDS, 0, 'w'); // stampa messaggio di errore sul server
			write_sock (WR_SOCK_ERR, E_TOO_MANY_INVALID_CMDS, csd); // invia messaggio errore al client

			ret_close = close (csd); // ...chiudi la connessione
			if (ret_close == -1)
				err_handler(E_CLOSE_SOCK, 0, 'w');
			logged_in = 0; // l'utente è ora scollegato
			closed_csd = 1;
		}

		if (closed_csd == 0) // Se l'utente è ancora collegato allora:
			ret_get = get_and_parse_cmd (msg_t_do_tworker, csd); // leggi il comando dal socket
		else // altrimenti il socket è stato chiuso in precedenza quindi non devo più fare operazioni. Esco dal ciclo while
			break;

		if (ret_get == 1) // disconnessione forzata client
		{
			if (logged_in == 1) // se l'uttente è loggato va azzerato il sockd dalla tabella
			{
				search = CERCAHASH (local_user, H); // ricerca utente nella tabela hash per impostare sockid = -1
				if (search != NULL)
					search -> sockid = -1;
				else // vedi altro if sopra
				{
					write_sock (WR_SOCK_ERR, E_NOT_THAT_USR_IN_TABLE, csd);
					err_handler (E_NOT_THAT_USR_IN_TABLE, 0, 'w');
				}
			}
			ret_close = close (csd); // disconnessione
			if (ret_close == -1)
				err_handler(E_CLOSE_SOCK, 0, 'w');

			logged_in = 0; // l'utente non è più collegato
			closed_csd = 1;

			break; // arresta questo ciclo while perchè l'utente si è scollegato
		}
		else if (ret_get == 2) // comando sconosciuto
			msg_t_do_tworker -> type = 'U'; // U = unknown command, così cade nel caso default
		else if (ret_get == 3) // formato non valido
			msg_t_do_tworker -> type = 'N'; // N = Not valid format


		timestamp_crt (time_string, 25); // creo il timestamp da mettere nell'intestazione del log. E' qui perchè deve registrare il momento della ricezione del comando, non il momento della scittura del comando sul log file, che, a causa del semaforo, potrebbe essere a posteriori.

		if (logged_in == 0) // l'utente non e' ancora abilitato al servizio...
		{
			switch (msg_t_do_tworker -> type)
			{
				case MSG_LOGIN: // un utente che effettua login deve già essere registrato
				{
					search = CERCAHASH (msg_t_do_tworker -> msg, H); // controllo in lista trabocco
					if (search != NULL) // se c'è user in tabella, accetta
					{
                	        	        if (search -> sockid == -1) // se l'utente non è ancora collegato...
                        			{
							search -> sockid = csd; // salvataggio sockid nella lista. alla disconnessione deve essere settato a -1

							write_logfile_cmd (msg_t_do_tworker, msg_t_do_tworker -> msg, "", time_string, l); // scrivi comando login sul log file
							write_sock (WR_SOCK_OK, NULL, csd); // manda messaggio OK al client

							local_user = strdup (msg_t_do_tworker -> msg); // copio la stringa msg (che contiene il nome utente appena loggato).
							if (local_user == NULL)
								err_handler (E_MALLOC, 'x', 'w');

							logged_in = 1; // utente abilitato al servizio

						}
						else // utente già loggato...
						{

							err_handler_argv (E_USR_ALREADY_LOGGED, msg_t_do_tworker -> msg, 0, 'w'); // scrivi errore sul server
							write_sock (WR_SOCK_ERR, E_USR_ALREADY_LOGGED, csd); // scrivi errore sul socket client

							ret_io_bytes = close (csd); // rifiuto/chiusura della connessione
							if (ret_io_bytes == -1)
								err_handler (E_CLOSE_SOCK, 0, 'w');

							closed_csd = 1;
						}
					}
                        		else // non esiste l'utente specificato/utente non registrato
					{
						err_handler_argv (E_USR_NOT_REG, msg_t_do_tworker -> msg, 0, 'w'); // scrivi errore sul server
						write_sock (WR_SOCK_ERR, E_USR_NOT_REG, csd); // scrivi errore sul socket

                		                ret_io_bytes = close (csd); // rifiuto/chiusura della connessione
						if (ret_io_bytes == -1)
							err_handler (E_CLOSE_SOCK, 0, 'w');

						closed_csd = 1;
					}

					break;
				}
				case MSG_REGLOG: // regitrazione e login
				{

					insert = (hdata_t *) malloc (sizeof (hdata_t)); // nuovo elemento tabella hash
					if (insert == NULL)
						err_handler (E_MALLOC, 'x', 'w');

					ret_str_token_read = str_token_read (msg_t_do_tworker -> msg, insert); // nella registrazione va tokenizzato la parte msg, che normalmente contiene 3 campi
					if (ret_str_token_read == 0) // ok
					{
						search = CERCAHASH (insert -> uname, H); // controllo in lista trabocco
						if (search == NULL) // aggiunta utente in tabella e acceta connessione, se non è già presente (altrimenti restituisci errore)
						{
							insert -> sockid = csd; // salvataggio sokid nella lista. alla disconnessione deve essere settato a -1
							INSERISCIHASH (insert -> uname, insert, H); // inserimento nuovo utente nella lista

							head_do_tworker = ADD_USR_TO_LIST (insert -> uname, head_do_tworker); // aggiungo l'utente nella lista

							write_logfile_cmd (msg_t_do_tworker, insert -> uname, "", time_string, l); // scrivi comando sul log file
							write_sock (WR_SOCK_OK, NULL, csd);

							local_user = strdup (insert -> uname); // copio la stringa dello user name
							if (local_user == NULL)
								err_handler (E_MALLOC, 'x', 'w');

							logged_in = 1; // utente abilitato al servizio

						}
						else
						{
							err_handler_argv (E_USR_ALREADY_REG, msg_t_do_tworker -> msg, 0, 'w'); // analogo al caso login
							write_sock (WR_SOCK_ERR, E_USR_ALREADY_REG, csd);

	               			                ret_io_bytes = close (csd); // rifiuto della connessione
							if (ret_io_bytes == -1)
								err_handler (E_CLOSE_SOCK, 0, 'w');

							closed_csd = 1;
						}
					}
					else
						write_sock (WR_SOCK_ERR, E_INVALID_USR_NAME, csd);

					break;
				}
				case MSG_LOGOUT: // un utente non loggato non può eseguire questi comandi
				case MSG_LIST:
				case MSG_SINGLE:
				case MSG_BRDCAST:
				{
					err_handler (E_MUST_LOGIN_FOR_CMD, 0, 'w');
					write_sock (WR_SOCK_ERR, E_MUST_LOGIN_FOR_CMD, csd);

					break;
				}
				case 'N': // formato invalido
				{
					err_handler (E_INVALID_FORMAT, 0, 'w');
					write_sock (WR_SOCK_ERR, E_INVALID_FORMAT, csd);
					bad_requests++;
					break;
				}
				default: // comando sconosciuto
				{
					err_handler (E_UNKNOWN_CMD, 0, 'w');
					write_sock (WR_SOCK_ERR, E_UNKNOWN_CMD, csd);
					bad_requests++;

					break;
				}

			}

		}
		else if (logged_in == 1) // se l'utente si è loggato:
		{
			switch (msg_t_do_tworker -> type)
			{

				case MSG_LOGOUT: // vedi LOGIN e REGLOG
				{
					search = CERCAHASH (local_user, H); // ricerca utente nella tabella hash per impostare sockid = -1
					if (search != NULL)
					{
						search -> sockid = -1;
						write_logfile_cmd (msg_t_do_tworker, local_user, "", time_string, l); // write cmd on logfile
					}
					else
					{
						err_handler_argv (E_NOT_THAT_USR_IN_TABLE, local_user, 0, 'w');
						write_sock (WR_SOCK_ERR, E_NOT_THAT_USR_IN_TABLE, csd);
					}

               		                ret_io_bytes = close (csd); // rifiuto della connessione
					if (ret_io_bytes == -1)
						err_handler (E_CLOSE_SOCK, 0, 'w');

					logged_in = 0;
					closed_csd = 1;
					break;

				}
				case MSG_LIST:
				{
					user_count = 0;

					tmp = head_do_tworker;
					while (tmp != NULL)
					{
						search = CERCAHASH (tmp -> username, H);
						if (search != NULL)
							if(search -> sockid != -1)
								user_count++; // conta numero utenti collegati

						tmp = tmp -> next;
					}
					if (user_count == 0) // controllo numero utenti
						err_handler (E_USR_NUMBER, 'x', 'w');

					users_string = (char *) malloc (sizeof (char) * ((user_count * 256) + user_count + (256 * SOMAXCONN) + 1)); // diamo anche dello spazio extra (SOMAXCONN) nel caso si colleghino altri utenti da questo punto al riempimento della stringa. Qui vengono anche contati i ':' (che corrispondono a user_count (-1). Includiamo anche '\0'
					if (users_string == NULL)
						err_handler (E_MALLOC, 'x', 'w');

					bzero ((void *) users_string, sizeof (char) * ((user_count * 256) + user_count + (256 * SOMAXCONN) + 1)); // azzero l'array per evitare problemi

					GET_CONN_USR (H, users_string, head_do_tworker); // questa funzione in hash.c controlla tutti i csd != -1 e salva gli uname in users_string separati da un ':'

					len_msg_to_client = snprintf (0, 0, "%d", (int) strlen (users_string)); // ricavo numero caratteri per l'intero che rappresenta la lunghezza della stringa users_string
					len_msg_to_client += strlen (users_string) + 2; // 1 per i ':', 1 per '\0'
					msg_to_client = (char *) malloc (sizeof (char) * len_msg_to_client);
					if (msg_to_client == NULL)
						err_handler (E_MALLOC, 'x', 'w');

					bzero ((void *) msg_to_client, sizeof (char) * len_msg_to_client); // azzero l'array per evitare problemi

					snprintf (msg_to_client, len_msg_to_client, "%d:%s", (int) strlen (users_string) + 1, users_string); // salvataggio stringa da scrivere sul socket
					write_sock (WR_SOCK_LIST_SRV, msg_to_client, csd); // invio stringa utenti collegati al client

					write_logfile_cmd (msg_t_do_tworker, local_user, "", time_string, l); // scrittura comando sul log file

					free (users_string);
					free (msg_to_client);

					break;

				}
				case MSG_SINGLE:
				case MSG_BRDCAST:
				{

					ret_thrd_op = pthread_mutex_lock (&c -> mux_tdispatcher); // mutua esclusione per accedere alla tabela hash e per le altre operazioni
					if (ret_thrd_op != 0)
						err_handler (E_TWORKER_OP, 'x', 'w');
					while (c -> count == K) // mentre il buffer è pieno (K è la macro che definisce la dimensione del buffer circolare)...
					{
						ret_thrd_op = pthread_cond_wait (&c -> full, &c -> mux_tdispatcher); // ...attende una signal sul mux
						if (ret_thrd_op != 0)
							err_handler (E_TWORKER_OP, 'x', 'w');
					}


					r = (r + 1) % (REQ_NUM * 2); // incremento il numero della richiesta r. all'inizio r = -1
					user_count = 0;

					tmp = head_do_tworker;
					while (tmp != NULL)
					{

						if (msg_t_do_tworker -> type == MSG_BRDCAST)
						{
							tmp -> req = r; // associo ad ogni utente il numero di richiesta
							user_count++; // aumento contatore degli utenti a cui mandare msg
							write_logfile_cmd (msg_t_do_tworker, local_user, tmp -> username, time_string, l); // write cmd on logfile
						}
						else // msg ad un singolo utente
							if (strcmp (msg_t_do_tworker -> receiver, tmp -> username) == 0) // individuazione utente
							{
								tmp -> req = r; // vedi sopra
								user_count++;
								write_logfile_cmd (msg_t_do_tworker, local_user, msg_t_do_tworker -> receiver, time_string, l); // write cmd on logfile
								//write_sock (NULL, ":A::::", csd); // manda messaggio keepalive al client
							}

						tmp = tmp -> next;
					}

					if (user_count < 1) // se non c'è almeno un utente (compreso chi invia) a cui mandare il messaggio, ritorna errore non fatale.
					{
						err_handler_argv (E_MSG_TO_USR, msg_t_do_tworker -> receiver, 0, 'w');
						write_sock (WR_SOCK_ERR, E_MSG_TO_USR, csd); // write it on socket
					}

					len_msg_to_client = snprintf(0, 0, "%u", msg_t_do_tworker -> msglen); // ricavo numero caratteri per l'intero che rappresenta la lunghezza del messaggio
					len_msg_to_client += snprintf(0, 0, "%d", r); // ricavo numero caratteri per l'intero che rappresenta il numero della richiesta
					len_msg_to_client += 8 + strlen (local_user) + strlen (msg_t_do_tworker -> msg);
					msg_to_client = (char *) malloc (sizeof (char) * len_msg_to_client);
					if (msg_to_client == NULL)
						err_handler (E_MALLOC, 'x', 'w');

					snprintf (msg_to_client, len_msg_to_client, "%d::%c:%s::%u:%s", r, msg_t_do_tworker -> type, local_user, msg_t_do_tworker -> msglen + 1, msg_t_do_tworker -> msg); // local_user è il sender
					msg_to_client[len_msg_to_client - 1] = '\0'; //aggiungo il terminatore di stringa

					c -> buff_r[c -> pos_w] = strdup (msg_to_client); // scrivo il messaggio nel buffer circolare, sulla posizione c -> pos_w
					if (c -> buff_r[c -> pos_w] == NULL) // controllo allocazione spazio sringa sul buffer circolare
						err_handler (E_MALLOC, 'x', 'w');

					free (msg_to_client);

					c -> count++; // incremento il contatore generale per il numero di richieste ancora da smaltire
					c -> pos_w = (c -> pos_w + 1) % K; // incremento posizione scrittura buffer circolare

					ret_thrd_op = pthread_cond_signal (&c -> empty);
					if (ret_thrd_op != 0)
						err_handler (E_TWORKER_OP, 'x', 'w');
					ret_thrd_op = pthread_mutex_unlock (&c -> mux_tdispatcher);
					if (ret_thrd_op != 0)
						err_handler (E_TWORKER_OP, 'x', 'w');

					break;
				}
				case MSG_LOGIN:
				case MSG_REGLOG: // se l'utente è già loggato non può fare di nuovo questi comandi
				{
					err_handler (E_ALREADY_LOGGED_FOR_CMD, 0, 'w');
					write_sock (WR_SOCK_ERR, E_ALREADY_LOGGED_FOR_CMD, csd);

					break;

				}
				case 'N':
				{
					err_handler (E_INVALID_FORMAT, 0, 'w');
                                        write_sock (WR_SOCK_ERR, E_INVALID_FORMAT, csd);
					bad_requests++;

					break;
				}
				case 'A': // messaggio keep alive, usato dall'applicativo client
					break;
				default:
				{
					err_handler (E_UNKNOWN_CMD, 0, 'w');
                                        write_sock (WR_SOCK_ERR, E_UNKNOWN_CMD, csd);
					bad_requests++;

					break;
				}

			}

		}

		if (closed_csd == 1)
			break; // break while se utente ha richiesto logout

	}

	numThreadAttivi--; // il lavoro del thread è terminato
	pthread_exit (NULL); // essendo detached l'uscita va fatta manualmente
}

/*
 * Funzione per la registrazione del comando nel log file in mutua esclusione.
 *
 * FORMATO
 * =======
 *
 * timestamp:command:sender[:receiver:message]
 *
 * Per prima cosa viene scritto il timestamp passato come argomento alla funzione, cioè time_string.
 * Poi viene letto il tipo di comando, cioè msg_t_local -> type.
 * Se si tratta di un comando di tipo send allora viene scritto anche il ricevente ed il messaggio.
 */
void write_logfile_cmd (msg_t *msg_t_local, char *user_name_local, char *receiver_local, char *time_string, tw_log_c *l)
{

	int ret_thrd_op; // return thread operation
	ssize_t ret_io_bytes, len_msg, len_dotdot = strlen (":"), len_user_name_local = strlen (user_name_local); // siccome vengono usate molto le strlen, vengono salvate in variabili altrimenti si calcolerebbe la lunghezza ad ogni chiamata di strlen


	ret_thrd_op = pthread_mutex_lock (&l -> mux_log); // mutua esclusione per log file
	if (ret_thrd_op != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');

	while (l -> w_logfile > 0) // attendi mentre c'è un altro thread che scrive
	{
		ret_thrd_op = pthread_cond_wait (&l -> cond_log, &l -> mux_log);
		if (ret_thrd_op != 0)
			err_handler (E_TWORKER_OP, 'x', 'w');

	}

	l -> w_logfile++; // il thread corrente sta scrivendo sul file

	ret_io_bytes = write (log_fd, time_string, 25);
	if (ret_io_bytes != 25)
		err_handler (E_LOG_FILE_WRITE, 0, 'w');

	ret_io_bytes = write (log_fd, ":", len_dotdot);
	if (ret_io_bytes != len_dotdot)
		err_handler (E_LOG_FILE_WRITE, 0, 'w');

	switch (msg_t_local -> type)
	{
		case MSG_LOGIN:
		{
			ret_io_bytes = write (log_fd, "login", strlen ("login"));
			len_msg = strlen ("login");
			break;
		}
		case MSG_LOGOUT:
		{
			ret_io_bytes = write (log_fd, "logout", strlen ("logout"));
			len_msg = strlen ("logout");
			break;
		}
		case MSG_REGLOG:
		{
			ret_io_bytes = write (log_fd, "reglog", strlen ("reglog"));
			len_msg = strlen ("reglog");
			break;
		}
		case MSG_LIST:
		{
			ret_io_bytes = write (log_fd, "list", strlen ("list"));
			len_msg = strlen ("list");
			break;
		}
	}
	switch (msg_t_local -> type) // va fatto di nuovo altrimenti andrebbero riscritte le stesse cose per ogni caso
	{
		case MSG_LOGIN:
		case MSG_LOGOUT:
		case MSG_REGLOG:
		case MSG_LIST:
		{
			if (ret_io_bytes != len_msg)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');

			ret_io_bytes = write (log_fd, ":", len_dotdot);
			if (ret_io_bytes != len_dotdot)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');
			ret_io_bytes = write (log_fd, user_name_local, len_user_name_local);
			if (ret_io_bytes != len_user_name_local)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');

			break;
		}
		case MSG_SINGLE:
		case MSG_BRDCAST:
		{
			ret_io_bytes = write (log_fd, user_name_local, strlen (user_name_local)); // scrivi mittente
			if (ret_io_bytes != len_user_name_local)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');

			ret_io_bytes = write (log_fd, ":", len_dotdot);
			if (ret_io_bytes != len_dotdot)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');

			ret_io_bytes = write (log_fd, receiver_local, strlen (receiver_local)); // scrivi destinatario
			if (ret_io_bytes != (ssize_t) strlen (receiver_local))
				err_handler (E_WRITE_SOCK, 0, 'w');

			ret_io_bytes = write (log_fd, ":", len_dotdot);
			if (ret_io_bytes != len_dotdot)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');

			ret_io_bytes = write (log_fd, msg_t_local -> msg, msg_t_local -> msglen); // scrivi messaggio
			if (ret_io_bytes != (ssize_t) msg_t_local -> msglen)
				err_handler (E_LOG_FILE_WRITE, 0, 'w');

			break;
		}
	}

	ret_io_bytes = write (log_fd, "\n", strlen ("\n")); // stampa a capo
	if (ret_io_bytes != (ssize_t) strlen ("\n"))
		err_handler (E_LOG_FILE_WRITE, 0, 'w');

	l -> w_logfile--; // la scrittura è terminata

	ret_thrd_op = pthread_cond_signal (&l -> cond_log);
	if (ret_thrd_op != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');

	ret_thrd_op = pthread_mutex_unlock (&l -> mux_log);
	if (ret_thrd_op != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');

}

/*
 * Funzione di demarshalling.
 * Questa funzione si occupa di scomporre il messaggio appena letto dal socket.
 * Vengono allocate 5 stringhe da 256 caratteri ognuno, con il puntatore alle 5 stringhe chk_elts.
 * Viene poi fatto la tokenizzazione con un clclo while, usando come delimitatore il carattere ':'.
 * Poi si controlla msg_t_local -> type, cioè il tipo di messaggio. Se si tratta di un tipo
 * valido la tokenizzazione prosegue.
 *
 * i = indice generico usato per iterare la stringa da tokenizzare cioè cp
 * j = indice che si rierisce al numero di token
 * k = indice per il carattere corrente del token
 *
 * Valori di ritorno
 * =================
 *
 * 0 = OK
 * 2 = tipo messaggio sconosciuto
 * 3 = errore formato messaggio
 * //65 = è stato ricevuto un messaggio di acknowledgment da re-inviare al client
 */
	/*	Legenda comm_prot client to server
	 *	==================================
	 *
	 *	type	=	{'L', 'R', 'S', 'B', 'I', 'X'}
	 *
	 *	MSG_LOGIN
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	strlen (msg)
	 *		msg		=	user_name
	 *
	 *	MSG_REGLOG
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	strlen (msg)
	 *		msg		=	user_name:Name Surname:email
	 *
	 *	MSG_LOGOUT
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	0
	 *		msg		=	NULL
	 *
	 *	MSG_LIST
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	0
	 *		msg		=	NULL
	 *
	 *	MSG_SINGLE
	 *		sender		=	NULL
	 *		receiver	=	user_name
	 *		msglen		=	strlen (msg)
	 *		msg		=	message
	 *
	 *	MSG_BRDCAST
 	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	strlen (msg)
	 *		msg		=	message
	 *
	 *
	 */
int demar (char *buff_msg_local, msg_t *msg_t_local)
{

	int i = 0, j = 0, k = 0; // indici
	char *cp, *chk_elts[5] = {NULL, NULL, NULL, NULL, NULL}; // stringhe temporanee per tokenizzare il messsaggio


	cp = strdup (buff_msg_local); // duplico la stringa originale
	if (cp == NULL)
		err_handler (E_MALLOC, 'x', 'w');

	for (i = 0; i < 4; i++)
	{
		chk_elts[i] = (char *) malloc (sizeof (char) * 257); // per semplicità alloco 257 (256 + '\0') caratteri per ogni array temporaneo. In questo modo tutti i parametri hanno una lunghezza fissata ma ottengo performances migliori rispetto ad una realloc carattere per carattere (che inoltre può provocare errori di segmentazione)
		if (chk_elts[i] == NULL)
			err_handler (E_MALLOC, 'x', 'w');
		bzero ((void *) chk_elts[i], 257); // azzero la stringa appena allocata

	}

	// in questo punto i = 4
	chk_elts[i] = (char *) malloc (sizeof (char) * ((256 * 3) + 1)); // se un utente si registra (MSG_REGLOG) il campo msg può essere lungo al massimo (256 * 3) + 1 caratteri
	if (chk_elts[i] == NULL)
		err_handler (E_MALLOC, 'x', 'w');
	bzero ((void *) chk_elts[i], (256 * 3) + 1); // azzero la stringa

	i = 0; // azzero l'indice della (nuova) stringa originale

	while (cp[i] != '\0' && j < 5) // tokenizzazione
	{

		if (k > 256 && j < 5) // se supero il massimo di caratteri possibili rischio errore di segmentazione quindi...
		{
			chk_elts[j][k] = '\0'; // per sicurezza
			return 3; // esci con errore di formato
		}
		else if (k > (256 * 3) && j == 4) // caso separato per l'ultima stringa, perchè se arriva messaggio REGLOG può contenere al massimo 256 * 3 caratteri
		{
			chk_elts[j][k] = '\0'; // per sicurezza
			return 3; // esci con errore di formato
		}

		if (j == 4) // l'ultima stringa va trattata come caso separato perchè vanno ignorati i ':' come delimitatore
		{
			chk_elts[j][k] = cp[i]; // salvataggio carattere in stringa temporanea
			k++; // avanzamento indice caraatteri stringa temp.
		}
		else
		{
			if (cp[i] != ':') // controllo assenza delimitatore
			{
				chk_elts[j][k] = cp[i]; // salvataggio carattere in stringa temporanea
				k++; // avanzamento indice caratteri stringa temp.
			}
			else // se viene letto un delimitatore
			{
				chk_elts[j][k] = '\0'; // aggiunta terminatore stringa
				j++; // avanzo a stringa temp. successiva
				k = 0; // azzero indice caratteri stringa temp
			}
		}

		i++; // avanzamento indice caratteri stringa da tokenizzare

	}

	free (cp);

	msg_t_local -> type = chk_elts[0][0]; // lettura tipo di comando
	switch (msg_t_local -> type) // controllo tipo messaggio
	{
		/*case 'A': // keep alive msg
		{
			return 65; // 65 = 'A'
			break;
		}*/
		case MSG_LOGIN: // tipi messaggio validi
		case MSG_REGLOG:
		case MSG_SINGLE:
		case MSG_BRDCAST:
		case MSG_LIST:
		case MSG_LOGOUT:
			break;
		default:
			return 2; // se il messaggio non è tra quelli elencati allora ritorna messaggio sconosciuto

	}

	msg_t_local -> sender = NULL; // il sender è sempre NULL nei messaggi da clent a server

	if (msg_t_local -> type == MSG_SINGLE) // se il messaggio è diretto ad un solo utente:
	{
		if (chk_elts[2][0] != 0) // ci deve essere il nome del ricevente
		{
			msg_t_local -> receiver = strdup (chk_elts[2]); // copia valore nella struttura
			if (msg_t_local -> receiver == NULL)
				err_handler (E_MALLOC, 'x', 'w');

		}
		else // altrimenti esci con errore di formato
			return 3;
	}
	else // se è dietto a tutti gli utenti non si deve specificare il ricevitore
		msg_t_local -> receiver = NULL;

	if ((msg_t_local -> type != MSG_LIST && msg_t_local -> type != MSG_LOGOUT) && (chk_elts[3][0] != 0)) // controllo perchè la sscanf fallisce se il buffer str è null
		sscanf (chk_elts[3], "%u", &msg_t_local -> msglen); // copia valore nella struttura
	else if ((msg_t_local -> type != MSG_LIST && msg_t_local -> type != MSG_LOGOUT) && (chk_elts[3][0] == 0)) // controllo perchè la sscanf fallisce se il buffer str è nul
		return 3;
	else if (msg_t_local -> type == MSG_LIST || msg_t_local -> type == MSG_LOGOUT)
		msg_t_local -> msglen = 0;

	if ((msg_t_local -> type != MSG_LIST && msg_t_local -> type != MSG_LOGOUT) && (chk_elts[4][0] != 0))
	{
		msg_t_local -> msg = strdup (chk_elts[4]); // copia valore nell struttura
		if (msg_t_local -> msg == NULL)
			err_handler (E_MALLOC, 'x', 'w');
	}
	else if ((msg_t_local -> type != MSG_LIST && msg_t_local -> type != MSG_LOGOUT) && (chk_elts[4][0] == 0))
		return 3;
	else if (msg_t_local -> type == MSG_LIST || msg_t_local -> type == MSG_LOGOUT)
		msg_t_local -> msg = NULL;

	if ((msg_t_local -> type != MSG_LIST && msg_t_local -> type != MSG_LOGOUT) && (msg_t_local -> msglen != (unsigned int) strlen (chk_elts[4]))) // controllo se msglen == strlen (msg)
		msg_t_local -> msglen = strlen (msg_t_local -> msg); // correzione automatica lunghezza


	for (i = 0; i < 5; i++)
		free (chk_elts[i]); // libero la memoria per strighe temporanee

	return 0; // ritorna ok

}
