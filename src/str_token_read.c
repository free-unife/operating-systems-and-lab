/*
 * str_token.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * str_token.c
 *
 * Il file contiene la funzione str_token_read per la tokenizzazione della stringa finale
 * del messaggio REGLOG e per lo user file.
 *
 */

#include "main_includes.h"

/*
 * Funzione per la tokenizzazione della stringa contenente le informazioni dell'utente.
 * Viene utilizzata sia dal thread main sia dal thread worker. Quando è utilizzata dal thread main
 * serve a leggere lo user file, inoltre il log file non è ancoora stato aperto. Invece quando è
 * utilizzata dal thread worker serve a tokenizzare l'ultima parte del messaggio di REGLOG e,
 * in questo caso il log file è aperto. Per questo motivo vanno distinti i due casi, controllando
 * se la variabile log_fd, il log file descriptor = -1 (cioè non è ancora stato settato).
 * La stringa in input viene salvata localmente in cp. Vengono poi allocate tre stringhe, chk_elts,
 * che servono come copia locale dei tre token. Viene fatta la tokenizzazione poi il salvataggio
 * nella variabile a che è un puntatore alla struttura che conterrà i dati dei token. Prima del
 * salvatagio viene controllata la vadilità dei dati, e, se non validi, viene salvata la stringa
 * "<empty>".
 *
 * i = indice per scorrere stringa cp
 * j = rappresenta il numero di stringa/token corrente, cioè chk_elts[j]
 * k = indice per scorrere stringa chk_elts
 *
 * Valori di ritorno
 * =================
 *
 * 0 = OK
 * 1 = errore
 */
int str_token_read (char *string_local, hdata_t *a) // string_local è string literal cioè è immodificabile e causa crash/undefined behaviour se usata in uncerto modo
{

	char *cp; // copia temporanea modificabile
	char *chk_elts[3] = {NULL, NULL, NULL}; // stringhe temporanee
	int i = 0, j = 0, k = 0; // indici per le stringhe


	cp = strdup (string_local); // copio la stringa localmente
	if (cp == NULL)
	{
		if (log_fd == -1) // modo semplice e veloce per controllare se il file di log è aperto
			err_handler (E_MALLOC, 'x', 0);
		else
			err_handler (E_MALLOC, 'x', 'w');
	}

	for (i = 0; i < 3; i++)
        {
                chk_elts[i] = (char *) malloc (sizeof (char) * 257); // alloco 257 caratteri per ogni array temporaneo, cioè 26 + '\0'
                if (chk_elts[i] == NULL)
		{
			if (log_fd == -1)
				err_handler (E_MALLOC, 'x', 0);
			else
				err_handler (E_MALLOC, 'x', 'w');
		}
		bzero ((void *) chk_elts[i], 257); // azzero la stringa
        }

	i = 0;

	while (cp[i] != '\0') // trovo un newline e lo ignoro
	{
		if(cp[i] == '\n')
			cp[i] = '\0';
		i++;
	}

        i = 0;

	// per avere pieno controllo preferisco scrivermi il ciclo piuttosto che usare strtok
        while (cp[i] != '\0' && j < 3) // tokenizzazione
        {
                if (k > 256) // se supero il numero massimo di caratteri possibili...
                {
                        chk_elts[j][k] = '\0'; // ...aggiungo terminatore stringa nell'ultimo indice, così tronco quella parte di messaggio in più
			return 1; // ritorna errore
                }

                if (cp[i] != ':') // controllo assenza delimitatore
                {
                        chk_elts[j][k] = cp[i]; // salvataggio carattere in stringa temporanea
                        k++; // avanzamento indice caratteri stringa temp

                }
                else // se viene letto un delimitatore...
                {
                        chk_elts[j][k] = '\0'; // ...aggiunta terminatore stringa
                        j++; // avanzo a stringa temp. successiva
                        k = 0; // azzero indice caratteri stringa temp
		}
		i++; // avanzamento prossimo carattere stringa cp
	}

	free (cp);

	if (chk_elts[0][0] != '\0') // controllo se il primo campo non è vuoto
	{
		a -> uname = strdup (chk_elts[0]); // salvataggio stringa
		if (a -> uname == NULL)
			err_handler (E_MALLOC, 'x', 0);

		if (strchr (a -> uname, ' ') != NULL) // se c'è uno spazio nello user name...
		{
			if (log_fd == -1) // ...avverti ma continua il salvataggio
				err_handler_argv (E_INVALID_USR_NAME, a -> uname, 0, 0);
			else
				err_handler_argv (E_INVALID_USR_NAME, a -> uname, 0, 'w');
		}
	}
	else
	{
		if (log_fd == -1)
			err_handler (E_INVALID_USR_NAME, 0, 0);
		else
			err_handler (E_INVALID_USR_NAME, 0, 'w');

		return 1; // lo user name è invalido quindi non proseguire e ritorna errore
	}

	if (chk_elts[1][0] != '\0') // analogo a sopra
	{
		a -> fullname = strdup (chk_elts[1]);
		if (a -> fullname == NULL)
			err_handler (E_MALLOC, 'x', 0);

	}
	else
	{
		a -> fullname = "<empty>";

		if (log_fd == -1)
			err_handler (E_INVALID_USR_FULLNAME, 0, 0);
		else
			err_handler (E_INVALID_USR_FULLNAME, 0, 'w');
	}

	if (chk_elts[2][0] != '\0')
	{
		a -> email = strdup (chk_elts[2]);
		if (a -> email == NULL)
			err_handler (E_MALLOC, 'x', 0);

		if ((strchr (a -> email, '@') == NULL) || (strchr (a -> email, '.') == NULL)) // di solito le email hanno almeno una '@' e un '.'
		{
			if (log_fd == -1) // se non hanno quei due caratteri avverti ma prosegui
				err_handler_argv (E_INVALID_USR_EMAIL, a -> uname, 0, 0);
			else
				err_handler_argv (E_INVALID_USR_EMAIL, a -> uname, 0, 'w');
		}
	}
	else
	{
		a -> email = "<empty>";

		if (log_fd == -1)
			err_handler (E_INVALID_USR_EMAIL, 0, 0);
		else
			err_handler (E_INVALID_USR_EMAIL, 0, 'w');
	}

	for (i = 0; i < 3; i++)
                free (chk_elts[i]);

	return 0; // OK

}
