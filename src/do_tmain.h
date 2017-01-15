/*
 * do_tmain.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * do_tmain.h
 *
 * Contiene definizione struttura da passare al thread main e macro porta server.
 *
 */

#define MSD_PORT 1234 // master socket descriptor port server

struct thread_main_struct // dati thread main
{
        char *log_file; // nome del file di log
        int user_file; // file descriptor del file degli utenti
        char *usr_file; // nome del file utenti
	hash_t *hash; // hash table
};

struct thread_main_struct thread_main; // oggetto che contiene i dati da passare al thread main

void *do_tmain (void *thread_main_args); // funzione eseguita dal thread main
