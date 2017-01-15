/*
 * main_includes.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * main_includes.h
 *
 * Questo file contiene tutte le informazioni per l'inclusione condizionale degli headers.
 * In questo modo gli headers vengono inclusi una sola volta al momento della compilazione.
 *
 */

#ifndef M_HASH_H
#define M_HASH_H
	#include "hash.h"
#endif

#ifndef GLOBAL_VARS
#define GLOBAL_VARS
	extern int go; // dichiarazione. l'inizializzazone si trova nel file sorgente chat-server.c
	extern int numThreadAttivi;
	extern int log_fd; // variabile globale per il log file descriptor
	extern int msd; // variabile globale per il master socket descriptor
	extern hash_t H; // hash globale
	extern users *head_signal; // testa globale per la lista degli utenti
#endif

#ifndef M_STDIO_H
#define M_STDIO_H
	#include <stdio.h>
#endif

#ifndef M_STDLIB_H
#define M_STDLIB_H
	#include <stdlib.h>
#endif

#ifndef M_SYS_TYPES_H
#define M_SYS_TYPES_H
	#include <sys/types.h>
#endif

#ifndef M_SYS_STAT_H
#define M_SYS_STAT_H
	#include <sys/stat.h> // controlla stato file
#endif

#ifndef M_PTHREAD_H
#define M_PTHREAD_H
	#include <pthread.h>
#endif

#ifndef M_FCNTL_H
#define M_FCNTL_H
	#include <fcntl.h>
#endif

#ifndef M_SYS_SOCKET_H
#define M_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifndef M_SIGNAL_H
#define M_SIGNAL_H
	#include <signal.h>
#endif

#ifndef M_SIGNAL_H
#define M_SIGNAL_H
	#include <signal.h>
#endif

#ifndef M_NETINET_IN_H
#define M_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifndef M_ARPA_INET_H
#define M_ARPA_INET_H
	#include <arpa/inet.h>
#endif

#ifndef M_UNISTD_H
#define M_UNISTD_H
	#include <unistd.h> // per exit
#endif

#ifndef M_STRING_H
#define M_STRING_H
	#include <string.h>
#endif

#ifndef M_STRINGS_H
#define M_STRINGS_H
	#include <strings.h> // per bzero
#endif

#ifndef M_TIME_H
#define M_TIME_H
	#include <time.h>
#endif

#ifndef M_SYS_TIME_H
#define M_SYS_TIME_H
	#include <sys/time.h>
#endif

#ifndef M_ERRNO_H
#define M_ERRNO_H
	#include <errno.h>
#endif

#ifndef M_SYS_IOCTL_H
#define M_SYS_IOCTL_H
	#include <sys/ioctl.h> // controlla stream socket.
#endif

// Qusi tutti i seguenti header fanno riferimento a funzioni condivise fra più file

#ifndef M_DO_TMAIN_H
#define M_DO_TMAIN_H
	#include "do_tmain.h" // i vari thread del server
#endif

#ifndef M_DO_TWORKER_H
#define M_DO_TWORKER_H
	#include "do_tworker.h"
#endif

#ifndef M_DO_TDISPATCHER_H
#define M_DO_TDISPATCHER_H
	#include "do_tdispatcher.h"
#endif

#ifndef M_COMM_PROT_H
#define M_COMM_PROT_H
	#include "comm_prot.h" // protocollo di comunicazione fra client e server contenente il formato dei messaggi
#endif

#ifndef M_TIMESTAMPCRT_H
#define M_TIMESTAMPCRT_H
	#include "timestamp_crt.h" // crea timestamp
#endif

#ifndef M_STR_TOKEN_READ_H
#define M_STR_TOKEN_READ_H
	#include "str_token_read.h" // per tokenizzare file usr e messaggi login e reglog
#endif

#ifndef M_ERR_HANDLER_H
#define M_ERR_HANDLER_H
	#include "err_handler.h" // scrittura errore
#endif

#ifndef M_WRITE_SOCK_H
#define M_WRITE_SOCK_H
	#include "write_sock.h" // scrittura mesaggi sul csd
#endif

#ifndef M_PARSE_H
#define M_PARSE_H
	#include "parse.h" // lettura e parsing messaggio sul socket
#endif
