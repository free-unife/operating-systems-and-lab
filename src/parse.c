/*
 * parse.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * parse_and_demar.c
 *
 * Il file contiene la funzione get_and_parse_cmd, condivisa da client e server.
 * La funzione legge i dati in entrata dal socket, ne controlla la validità,
 * poi chiama la funzione demar (specifica per server e client) che scompone
 * e salva il messaggio.
 *
 */

#include "main_includes.h"

/*
 * Funzione di lettura e parsing dal socket.
 * La funzione si mette immediatamente in lettura sul socket, aspettando di leggere il primo byte dummy_byte,
 * in modo da rendere la chiamata a ioctl bloccante. la variabile total_msglen conterrà la lunghezza totale
 * del messaggio escluso il primo byte. Questo serve per la malloc del buff_msg.
 * Successivamente viene controllato il formato del messaggio con la variabile total_msglen.
 * Viene letto il messaggio e salvato in buff_msg. Vengono controllati il numero dei ':' nel messaggio, che
 * non devono essere meno di 4, attravero la variabile occur.
 * Infine viene tolto il carattere '\n' o '\r', se presenti, dal buffer msg e viene chiamata la funzione demar
 * (specifica per il client e il server) per il demarshalling.
 *
 * Valori di ritorno
 * =================
 *
 * 0 = OK
 * 1 = errore fatale / socket chiuso / chiusura forzata
 * 2 = comando sconosciuto
 * 3 = errore formato
 */
int get_and_parse_cmd (msg_t *msg_t_local, int csd_local)
{

	ssize_t ret_io_bytes; // usata da ioctl
	char *buff_msg; // buffer che contiene messaggio in entrata
	int total_msglen = 0, i = 0, occur = 0; // variabili usate per controllo stringa letta dal socket
	char dummy_byte[2]; // byte fittizio per rendere la ioctl bloccante
	int ret_demar; // intero di ritorno per la funzione demar
	const char ctrl_c = -1; // se un client collegato con telnet fa ctrl-c deve uscire. CTRL-C è il carattere 0x03 ma viene letto come -1 in questo caso


	bzero ((void *) dummy_byte, 2); // azzero il dummy byte

	ret_io_bytes = read (csd_local, dummy_byte, 1); // viene letto il primo byte (dummy) del messaggio. Con questa read si rende ioctl bloccante
	if (ret_io_bytes == 0 || errno == EINVAL) // quando un client si disconnette allora viene chiuso csd. In quel momento la read ritona immediatamente (che ha letto 0 byte) e il ciclo while continua all'infinito....
		return 1;  // ...per questo ci deve essere il ritorno della funzione

	ret_io_bytes = ioctl (csd_local, FIONREAD, &total_msglen); // funzione che dice quanti byte sono disponibli nel buffer, NON bloccante, conta sia il '\n' che il '\0'

	buff_msg = (char *) malloc (sizeof (char) * total_msglen); // allocazione dinamica del buffer in base ai byte presenti nel socket.
	if (buff_msg == NULL)
		err_handler (E_MALLOC, 'x', 'w');
	bzero ((void *) buff_msg, total_msglen); // azzero la nuova stringa

	if (ret_io_bytes == -1)
		err_handler (E_IOCTL, 'x', 'w');
	else
	{
		if (dummy_byte[0] == '\0') // controllo se la stringa è vuota per evitare eventuali crash. Dovrebbe arrivare qui solo se il client chiude immediatamente la connessione dopo avere mandato un msg
			return 1; // ritorna chiusura forzata
		if (strcmp (dummy_byte, ":") != 0) // controllo formato: anche il dummy byte deve essere uguale a -1
		{
			if (dummy_byte[0] == ctrl_c) // questo controlla se l'utente ha premuto CTRL-C (ASCII 3)
 				return 1; // ritorna chiusura forzata

			ret_io_bytes = read (csd_local, buff_msg, total_msglen);
			if (ret_io_bytes != (ssize_t) total_msglen)
				err_handler (E_READ_SOCK, 'x', 'w');
			free (buff_msg);

			return 3; // formato non valido
		}
	}

	if (total_msglen < 5) // primo controllo formato msg. 5 = 1 carattere per il comando + almeno 4 ':'. - 1 perchè non va considerato il newline
	{
		read (csd_local, buff_msg, (size_t) total_msglen); // flush socket altrimenti la funzione viene chiamata di nuovo con i dati vecchi
		free (buff_msg);

		return 2; // bad cmd
	}

	ret_io_bytes = read (csd_local, buff_msg, total_msglen); // copia comando client in un buffer locale msg_buff
	if (ret_io_bytes != (ssize_t) total_msglen)
		err_handler (E_READ_SOCK, 'x', 'w');

	// secondo controllo formato
	i = 0;
	while (buff_msg[i] != '\0') // conto numero di ':' presenti nel mesaggio
	{
		if (buff_msg[i] == ':')
			occur++;
		i++;
	}
	if (occur < 4) // se il messaggio non ha almeno 4 ':'
		return 2; // ritorna unnown cmd

	i = 0;
	while (buff_msg[i] != '\n' && buff_msg[i] != '\r' && buff_msg[i] != '\0') // trova la posizione di '\n' o '\r'
		i++;

	if (buff_msg[i] == '\n' || buff_msg[i] == '\r') // Si prevede che gli ultimi due caratteri siano '\n' e '\r', quindi copio la sringa escludendo '\n' e '\r' (se esistenti)
		buff_msg[i] = '\0';

	ret_demar = demar (buff_msg, msg_t_local); // demarshalling msg

	free (buff_msg);

	if (ret_demar == 2) // comando sconosciuto
		return 2; // ritorna comando sconosciuto
	else if (ret_demar == 3) // formato messaggo non valido
		return 3; // ritorna formato non valido
	/*else if (ret_demar == 65) // se il client ha mandato keepalive allora viene inviato indietro
		write_sock (NULL, ":A::::", csd_local); // manda messaggio keepalive al client*/

	return 0; // OK
}
