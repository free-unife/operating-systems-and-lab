/*
 * chat-server.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * chat-server.c
 *
 * ChatFe server.
 * File sorgente iniziale del server contenente funzioni main,
 * sig_handler, do_server.
 *
 */

#include "main_includes.h"
#include "chat-server.h" // questo header è usato solo da questo file quindi non ha bisogno di essere all'interno di main_includes.h

int numThreadAttivi = 0; // inizializzazione variabile globale che rappresenta numero thread worker attivi
int go = 1;  // inizializzazione variabile globale go che serve per i cicli while.
int log_fd = -1; // variabile globale che rappresenta il file descriptor del log file. È settata a -1 così possono essere fatti controlli validità
int msd; // master socket descriptor
hash_t H; // tabella hash per gli utenti
users *head_signal = NULL; // testa della lista degli utenti, di tipo users

/*
 * Viene abilitato il riconosciemnto dei due segnali di interruzione
 * Viene controllato il numero di parametri passati da riga di comando
 * Se i parametri sono (almeno) tre viene fatto il fork del processo padre, il quale terminerà subito con successo.
 * Il processo figlio invece avvia il server chiamando la funzione do_server.
 * Nel caso il fork non andasse a buon fine oppure il numero di argomenti risultasse insufficiente allora viene gestito l'errore.
 */
int main (int argc, char **argv)
{

	pid_t pid; // variabile che conterrà il process id del processo padre/figlio. Per il figlio pid = 0

	signal (SIGINT, sig_handler); // abilitazione del riconoscimento dei due segnali di terminazione
	signal (SIGTERM, sig_handler);

	if (argc < 3) // controllo numero parametri
        {
                fprintf (stderr, "Uso: %s USER-FILE LOG-FILE\n", argv[0]); // stampa errore
                exit (EXIT_FAILURE); // esci senza successo
        }

	pid = fork(); // creazione del processo figlio

	if (pid == 0) // codice processo figlio
	{
		do_server (argv); // chiamata alla funzione server daemon
		exit (EXIT_SUCCESS); // terminazione processo figlio
	}
	else if (pid < 0)
		err_handler (E_FORK_FAILED, 'x', 0); // gestione errore processo figlio

	exit (EXIT_SUCCESS); // terminazione processo padre

}

/*
 * Funzione di gestione dei segnali per la chiusura in sicurezza del server.
 * Se viene mandato un segnale e questo è SIGINT o SIGTERM allora la variabile go viene settata a 0
 * in modo che i thread escano con sicurezza dai loop.
 * Viene chiuso il master socket descriptor poi vengono disconnessi tutti i clienti collegati
 * attraverso una ricerca nella lista utenti e nella tabella hash.
 */
void sig_handler (int sig)
{

	if (sig == SIGINT || sig == SIGTERM) // se viene mandato uno dei due segnali...
	{

		hdata_t *search_disconnect; // variabile di tipo hash per fare la ricerca, vedi sotto
		users *tmp; // testa temporanea, vedi sotto


		fprintf (stderr, "SIGINT o SIGTERM ricevuto. Chiusura server...\n");
		go = 0; // in questo modo tutti i cicli possono terminare in sicurezza
		shutdown (msd, SHUT_RDWR); // chiusura forzata msd

		// disconnessione client
		tmp = head_signal; // assegnazione di una testa temporanea, tmp, da usare per l'accesso alla lista
		while (tmp != NULL) // finchè la lista utenti non è vuota
		{
			search_disconnect = CERCAHASH (tmp -> username, H); // accesso alla tabella
	                if (search_disconnect != NULL) // se viene trovato un client...
        	        {
                        	if (search_disconnect -> sockid != -1) // ...disconnetti tutti il client collegato
                                	shutdown (search_disconnect -> sockid, SHUT_RDWR); // chiusura socket del client.
			}

                	tmp = tmp -> next; // avanzamento del prossimo elemento della lista
		}
	}
}

/*
 * Funzione server daemon.
 * Viene creato il thread main (tmain) se e solo se il file degli utenti (argv[1]) può essere aperto in modalità rw.
 * Questo viene fatto con la funzione fopen.
 * Poi vengono riempiti i campi dell' oggetto thread_main, che è una struttura di tipo struct thread_main_struct.
 * Questi campi vengono passati come argomento di pthread_create al thread main.
 * Viene poi creato il thread vero e proprio con la primitiva pthread_create la quale chiama la funzione do_tmain (cioè do thread main)
 * Successivamente è controllato il valore di ritorno di pthread_create per gestire eventuali errori.
 * Infine, quando il programma termina, il processo principale attende il thread_main con una pthread_join.
 */
void do_server (char **argv)
{

	pthread_t tmain; // ID del thread main
	int ret_thread; // valore di ritorno delle funzioni riferite ai thread
	char *logfile = argv[2]; // assegnazione nome del log-file, come copia del valore del puntatore di argv[2]
	int usr_fd; // file descriptor file utenti
        hash_t *Hash; // puntatore di tipo hash_t

	usr_fd = open (argv[1], O_RDWR); //apre file contenente la tabella degli utenti in modalità lettura e scrittura
	if (usr_fd == -1) // se il file descriptor = 1...
		err_handler_argv (E_USR_FILE_OPEN, argv[1], 0, 0); // ...gestisci errore avvertendo solo.

	H = CREAHASH (); // init hash globale. non si può inizializzare l'hash a livello globale a causa di una limitazione del C: error: initializer element is not constant
	Hash = &H; // così si può passare H al thread main

	thread_main.log_file = logfile; // riempimento dati istanza/oggetto thread_main
	thread_main.user_file = usr_fd;
	thread_main.usr_file = argv[1];
	thread_main.hash = Hash;

	ret_thread = pthread_create (&tmain, NULL, do_tmain, (void *) &thread_main); // creazione del thread main
	if (ret_thread != 0) // controllo errori thread create
		err_handler (E_TMAIN_OP, 'x', 0);

	ret_thread = pthread_join (tmain, NULL);
	if (ret_thread != 0) // controllo errori thread join
		err_handler (E_TMAIN_OP, 'x', 0);
}

