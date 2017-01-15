/*
 * do_main.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * do_tmain.c
 *
 * Il file include solo il tread main eseguito dal server.
 *
 */

#include "main_includes.h"

/*
 * Funzione eseguita dal thread main.
 * Viene fatto il casting esplicito della struttura da void a struct thread_main_struct.
 * Vengono salvati i campi della struttura in variabili locali.
 * Se il processo riesce ad aprire il file degli utenti, lo legge, scorrendo riga per riga
 * e salvando gli utenti nella tabella hash.
 * Con un ciclo while viene letto tutto il file riga per riga. fgets legge da un file
 * pointer (fp) fino ad un massimo di caratteri - 1 definiti nel secondo argomento (sizeof...)
 * oppure quando raggiunge EOF, oppure ancora se c'è un carattere di newline (\n).
 * Se si presenta uno dei tre casi fgets ritorna NULL.
 * I caratteri letti vengono scritti nel buffer (tmp_f_l_buff).
 * All'interno del ciclo while viene creato un nuovo elemento di tipo hdata_t nel quale saranno copiate
 * le informazioni dell'utente.
 * Con la funzione str_token_read (definita in str_token.h) la stringa di partenza viene suddivisa nei
 * tre campi che descrivono le informazioni dell'utente e questi vengono salvati nella struttura "a".
 * Viene poi creato il valore di hash a partire dallo username che identifica univocamente ogni utente.
 * A questo punto, dopo il caricamento di tutti gli utenti dal file, viene aperto il file di log (log)
 * nel quale viene scritta l'intestazione contenente data e ora nel formato locale (grazie alla funzione
 * timestamp_crt definita in timestamp_crt.h).
 * Poi vengono inizializzate le strutture del thread dispatcher.
 * Vengono poi fatte le operazioni sul socket (vedi descrizione sotto).
 * Vengono inizializzati i mutex e viene settato l'attributo (variabile attr) detached per il thread worker.
 * All'interno del ciclo while la system call accept rimane in attesa di una connessione da parte di un client.
 * Al momento della connessione viene settata la variabile csd la quale viene passata come argomento al nuovo
 * thread worker appena creato. Se il thread viene creato si incrementa la variabile numThreadAttivi.
 * Viene infine riscritto il file degli utenti (compresi i nuovi utenti) scorrendo la lista di tipo users
 * con il puntatore tmp.
 */
void *do_tmain (void *thread_main_args) // procedura eseguita dal thread main
{

	int usr_fd; // log file descriptor, user file descriptor
	char *log_name, time_string[25]; // la stringa relativa all'ora è lunga 25 caratteri
	FILE *tmp_usr_fd; // file pointer (associato al file descriptor usr_fd) da usare per la fgets.
	ssize_t ret_io_bytes; // per read/write
	int ret_close, ret_str_token_read; // variabile che serve a controllare la corretta chiusura dei file descriptors

	int csd; // csd = client socket descriptor

	struct thread_main_struct *main_args; // nome locale

	char tmp_f_l_buf[771] = ""; // riga (temporanea) del file che andrà poi tokenizzata. 256*3 perchè ho 3 valori da max 256 + i 2 ':' e '\0'. significato variabile = temp file line buffer

	hash_t H; // hash locale
	hash_t *Hash; // puntatore di tipo hash_t
	hdata_t *search, *insert; // variabili per ricerca e inserimento nella tabella hash

	pthread_t tworker, tdispatcher; // thread id del thread worker e dispatcher
	pthread_attr_t attr; // attributi thread worker (che sarà detached)
	int ret_thread; // valore ritorno chiamata pthread_create

	users *head_do_tmain = NULL, *tmp; // testa lista utenti e puntatore temporaneo, tmp, per lo scorriemto lista

	char log_header_stars[54] = "****************************************************\n"; // serve per instestazione log file (53 chars + '\0')
	char log_header_timestamp[54]; // instestazione log file

	td_c_p *c; // dichiarazione puntatore a struttura di comunicazione tworker <-> tdispatcher
	c = &C; // assegnazione puntatore
	tw_log_c *l; // dichiarazione puntatore a struttura di gestione log file
	l = &L;

	char *usr_name, *usr_record; // usati per scrivere informazioni utenti sul file alla chiusura del programma
	size_t len_usr_record; // vedi riga sopra

	int ret, optvalue; // ret = variabile return generica per il socket, optvalue = per fare in modo che il socket possa essere riutilizzato immediatamente. Questa variabile sarà settata a 1.
	struct sockaddr_in sa; // struttura socket internet

	// -- Fine dichiarazioni --

	main_args = (struct thread_main_struct *) thread_main_args; // istanza locale della struttura con casting da void a struct thread_main_struct *
	usr_fd = main_args -> user_file; // riempimento nomi locali
	log_name = main_args -> log_file;
	usr_name = main_args -> usr_file;
	Hash = main_args -> hash;
      	H = *Hash; // riassegnazione

	if (usr_fd != -1) // se sono riuscito ad aprire il file utenti allora posso leggerne il contenuto, altrimento ignoro il problema
	{
		tmp_usr_fd = fdopen(usr_fd, "r"); // in questo modo ottengo un file pointer. fgets infatti non accetta file descriptors ma solo file pointers
		if (tmp_usr_fd == NULL)
			err_handler (E_USR_FILE_FDOPEN, 0, 0);
		else // se il file pointer è stato aperto...
		{
			//...inizializzazione tabella utenti
			while (fgets (tmp_f_l_buf, sizeof (tmp_f_l_buf), tmp_usr_fd) != NULL) // non sapendo quanto sarà grande la n-esima riga del file, uso fgets che legge fino al carattere newwline o fino a EOF ed aggiunge \0 alla fine della stringa
			{
				insert = (hdata_t *) malloc (sizeof (hdata_t)); // allocazione nuovo elemento per un singolo utente
				ret_str_token_read = str_token_read (tmp_f_l_buf, insert); // funzione string token
				if (ret_str_token_read == 0) // se non ci sono errori con la tokenizzazione...
				{
					insert -> sockid = -1; // non essendo ancora connesso si tratta di un socket dummy
					search = CERCAHASH (insert -> uname, H); // prima di inserire l'utente devo controllare se questo non esiste già nella tabella utenti
					if (search == NULL) // non c'è già un utente con lo stesso uname allora posso inserirlo
					{
						INSERISCIHASH (insert -> uname, insert, H); // inserimento utente nela tabella hash
						head_do_tmain = ADD_USR_TO_LIST (insert -> uname, head_do_tmain); // aggiunta chiave in lista globale utenti
					}
					else // se c'è già un utente con lo stesso nome segnala l'errore ma non uscire dal programma
						err_handler_argv (E_USR_ALREADY_EXISTS, insert -> uname, 0, 0);
				}
			}

			fclose (tmp_usr_fd); // chiudo file pointer
		}
	}

	head_signal = head_do_tmain; // assegno la testa della lista utenti locale alla variabile globale che sarà usata nella funzione di gestione dei segnali cioè head_signal

	log_fd = open (log_name, O_CREAT | O_WRONLY | O_TRUNC, 00600); //apri file di log in modalità rw per l'utente che ha lanciato il server ed in modalità troncamento
	if (log_fd == -1)
		err_handler_argv (E_LOG_FILE_OPEN, log_name, 'x', 0);

	timestamp_crt (time_string, 25); // creo il timestamp da mettere nell'intestazione del log
	ret_io_bytes = (int) snprintf (log_header_timestamp, 54, "** Chat Server started @ %s **\n", time_string);
	if ((size_t) ret_io_bytes != strlen (log_header_stars))
		err_handler (E_LOG_FILE_WRITE, 0, 0);
	ret_io_bytes = write (log_fd, log_header_stars, strlen (log_header_stars)); // intestazione log
	if (ret_io_bytes != (ssize_t) strlen (log_header_stars))
		err_handler (E_LOG_FILE_WRITE, 0, 0);
	ret_io_bytes = write (log_fd, log_header_timestamp, strlen (log_header_timestamp)); // stampa del timestamp
	if (ret_io_bytes != (ssize_t) strlen (log_header_timestamp))
		err_handler (E_LOG_FILE_WRITE, 0, 0);
	ret_io_bytes = write (log_fd, log_header_stars, strlen (log_header_stars));
	if (ret_io_bytes != (ssize_t) strlen (log_header_stars))
		err_handler (E_LOG_FILE_WRITE, 0, 0);

	// Da questo punto la scrittura sul log file è ottimistica, cioè anche se le write sopra falliscono il programma tenterà di scrivere eventuali errori sul file.

	// inizializzazione variabili struttura tdispatcher
	ret_thread = pthread_mutex_init (&c -> mux_tdispatcher, NULL);
	if (ret_thread != 0)
		err_handler (E_TDISPATCHER_OP, 'x', 'w');
	pthread_cond_init (&c -> empty, NULL);
	if (ret_thread != 0)
		err_handler (E_TDISPATCHER_OP, 'x', 'w');
	pthread_cond_init (&c -> full, NULL);
	if (ret_thread != 0)
		err_handler (E_TDISPATCHER_OP, 'x', 'w');

	c -> pos_r = 0; // inzializzazioni variabili buffer circolare delle richieste
	c -> pos_w = 0;
	c -> count = 0;

	thread_dispatcher.head_usrs_list = head_do_tmain;
	thread_dispatcher.td_mux_struct = c; // assegnazione struttura contenene semafori ed altri dati del buffer circolare

        ret_thread = pthread_create (&tdispatcher, NULL, do_tdispatcher, (void *) &thread_dispatcher); // creazione thread dispatcher
        if (ret_thread != 0) // controllo creazione/errori thread
                err_handler (E_TDISPATCHER_OP, 'x', 'w');

	/* Inizializzazione del master socket e di tutte le strutture necessarie al suo funzionamento.
	 * La funzione inizializza il master socket e ne ritorna l'identificatore (msd) in modo che possa essere chiuso successivamente.
	 * Vengono definiti dominio internet (quindi AF_INET), porta (1234 perchè facile da ricordare e non ha bisogno dei permessi di root),
	 * indirizzo ip della scheda di rete (riempito grazie alla macro INADDR_ANY), tutti decisi a priori.
	 * Questi sono usati dalla funzione bind in modo da rendere visibile il socket.
	 * Viene creata la coda di ascolto delle richieste con capienza massima possibile (grazie alla macro SOMAXCONN).
	 * Infine viene ritornato l'intero che rappresenta il socket descriptor (msd).
	 * Per ogni utilizzo di primitiva viene fatto il controllo di eventuali errori
	 */

	msd = socket (AF_INET, SOCK_STREAM, 0); //apro master socket di dominio AF_INET e tipo SOCK_STREAM per la connessione dei client. In questo modo la trasmissione dei dati è sicura e si portebbe fare fra nodi diversi
	if (msd < 0) // controllo errori creazione socket
		err_handler (E_SOCK, 'x', 'w');

	/* questo permette di riutilizzare il socket immediatamente dopo il riavvio del programma
	   SO_REUSEADDR = riutilizza l'indirizzo
	   optvalue è settato ad uno quindi rende vero SO_REUSEADDR
	   ultimo parametro è la lunghezza / dimensione di optvalue
	 */
	optvalue = 1;
	ret = setsockopt (msd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optvalue, (socklen_t) sizeof (int));
	if (ret == -1)
		err_handler (E_SETSOCKOPT, 'x', 'w');

	bzero (&sa, sizeof (struct sockaddr_in)); // azzera l'oggetto "sa"
	sa.sin_family = AF_INET; // dominio internet
	sa.sin_port = htons (MSD_PORT); // 1234 è facile da ricordare, vedi macro
	sa.sin_addr.s_addr = INADDR_ANY; // riempie automaticamente con il giusto indirizzo IP

	ret = bind (msd, (struct sockaddr *) &sa, sizeof (sa)); // lega socket all'indirizzo e alla porta (specificati sopra)
	if (ret == -1)
		err_handler (E_BIND, 'x', 'w');

	ret = listen (msd, SOMAXCONN); // creazione coda richieste fino al massimo possibile definita dalla macro
	if (ret == -1)
		err_handler (E_LISTEN, 'x', 'w');


	// inizializzazione variabili struttura tworker per log file e attributi tworker
	ret_thread = pthread_mutex_init (&l -> mux_log, NULL);
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');
	pthread_cond_init (&l -> cond_log, NULL);
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');
	ret_thread = pthread_attr_init (&attr);
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');

	l -> w_logfile = 0; // variabile usata per il mutex write log file

	ret_thread = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED); // crea un attributo attr che ha valore PTHREAD_CREATE_DETACHED da usare per i thread worker
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 'x', 'w');


	while (go)
	{
		csd = accept (msd, NULL, NULL); //non sapendo a priori chi sarà il peer, setto NULL e NULL, vedi $ man 2 accept
		if (errno != EINVAL && go != 0) // questo controllo va fatto per evitare errori all'uscita del programma. Infatti, in quel momento, viene invalidato msd, quindi l'accept si sblocca ritornando EINVAL. Il corpo dell'if non deve essere eseguito in quel caso
		{
			if (csd == -1)
				err_handler (E_ACCEPT, 'x', 'w');

			//argomenti da passare a do_tworker
			thread_worker.csd_socket = csd; // a do_worker vengono passati csd e Hash
			thread_worker.head_usrs_list = head_do_tmain;
			thread_worker.td_mux_struct = c; // viene passata la struttura c, che è del thread dispatcher, al thread worker
			thread_worker.tw_mux_struct = l; // l = log file

			ret_thread = pthread_create (&tworker, &attr, do_tworker, (void *) &thread_worker); // creazione thread worker in modalità detached
			if (ret_thread != 0) // controllo creazione/errori thread
				err_handler (E_TWORKER_OP, 'x', 'w');

			numThreadAttivi++; // incremento il numero di thead worker attivi (se la creazione è andata a buon fine)
		}
	}

	while (numThreadAttivi > 0); // attendi chiusura di tutti i thread worker

	// distruzione degli attributi, mutex, conditions dei thread
	ret_thread = pthread_attr_destroy (&attr); // distruzione dell'attributo detached
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 0, 'w'); // non è un errore fatale per cui il programma può continuare

	ret_thread = pthread_mutex_destroy (&l -> mux_log);
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 0, 'w');
	pthread_cond_destroy (&l -> cond_log);
	if (ret_thread != 0)
		err_handler (E_TWORKER_OP, 0, 'w');


	ret_thread = pthread_cancel (tdispatcher); // esce dal thread
        if (ret_thread != 0)
                err_handler (E_TDISPATCHER_OP, 'x', 'w');
 	ret_thread = pthread_join (tdispatcher, NULL); // attesa thread dispatcher
        if (ret_thread != 0) // controllo errori thread join
                err_handler (E_TDISPATCHER_OP, 'x', 'w');

        // prima di uscire scrive le chiavi sul file
	usr_fd = open (usr_name, O_CREAT | O_WRONLY | O_TRUNC, 00600); // apro il file utenti in modailtà scrittura (se esiste, oppure viene creato)
	if (usr_fd == -1)
		err_handler_argv (E_USR_FILE_OPEN, usr_name, 'x', 'w');

	tmp = head_do_tmain;
	while (tmp != NULL) // mentre c'è un utente...
	{
		search = CERCAHASH (tmp -> username, H); // ...se viene trovato l'utente nella tabella hash...
		if (search != NULL)
		{
			len_usr_record = strlen (search -> uname) + strlen (search -> fullname) + strlen (search -> email) + 4; // incluso il terminatore di stringa. i caratteri costanti sono 4

			usr_record = (char *) malloc (sizeof (char) * len_usr_record);
			if (usr_record == NULL)
				err_handler (E_MALLOC, 'x', 'w');

			bzero ((void *) usr_record, len_usr_record); // azzeramento

			snprintf (usr_record, len_usr_record, "%s:%s:%s\n", search -> uname, search -> fullname, search -> email); // salva il record nella variabile usr_record

			ret_io_bytes = write (usr_fd, usr_record, len_usr_record - 1); // il '- 1' serve a non stampare il carattere di fie stringa, che risulta '^@' in nano
			if (ret_io_bytes != (ssize_t) len_usr_record - 1)
				err_handler_argv (E_WRITE_USR_RECORD_FILE, search -> uname, 0, 'w');

			free (usr_record);
		}
		tmp = tmp -> next; // avanzo al prossimo elemento della lista
	}

	ret_close = close (log_fd); // chiusura log file
	if (ret_close == -1)
		err_handler (E_FILE_DESC, 0, 'w');

	pthread_exit (NULL); // chiusura sicura del thread

}
