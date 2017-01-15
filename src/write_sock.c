/*
 * write_sock.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * write_sock.c
 *
 * Il file contiene la funzione write_sock che serve
 * a scrivere su un socket. È una funzione condivisa da client/server.
 */

#include "main_includes.h"

/*
 * Funzione per l'invio dei messaggi al client/server.
 * La variabile csd_local rappresenta il socket su cui inviare il messaggio.
 * type_macro è la variabile che rappresenta una delle macro che fa parte del protocollo di comunicazione, presente in comm_prot.h
 * A seconda dei casi le variabili type_macro e msg_to_send posono essere settate a NULL, quindi vanno distinti 3 casi.
 * Le variabili (una o entrambe) vengono copiate in msg con snprintf. Inine viene mandato msg con una chiamata a send.
 * La funzione non ha valori di ritorno.
 */
void write_sock (char *type_macro, char *msg_to_send, int csd_local)
{

	char *msg; // messaggio finale, da inviare
	ssize_t ret_send; // valore ritorno send
	size_t len_msg, len_msg_to_send = 0, len_type_macro = 0; // lunghezze delle varie stringhe


	if (msg_to_send != NULL) // calcolo lunghezze stringhe
		len_msg_to_send = strlen (msg_to_send) + 2;
	if (type_macro != NULL)
		len_type_macro = strlen (type_macro) + 2;

	len_msg = len_msg_to_send + len_type_macro;
	msg = (char *) malloc (sizeof (char) * len_msg); // allocazione stringa da inviare
	if (msg == NULL)
		err_handler (E_MALLOC, 'x', 'w'); // write_sock è condivisa, quindi il parametro 'w' dovrebbe essere settatto a 0 per ilclient

	if (type_macro == NULL)
		snprintf (msg, len_msg - 1, "%s", msg_to_send); // salvataggio stringa in msg
	if (msg_to_send == NULL)
		snprintf (msg, len_msg - 1, "%s", type_macro);
	if ((msg_to_send != NULL) && (type_macro != NULL))
		snprintf (msg, len_msg - 1, "%s%s", type_macro, msg_to_send);

	msg[len_msg - 2] = '\n'; // aggiungo carattere newline e fine stringa
	msg[len_msg - 1] = '\0';

	ret_send = send (csd_local, msg, len_msg, 0); // invio messaggio sul socket
	if (ret_send != (ssize_t) len_msg)
		err_handler (E_WRITE_SOCK, 0, 'w');

	free (msg);

}
