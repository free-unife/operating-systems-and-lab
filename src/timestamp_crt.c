/*
 * timestamp_crt.c
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * timestamp_crt.c
 *
 * Il file include la semplice funzione timestamp_crt che genera
 * il timestamp in molte parti del programma.
 *
 */

#include "main_includes.h"

/*
 * La funzione crea un timestamp e lo mette nell'array di caratteri out_time.
 * La variabile t contiene il timestamp originale, tmp contiene il timestamp
 * che rappresenta il numero di secondi dallo UNIX epoch e out_time coniene la
 * data in forma leggibile. out_time è il puntatore di tipo char * passato
 * alla funzione.
 */
void timestamp_crt (char *out_time, int len)
{

	time_t t; // conterrà il timestamp
	struct tm *tmp; // struttura contenente tutti i parametri della data
	int ret; // valore di ritorno

	t = time (NULL); // gnerazione timestamp
	tmp = localtime (&t); // trasforma il tmestamp come numero di secondi passati dallo UNIX epoch
	if (tmp == NULL)
		err_handler (E_TIME, 'x', 'w');

	ret = strftime (out_time, sizeof (out_time) * len, "%c", tmp); // è stata usata questa funzione perchè ctime_r è obsoleto. %c = "The preferred daten and time representation for the current locale"
	if (ret == 0)
		err_handler (E_TIME, 'x', 'w');

}
