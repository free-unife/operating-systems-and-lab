/*
 * chat-client.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * chat-client.h
 *
 * Contiene macro per porta e indirizzo server e definizioni strutture dei due thread del client.
 */

#define ADDRESS "127.0.0.1" // indirizzo del server
#define PORT 1234 // porta server

struct thread_sender_struct // dati thread sender
{
	char *uname; // username client
};

struct thread_sender_struct thread_sender; // oggetto che contiene i dati da passare al thread sender


struct thread_receiver_struct // dati thread sender
{
	msg_t *msg_t_struct; // untatore alla struttura definita nel main
	char *uname;
};

struct thread_receiver_struct thread_receiver; // oggetto che contiene i dati da passare al thread receiver


void *do_tsender (void *thread_sender_args); // funzione eseguita dal thread sender
void *do_treceiver (void *thread_receiver_args); // funzione eseguita dal thread receiver
int parse_and_send_msg (char *msg_local);
