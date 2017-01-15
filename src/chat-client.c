/*
 * chat-client.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * chat-client.c
 *
 * Le parti commentate si riferiscono al prompt per rendere l'output più
 * chiaro. Eliminato perchè molto difficile da gestire.
 *
 */

#include "main_includes.h"
#include "chat-client.h"

int log_fd = -1; // variabile non utilizzata ma necessaria al funzionamento
int go = 1; // utilizzata per i cicli while nei thread
int csd; // csd globale
/*pthread_mutex_t mux_prompt; // variabili da usare per avere una stampa "decente" con un prompt
pthread_cond_t prompt;
int prompt_locked = 1;*/

/*
 * La funzione inizialmente controlla il numero di parametri passati da riga di comando.
 * Se i parametri sono almeno tre il programma fa il login oppure reglog, altrimenti stampa il
 * messaggio di aiuto. Viene aperto un socket, csd, poi viene effettuato il collegamento al server con
 * la system call connect su porta e indirizzo definiti rispettivamente dalle macro PORT e ADDRESS.
 * Vengono fatti 10 tentativi di collegamento poi il client esce.
 * Se tutto va a buon fine viene lanciato il thread sender (tsender) e il thread receiver (treceiver),
 * ai quali vengono passate le rispettive strutture (thread_sender, thread_receiver) contenti gli argomenti
 * necessari al loro funzionamento.
 */
int main (int argc, char **argv)
{

	int tries = 0, ret, optvalue; // tries = # prove connessione, optvalue valore settato ad 1 per SO_REUSEADDR
	struct sockaddr_in conn; // struttura contenente AF_INET, ADDRESS, PORT
	char *msg_help, *msg_log; // messaggio di aiuto, messaggio di login o reglog
	size_t len_msg_log, len_part_msg_log; // lunghezza di tutta o parte stringa log, reglog
	int i = 0, space_pos, space_counter = 0; // i = iteratore generico, space_* usate dal messaggio di reglog
	char conn_err[12]; //connection error
	char *username; // contiene nome utente del client
	msg_t msg_t_main; // Dichiaro istanza struttura
	msg_t *msg_t_pointer; //Dicharo puntatore di tipo msg_t
	msg_t_pointer = &msg_t_main; // In questo modo ottengo l'indirizzo di memoria della struttura msg_t_l.
	pthread_t tsender, treceiver; // id dei due thread


	if (argc < 2 || ((strcmp (argv[1], "-r") == 0) && argc < 4)) // controllo numero parametri
        {
                fprintf (stderr, "Uso: %s [-h] [-r \"Name Surname email\"] username\n", argv[0]); // stampa breve messaggio aiuto
                exit (EXIT_FAILURE);
        }

	if (strcmp (argv[1], "-h") == 0) // spiega funzionamento ed esci immediatamente
	{
		msg_help =
			"ChatFe client\n"
			"=============\n"
			"\tDESCRIZIONE\n"
			"\t\tClient per collegarsi ad un server ChatFe.\n"
			"\tOPZIONI\n"
			"\t\t-r\n"
			"\t\t\tRegistra un nuovo utente e connessione al server.\n"
			"\t\t\tSintassi: %s -r \"Name Surname Email\" username\n"
			"\t\t-h\n"
			"\t\t\tStampa questo messaggio di aiuto\n\n"
			"\tCOMANDI\n"
			"\t\t#dest receiver:text\\n\n"
			"\t\t\tInvia un messaggio `text` al destinatario `receiver`.\n"
			"\t\t#dest:text\\n\n"
			"\t\t\tInvia un messaggio `text` in broadcast.\n"
			"\t\t#ls\\n\n"
			"\t\t\tVisualizza la lista degli utenti collegati.\n"
			"\t\t#logout\\n\n"
			"\t\t\tDisconnessione dal server e uscita dal client.\n"
			"\n\t\tN.B. Tutti i comandi sono preceduti dal carattere `#` che non potrà essere usato come parte del messaggio.\n\n"
			"\tVISUALIZZAZIONE MESSAGGI\n"
			"\t\tOK\n"
			"\t\t\tMessaggio di ok.\n"
			"\t\tERR\n"
			"\t\t\tMessaggio di errore.\n"
			"\t\tLista utenti collegati\n"
			"\t\t\tVisualizza gli utenti collegati in quel momento al server\n"
			"\t\tsender:receiver:message\n"
			"\t\t\tVisualizzazione messaggio `msg` inviato dall'utente `sender` e ricevuto da `receiver`.\n"
			"\t\tsender:*:message\n"
			"\t\t\tVisualizzazione messaggio `msg` inviato dall'utente `sender` e ricevuto da tutti (messaggio broadcast).\n"
			;

		fprintf (stdout, msg_help, argv[0]);
		exit (EXIT_SUCCESS);
	}

	csd = socket (AF_INET, SOCK_STREAM, 0); // creazione socket
	if (csd == -1)
		err_handler (E_SOCK, 'x', 0);

	/* SO_REUSEADDR riutilizza l'indirizzo, cioè il socket immediatamente dopo il riavvio del programma
           optvalue è settato ad uno quindi rende vero SO_REUSEADDR
           ultimo parametro è la lunghezza / dimensione di optvalue
         */
        optvalue = 1;
        ret = setsockopt (csd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optvalue, (socklen_t) sizeof (int));
        if (ret == -1)
                err_handler (E_SETSOCKOPT, 'x', 'w');

	// inizializzazione struttura per il socket
	bzero (&conn, sizeof (struct sockaddr_in)); // azzera l'oggetto addr
	conn.sin_family = AF_INET;
	conn.sin_port = htons (PORT);
	conn.sin_addr.s_addr = inet_addr (ADDRESS);

	// connessione al server
	while (connect (csd, (struct sockaddr *) &conn, (socklen_t) sizeof (conn)) == -1 && tries < 10) // Connect (and reiterate max 10 times, 1 sec distance) to srv
	{
		sprintf (conn_err, "%s %d", "tentativo", tries + 1);
		err_handler_argv (E_CONNECT, conn_err, 0, 0);
		sleep (1); // attendi 1 sec tra i tentativi
		tries++;
	}
	if (tries >= 10) // esci se il numero di tentativi falliti è (maggiore) uguale a 10
		exit (EXIT_FAILURE);

	// se arriva qui il collegamento è accettato
	if (argc == 2) // fai il login
	{
		len_msg_log = (size_t) snprintf (0, 0, "%d", (int) strlen (argv[1])); // ritorna il numero di caratteri di strlen (argv[1])
		len_msg_log += strlen (argv[1]) + 2; // 1 è ':' e l'altro 1 è '\0'
		msg_log = (char *) malloc (sizeof (char) * len_msg_log);
		if (msg_log == NULL)
			err_handler (E_MALLOC, 'x', 0);
		bzero ((void *) msg_log, len_msg_log);

		snprintf (msg_log, len_msg_log, "%d:%s", (int) strlen (argv[1]) + 1, argv[1]);
		write_sock (WR_SOCK_LOGIN, msg_log, csd); // invio msg di login

		username = strdup (argv[1]); // copia stringa username in username
		if (username == NULL)
			err_handler (E_MALLOC, 'x', 0);

		free (msg_log);

	}
	else if (argc == 4) // reglog
	{
		/* Funzionamento registrazione e login
		 * ===================================
		 *
		 * ./chat-client	-r	"Name Surname email"	username
		 * argv[0]		argv[1]	argv[2]			argv[3]
		 *
		 * nomi variabili fittizi:
		 * msg_log = "username:Name Surname:email"
		 * msg_to_send = "strlen(msg):username:Name Surname:email"
		 * msg_final = ":R:::strlen(msg):username:Name Surname:email"
		 */

		len_msg_log = strlen (argv[3]) + strlen (argv[2]) + 3 + 1 ; // 3 = ':' e ' ', 1 = '\0'
		len_msg_log += 1; // i ':' prima del msg
		len_part_msg_log = len_msg_log; // lunghezza esclusa la strlen (msg_log);
		len_msg_log += (size_t) snprintf (0, 0, "%d", (int) len_msg_log); // numero di caratteri di len_msg_log

		msg_log = (char *) malloc (sizeof (char) * len_msg_log);
		if (msg_log == NULL)
			err_handler (E_MALLOC, 'x', 0);
		bzero ((void *) msg_log, len_msg_log);

		i = 0;
		space_counter = 0;
		while (argv[2][i] != '\0') // modifica di argv[2] per poterlo passare al server
		{
			if (argv[2][i] == ' ') // ricerca spazi
			{
				space_pos = i; // salvataggio indice spazio
				space_counter++; // conta gli pazi presenti nella stringa
			}
			i++;
		}
		if (space_counter == 2) // se argv[2] ha 2 spazi...
			argv[2][space_pos] = ':'; // ...sostituisco ':' al posto dell'ultimo ' ' in modo che aderisca al formato riconosciuto dal server
		else // altrimenti...
			err_handler (E_INVALID_FORMAT, 0, 0); // formato non valido

		snprintf (msg_log, len_msg_log, "%d:%s:%s", (int) len_part_msg_log, argv[3], argv[2]);
		write_sock (WR_SOCK_REGLOG, msg_log, csd); // invio reglog al server
		username = strdup (argv[3]); // copia nome utente in username
		if (username == NULL)
			err_handler (E_MALLOC, 'x', 0);

		free (msg_log);
	}

	// riempimento valori da passare al thread sender
	thread_sender.uname = username;
	// riempimento valori da passare al thread receiver
	thread_receiver.msg_t_struct = msg_t_pointer;
	thread_receiver.uname = username;

	ret = pthread_create (&tsender, NULL, do_tsender, (void *) &thread_sender); // creazione del thread sender
	if (ret != 0) // controllo creazione/errori thread
		err_handler (E_TSENDER_OP, 'x', 0);

	ret = pthread_create (&treceiver, NULL, do_treceiver, (void *) &thread_receiver); // creazione del thread receiver
	if (ret != 0) // controllo creazione/errori thread
		err_handler (E_TRECEIVER_OP, 'x', 0);

	ret = pthread_join (tsender, NULL);
	if (ret != 0) // controllo errori thread join
		err_handler (E_TSENDER_OP, 'x', 0);

	ret = pthread_join (treceiver, NULL);
	if (ret != 0) // controllo errori thread join
		err_handler (E_TRECEIVER_OP, 'x', 0);

	exit (EXIT_SUCCESS); // terminazione processo principale

}

/*
 * Funzione invio messaggi al server eseguita dal thread sender.
 * Viene passato lo username, poi la funzione entra nel ciclo while(go) dove viene chiamata la funzione fgets
 * per la lettura dei messaggi da stdin. Prima gi chiamare la funzione parse_and_send_msg, viene fatto un controllo
 * del formato. Se questo non è valido, viene stampato il rispettivo errore, poi il ciclo ricomincia. Altrimenti
 * viene passato il messaggio msg alla funzione parse_and_send_msg.
 */
void *do_tsender (void *thread_sender_args)
{

	size_t len_msg = (256 * 4) + 5 + (256 * 3) + 2 + 1; // primi 4 campi + 5 ':' + campo msg + eventuali 2 ':' + '\0'. Tuttavia non verranno usati tutti.
	char msg[len_msg], *ret_fgets = NULL, *username_do_tsender; // msg = messaggio, ret_fgets ritorna il puntatore della stringa passata come argomento se tutto è OK, NULL altrimenti
	int ret_parse; // ritorno funzione di parsing e invio
	struct thread_sender_struct *sender_args; // nome locale della struttura


	sender_args = (struct thread_sender_struct *) thread_sender_args; // istanza locale della struttura con casting da void a struct
        username_do_tsender = sender_args -> uname; // riempimento nomi locali
	(void) username_do_tsender; // per evitare waring di variabile non utilizzata.

	while (go)
	{

		bzero ((void *) msg, len_msg); // azzeramento stringa usata da fgets. Poichè è stata dichiarata sullo stack viene riciclata e deve essere azzerata ogni volta

		/*ret_thrd_op = pthread_mutex_lock (&mux_prompt); // mutua esclusione per accedere alla tabela hash e per le altre operazioni
		if (ret_thrd_op != 0)
                       	err_handler (E_TSENDER_OP, 'x', 0);

		while (prompt_locked == 1)
		{
			ret_thrd_op = pthread_cond_wait (&prompt, &mux_prompt); // ...quindi attende una signal sul mux
			if (ret_thrd_op != 0)
				err_handler (E_TSENDER_OP, 'x', 0);
		}

		fprintf (stdout, "%s> ", username_do_tsender); // stampa stringa prompt (con username)*/

		ret_fgets = fgets (msg, sizeof (msg), stdin); // salva messaggio inserito in msg
		if (ret_fgets != NULL) // se stdin è ancora aperto...
		{
			if (msg[0] == '\n')  // ...se l'utente ha premuto invio, non fare niente
				ret_parse = 1; // 1 non è tra gli errori
			else if (msg[0] != '#') // ...tutti i comandi incominciano con '#'
			{
				ret_parse = 3; // formato non valido
				err_handler (E_INVALID_FORMAT, 0, 0);
			}
			else
			{
				ret_parse = parse_and_send_msg (msg); //, username_do_tsender); // parsing e invio msg al server
				switch (ret_parse)
				{
					case 2: // il server ritorna comando sconosciuto
					{
						err_handler (E_UNKNOWN_CMD, 0, 0);
						break;
					}
					case 3: // il server ritorna formato non valido
					{
						err_handler (E_INVALID_FORMAT, 0, 0);
						break;
					}
				}
			}
		}
		else // stdin è stato chiuso
		{
			go = 0; // setta la variabile dei cicli a 0.
			fprintf (stdout, "\n"); // vai a capo dopo disconnessione
		}
		// il messaggio keep alive non fa parte del protocollo di comunicazione, ma è necessario al funzionamento:, A = Alive. Non va fatto se il comando va a buon fine (ret_parse = 0)
		/*if (ret_parse != 0)
			write_sock (NULL, ":A::::", csd); // messagio di keep alive usato per non bloccarsi sulla condition sopra, il server risponde con la stessa stringa

		prompt_locked = 1;

                ret_thrd_op = pthread_cond_signal (&prompt);
                if (ret_thrd_op != 0)
                        err_handler (E_TSENDER_OP, 'x', 0);

                ret_thrd_op = pthread_mutex_unlock (&mux_prompt);
                if (ret_thrd_op != 0)
                        err_handler (E_TSENDER_OP, 'x', 0);*/


	}

	pthread_exit (NULL); // uscita thread
}

/*
 * Funzione di invio e parsing messaggi.
 * Alla funzione viene passato il messaggio (msg_local) da elaborare.
 * Per prima cosa viene fatto un controllo più approfondito sul formato con
 * un ciclio while, usando i come iteratore. Il ciclo estrae il tipo di
 * messaggio oppure ritorna 3 se il formato non è valido. Per i messaggi
 * come "#ls" o "#logout" viene semplicemente mandata la stringa corrispondente
 * al server. Per "#dest" invece la funzione determina se si tratta di
 * MSG_SINGLE oppure MSG_BRDCAST. MSG_BRDCAST ha il sesto carattere == ':'.
 * Dopo aver distinto i due messaggi, se si tratta di MSG_BRDCAST, viene salvato
 * il contenuto del messaggio nella variabile content, poi vengono concatenati i
 * dati di formato e il tutto viene salvato in str_to_send. Se invece si tratta
 * di MSG_SINGLE viene prima estratto il nome del destinatario, con un cliclo while,
 * e salvato in dest. Poi vengono fatte le stesse operazioni che nel caso di
 * MSG_BRDCAST. Infine viene mandato il messaggio con la funzione write_sock.
 *
 * Struttura comandi
 * =================
 *
 * #ls\0x10\0x0
 * #logout\0x10\0x0
 * #dest\0x32:text\0x10\0x0
 * #dest\0x32receiver:text\0x10\0x0
 */
int parse_and_send_msg (char *msg_local)
{

	char msg_type[8], *dest, *content, *str_to_send; // msg_type è lungo 8 perchè 7 + '\0'
	int i, j; // vari iteratori
	size_t len_str_to_send;


	bzero ((void *) msg_type, 8); // così ho già '\0' alla fine della stringa
	i = 0;
	while (msg_local[i] != ' ' && msg_local[i] != '\n' && msg_local[i] != '\0')
	{
		if (i > 6) // se ho più di 7 caratteri... (N.B. i parte da 0 non da 1)
			return 3; // ritorna formato non valido
		msg_type[i] = msg_local[i]; // salva il tipo di messaggio
		i++;
	}

	if (strcmp (msg_type, "#ls") == 0) // list users
		write_sock (WR_SOCK_LIST, NULL, csd);
	else if (strcmp (msg_type, "#logout") == 0)
		write_sock (WR_SOCK_LOGOUT, NULL, csd); // soft logout, cioè il server provoca la chiusura del client, non è il client a killarsi qui
	else if (strcmp (msg_type, "#dest") == 0) // MSG_SINGLE or MSG_BRDCAST
	{
		i++; // così ignoriamo lo spazio
		// confronto carattere successivo allo spazio
		if (msg_local[i] == ':') // MSG_BRDCAST
		{
			content = (char *) malloc (sizeof (char) * (strlen (msg_local) - i + 1));
			if (content == NULL)
				err_handler (E_MALLOC, 'x', 0);
			bzero ((void *) content, strlen (msg_local) - i + 1);

			j = 0;
			i++; // così salta il primo ':'
			while (msg_local[i] != '\0') // ricava contenuto messaggio
			{
				if (msg_local[i] == '#') // il contenuto non può contenere questo carattere
					return 3; // invalid format
				content[j] = msg_local[i];
				j++;
				i++;
			}

			len_str_to_send = (size_t) snprintf (0, 0, "%d", (int) strlen (content)); // ricavo numero caratteri di strlen(content)
			len_str_to_send += strlen (content) + 6 + 1; // 1 è '\0', 6 sono i ':' e la 'B'

			str_to_send = (char *) malloc (sizeof (char) * len_str_to_send);
			if (str_to_send == NULL)
				err_handler (E_MALLOC, 'x', 0);

			bzero ((void *) str_to_send, len_str_to_send);

			snprintf (str_to_send, len_str_to_send, ":B:::%d:%s", (int) strlen (content), content); // siccome la stringa finisce sempre con '\n', che non viene contato, non si aggiunge '+1' nel conteggio. Infatti bisogna considerare anche '\0'

			free (content);
		}
		else // MSG_SINGLE
		{
			dest = (char *) malloc (sizeof (char) * (strlen (msg_local) - i + 1));
			if (dest == NULL)
				err_handler (E_MALLOC, 'x', 0);

			bzero ((void *) dest, strlen (msg_local) - i + 1);

			content = (char *) malloc (sizeof (char) * (strlen (msg_local) - i + 1));
			if (content == NULL)
				err_handler (E_MALLOC, 'x', 0);

			bzero ((void *) content, strlen (msg_local) - i + 1);

			j = 0;
			while (msg_local[i] != '\0' && msg_local[i] != ':') // ricava username destinatario (che si trova prima del ':')
			{
				dest[j] = msg_local[i];
				j++;
				i++;
			}

			// vedi caso MSG_BRDCAST da questo punto
			j = 0;
			i++; // così salta il primo ':'
			while (msg_local[i] != '\0') // ricava contenuto messaggio
			{
				if (msg_local[i] == '#') // il contenuto non può contenere questo carattere
					return 3;
				content[j] = msg_local[i];
				j++;
				i++;
			}

			len_str_to_send = snprintf (0, 0, "%d", (int) strlen (content));
			len_str_to_send += strlen (content) + strlen (dest) + 6 + 1; // 1 è '\0', 6 sono i ':' e la 'S'

			str_to_send = (char *) malloc (sizeof (char) * len_str_to_send);
			if (str_to_send == NULL)
				err_handler (E_MALLOC, 'x', 0);

			bzero ((void *) str_to_send, len_str_to_send);

			snprintf (str_to_send, len_str_to_send, ":S::%s:%d:%s", dest, (int) strlen (content), content); // siccome la stringa finisce sempre con '\n', che non viene contato, non si aggiunge '+1' nel conteggio. Infatti bisogna considerare anche '\0'

			free (dest);
			free (content);
		}

		write_sock (NULL, str_to_send, csd); // write msg
		free (str_to_send);


	}
	else
		return 2; // unkown cmd

	return 0;

}

/*
 * Funzione ricezione messaggi dal server eseguita dal thread receiver.
 * Alla funzione viene passata un'istanza (msg_t_struct) della struttura definita in comm_prot.h
 * che viene salvata in msg_t_do_treceiver.
 * All'inizio del ciclo while(go) viene chiamata la funzione condvisa get_and_parse_cmd,
 * che legge i messaggi in arrivo dal server, alla quale viene passata la struttura msg_t_do_treceiver
 * e il socket csd.
 *
 * Valori ritorno get_and_parse_cmd
 * ================================
 *
 * 0 = OK (È uno dei messaggi validi.)
 * 1 = Errore fatale o socket chiuso (il client deve essere terminato, settando go=0 e chiudendo stdin.)
 * 2 = Comando dal server sconosciuto.
 * 3 = Formato messaggio dal server non valido.
 *
 * Se il valore di ritorno è 0, viene controllato il tipo di messaggio (switch (msg_do_treceiver -> type))
 * e in base a questo fatta l'opportuna stampa. Altimenti viene fatta la stampa dell'errore/disconnessione.
 */
void *do_treceiver (void *thread_receiver_args)
{

	struct thread_receiver_struct *receiver_args; // nome locale
        int ret_parse; // valore ritorno funzione get_and_parse_cmd
	msg_t *msg_t_do_treceiver; // puntatore locale alla struttura di tipo msg_t
	char *username_do_treceiver; // username client locale


	receiver_args = (struct thread_receiver_struct *) thread_receiver_args; // istanza locale della struttura con casting da void a struct
        msg_t_do_treceiver = receiver_args -> msg_t_struct; // riempimento variabili locali
	username_do_treceiver = receiver_args -> uname;

	while (go)
	{

		ret_parse = get_and_parse_cmd (msg_t_do_treceiver, csd); // chiamata alla funzione condivisa

		/*if (msg_t_do_treceiver -> type == MSG_SINGLE || msg_t_do_treceiver -> type == MSG_BRDCAST) // tipo msg che server invia al client
		{
			prompt_locked = 1; // Poichè la ricezione di messaggi è impevedibile, va gestita come caso particolare
	                ret_thrd_op = pthread_cond_signal (&prompt);
        	        if (ret_thrd_op != 0)
                	        err_handler (E_TSENDER_OP, 'x', 0);

	                ret_thrd_op = pthread_mutex_unlock (&mux_prompt);
        	        if (ret_thrd_op != 0)
                	        err_handler (E_TSENDER_OP, 'x', 0);
		}

                ret_thrd_op = pthread_mutex_lock (&mux_prompt); // mutua esclusione per accedere alla tabela hash e per le altre operazioni
                if (ret_thrd_op != 0)
                        err_handler (E_TRECEIVER_OP, 'x', 0);

		while (prompt_locked == 0)
		{
			ret_thrd_op = pthread_cond_wait (&prompt, &mux_prompt); // ...quindi attende una signal sul mux
			if (ret_thrd_op != 0)
				err_handler (E_TRECEIVER_OP, 'x', 0);
		}*/

		switch (ret_parse)
		{
			case 0: // OK
			{
				switch (msg_t_do_treceiver -> type) // tipo msg che server invia al client
				{
					case MSG_OK:
					{
						fprintf (stdout, "OK\n");
						break;
					}
					case MSG_ERROR:
					{
						fprintf (stderr, "ERR: %s\n", msg_t_do_treceiver -> msg);
						if (strcmp (E_USR_NOT_REG, msg_t_do_treceiver -> msg) == 0
						|| strcmp (E_USR_ALREADY_REG, msg_t_do_treceiver -> msg) == 0
						|| strcmp (E_USR_ALREADY_LOGGED, msg_t_do_treceiver -> msg) == 0)  // controllo dei casi paricolari dove va stampato...
							fprintf (stdout, "Premi invio per uscire\n");
						break;
					}
					case MSG_LIST:
					{
						fprintf (stdout, "Lista utenti collegati\n\n"
								"%s\n", msg_t_do_treceiver -> msg);
						break;
					}
					case MSG_SINGLE:
					{
						fprintf (stdout, "%s:%s:%s\n", msg_t_do_treceiver -> sender, username_do_treceiver, msg_t_do_treceiver -> msg);
						break;
					}
					case MSG_BRDCAST:
					{
						fprintf (stdout, "%s:*:%s\n", msg_t_do_treceiver -> sender, msg_t_do_treceiver -> msg);
						break;
					}
					/*case 'A': // keep alive
						break;*/
				}

				break;

			}
			case 1: // fatal err or closed socket
			{
				go = 0;
				fprintf (stderr, "Connessione rifiutata o disconnessione\n");
				fprintf (stdout, "Premi invio per uscire\n");
				fclose (stdin);
				break;
			}
			case 2:
			{
				err_handler (E_UNKNOWN_CMD, 0, 0);
				break;
			}
			case 3:
			{
				err_handler (E_INVALID_FORMAT, 0, 0);
				break;
			}

		}

		fprintf (stdout, "\n");

		/*prompt_locked = 0;

                ret_thrd_op = pthread_cond_signal (&prompt);
                if (ret_thrd_op != 0)
                        err_handler (E_TRECEIVER_OP, 'x', 0);

                ret_thrd_op = pthread_mutex_unlock (&mux_prompt);
                if (ret_thrd_op != 0)
                        err_handler (E_TRECEIVER_OP, 'x', 0);*/

	}

	pthread_exit (NULL);
}

/*
 * Funzione di demarshalling client.
 * Questa funzione si occupa di scomporre il messaggio appena letto dal socket.
 * Vengono allocate 5 stringhe da 256 caratteri ognuno, con il puntatore alle 5 stringhe chk_elts.
 * Viene poi fatta la tokenizzazione con un clclo while, usando come delimitatore il carattere ':'.
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
 * //65 = è stato ricevuto un messaggio di acknowledgment da re-inviare al server
 */
	/*	Legenda comm_prot client to server
	 *	==================================
	 *
	 *	type	=	{'O', 'E', 'I', 'S', 'B'}
	 *
	 *	MSG_OK
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	0
	 *		msg		=	NULL
	 *
	 *	MSG_ERROR
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	strlen (error)
	 *		msg		=	error
	 *
	 *	MSG_LIST
	 *		sender		=	NULL
	 *		receiver	=	NULL
	 *		msglen		=	strlen (user_list)
	 *		msg		=	user_list
	 *
	 *	MSG_SINGLE
	 *		sender		=	user_name
	 *		receiver	=	NULL
	 *		msglen		=	strlen (msg)
	 *		msg		=	message
	 *
	 *	MSG_BRDCAST
 	 *		sender		=	user_name
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
		chk_elts[i] = (char *) malloc (sizeof (char) * 257); // per semplicità alloco 257 (256 + '\0') caratteri per ogni array temporaneo. In questo modo tutti i parametri hanno una lunghezza fissata ma ottengo performances miglioririspetto ad una realloc carattere per carattere (che inoltre può provocare errori di segmentazione)
		if (chk_elts[i] == NULL)
			err_handler (E_MALLOC, 'x', 'w');
		bzero ((void *) chk_elts[i], 257); // azzero la stringa

	}

	// in questo punto i = 4
	chk_elts[i] = (char *) malloc (sizeof (char) * ((256 * 3) + 1)); // se un utente si registra (MSG_REGLOG) il campo msg può essere lungo al massimo (256 * 3) + '\0'
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
		else if (k > (256 * 3) && j == 4) // caso separato per l'ultima stringa
		{
			chk_elts[j][k] = '\0'; // per sicurezza
			return 3; // esci con errore di formato
		}

		if (j == 4) // l'ultima stringa va trattata come caso separato perchè vanno ignorati i ':' come delimitatore
		{
			chk_elts[j][k] = cp[i]; // salvataggio carattere in stringa temporanea
			k++; // avanzamento indice caratteri stringa temp.
		}
		else
		{
			if (cp[i] != ':') // controllo assenza delimitatore
			{
				chk_elts[j][k] = cp[i]; // salvataggio carattere in stringa temporanea
				k++; // avanzamento indice caraatteri stringa temp.
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
		/*case 'A': // keep alive
		{
			return 0; // ritorna subito
			break;
		}*/
		case MSG_OK:
		case MSG_ERROR:
		case MSG_LIST:
		case MSG_SINGLE:
		case MSG_BRDCAST:
			break;
		default:
			return 2; // se il messaggio non è tra quelli elencati allora ritorna messaggio sconosciuto

	}

	msg_t_local -> receiver = NULL; // il receiver è sempre NULL nei messaggi da server a client

	if (msg_t_local -> type == MSG_SINGLE || msg_t_local -> type == MSG_BRDCAST)
	{
		if (chk_elts[1][0] != 0) // ci deve essere il nome del mittente
		{
			msg_t_local -> sender = strdup (chk_elts[1]); // valore riempito solo se valido
			if (msg_t_local -> sender == NULL)
				err_handler (E_MALLOC, 'x', 'w');
		}
		else // altrimenti esci con errore di formato
			return 3;
	}
	else
		msg_t_local -> sender = NULL;

	if ((msg_t_local -> type != MSG_OK && msg_t_local -> type != MSG_ERROR) && (chk_elts[3][0] != 0)) // controllo perchè la sscanf fallisce se il buffer str è null
		sscanf (chk_elts[3], "%u", &msg_t_local -> msglen); // valore riempito solo se valido
	else if ((msg_t_local -> type != MSG_OK && msg_t_local -> type != MSG_ERROR) && (chk_elts[3][0] == 0)) // controllo perchè la sscanf fallisce se il buffer str è nul
		return 3;
	else if (msg_t_local -> type == MSG_OK || msg_t_local -> type == MSG_ERROR)
		msg_t_local -> msglen = 0;

	if (msg_t_local -> type != MSG_OK && chk_elts[4][0] != 0)
	{
		msg_t_local -> msg = strdup (chk_elts[4]); // valore riempito solo se valido
		if (msg_t_local -> msg == NULL)
			err_handler (E_MALLOC, 'x', 'w');
	}
	else if (msg_t_local -> type != MSG_OK && chk_elts[4][0] == 0)
		return 3;
	else if (msg_t_local -> type == MSG_OK)
		msg_t_local -> msg = NULL;

	// controllo se msglen == strlen (msg)
	if (msg_t_local -> type != MSG_OK && msg_t_local -> msglen != (unsigned int) strlen (chk_elts[4]))
		msg_t_local -> msglen = strlen (msg_t_local -> msg); // correzione automatica lunghezza

	for (i = 0; i < 5; i++)
		free (chk_elts[i]);

	return 0; // ritorna ok

}
