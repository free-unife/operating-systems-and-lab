/*
 * err_handler.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * err_handler.c
 *
 * Il file contiene le due funzioni per la stampa deli errori.
 *
 */

# include "main_includes.h"

/*
 * Funzione per la stampa degli errori con messaggio specifico.
 * Ala funzione viene passata una macro contentente la stringa di errore (msg_err), oltre che
 * una descrizione dettagiata dell'errore stesso (arg_err). La variabile x, se settata a 'x'
 * significa che il programma deve terminare immediatamente. La variabile wr_to_log, settata a 'w',
 * significa che l'errore deve essere scritto sul file di log.
 */
void err_handler_argv (char *msg_err, char *arg_err, char x, char wr_to_log)
{

	char *err, *arg_err_space = ", "; // futura stringa errore, delimitatore tra macro
	size_t len_err;

	len_err = strlen (msg_err) + strlen (arg_err) + strlen (arg_err_space) + 1; // calcolo lunghezza per evitare di ripere gli strlen
	err = (char *) malloc (sizeof (char) * len_err);
	if (err == NULL) // per protezione
		exit (EXIT_FAILURE);

	bzero ((void *) err, len_err);

	snprintf (err, len_err, "%s%s%s", msg_err, arg_err_space, arg_err); // err contiene tutta la stringa da stampare (escluso il timestamp)
	err_handler (err, x, wr_to_log); // chiamo la funzione di stampa degli errori con la stringa appena creata e gli stessi valori dei flag

	free (err);

}

/*
 * Questa funzione viene sempre chiamata in caso di errori o direttamente, o da err_handler_argv.
 * Vedi descrizione err_handler_argv.
 *
 * Formato errore
 * ==============
 *
 * timestamp:error_macro[:error_argv]
 */
void err_handler (char *msg_err, char x, char wr_to_log)
{

	char *str_error, *error, *msg_exit; // error è la stringa che contiene l'errore completo, str_error è la copia locale di strerror (errno)
	size_t len_msg_err, len_strerr, len_err, len_msg_exit;
	char time_string[25];


	timestamp_crt (time_string, 25); // creo il timestamp da mettere nell'intestazione del log

	len_msg_err = strlen (msg_err) + 1;
	len_strerr = strlen (strerror (errno)) + 1;
	len_err = len_strerr + len_msg_err + 35; // 34 = 25 + 9 + 1 dove 25 è la time_string e 9 sono ' ' e ':' e '\0'

	str_error = strdup (strerror (errno)); // salvo la stringa contenente l'errore
	error = (char *) malloc (sizeof (char) * (len_err));
	if (str_error == NULL || error == NULL) // per protezione
		exit (EXIT_FAILURE);

	bzero ((void *) error, len_err);

	timestamp_crt (time_string, 25); // creo il timestamp

	if (strncmp (str_error, "Success", strlen ("Success")) == 0) // alcuni errori ritornano "Success" in STRERROR (ERRNO) quindi...
		snprintf (error, len_err - len_strerr - 2, "%s:ERR, %s\n", time_string, msg_err); // ...evita di stampare errore + "Success" ma solo errore
	else
		snprintf (error, len_err, "%s:ERR, %s, %s\n", time_string, msg_err, str_error); // di solito si cade in questo caso

	fprintf (stderr, "%s", error); // stampa su stderr

	if (wr_to_log == 'w')
		write (log_fd, error, len_err); // scrittura sul file di log

	if (x == 'x') // x = 1 sono errori fatali che provocano arresto anomalo programma.
	{
		len_msg_exit = 44; // 25 char di time_string + strlen (":Arresto programma\n") (19).

		msg_exit = (char *) malloc (sizeof (char) * (len_msg_exit));
		if (msg_exit == NULL)
			exit (EXIT_FAILURE);

		bzero ((void *) msg_exit, len_msg_exit);

		snprintf (msg_exit, len_msg_exit, "%s:Arresto programma\n", time_string); // stampa il messaggio di arresto programma
		fprintf (stderr, "%s", msg_exit);
		if (wr_to_log == 'w')
			write (log_fd, msg_exit, len_msg_exit); // scrittura sul file di log

		exit (EXIT_FAILURE); // ritorna -1, cioè con un problema
	}

}
