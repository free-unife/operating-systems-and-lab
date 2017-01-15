/*
 * err_handler.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * err_handler.h
 *
 * Contiene tutte le macro con le corrispondenti stringhe degli errori.
 *
 */

// Macro per gli errori stderr e log file
#define		E_FORK_FAILED			"fork fallita"
#define		E_USR_FILE_OPEN			"impossibile aprire file utenti"
#define		E_TMAIN_OP			"problema thread main"
#define		E_USR_FILE_FDOPEN		"impossibile aprire file utenti con fdopen"
#define		E_USR_ALREADY_EXISTS		"impossibile aggiungere utente alla tabella utenti, già presente"
#define		E_LOG_FILE_OPEN			"impossibile aprire log file"
#define		E_LOG_FILE_WRITE		"problema scrittura log file"
#define		E_TDISPATCHER_OP		"problema thread dispatcher"
#define		E_TWORKER_OP			"problema thread worker"
#define		E_ACCEPT			"problema accept"
#define		E_FILE_DESC			"problema file descriptor"
#define		E_SOCK				"problema socket"
#define		E_BIND				"problema bind"
#define		E_LISTEN			"problema listen"
#define		E_TIME				"impossibile assegnare data e ora"
#define		E_WRITE_SOCK			"problema write socket"
#define		E_MALLOC			"problema malloc"
#define		E_USR_ALREADY_LOGGED		"utente già loggato"
#define		E_CLOSE_SOCK			"problema chiusura socket"
#define		E_USR_NOT_REG			"utente non esiste, occorre registrazione"
#define		E_USR_NOT_CONN			"utente remoto non collegato"
#define		E_USR_ALREADY_REG		"utente già registrato"
#define		E_INVALID_FORMAT		"formato messaggio non valido"
#define		E_UNKNOWN_CMD			"comando sconosciuto"
#define		E_MUST_LOGIN_FOR_CMD		"per eseguire il comando bisogna loggarsi"
#define		E_NOT_THAT_USR_IN_TABLE		"impossibile trovare utente nella tabella hash"
#define		E_MSG_TO_USR			"invio messaggio all'utente fallito"
#define		E_ALREADY_LOGGED_FOR_CMD	"impossibile eseguire comando, utente già loggato"
#define		E_READ_SOCK			"errore read socket"
#define		E_USR_NUMBER			"problema numero utenti"
#define		E_INVALID_USR_EMAIL		"l'utente non ha una email valida"
#define		E_IOCTL				"problema ioctl"
#define		E_TOO_MANY_INVALID_CMDS		"troppi comandi errati, disconnessione forzata"
#define		E_INVALID_USR_NAME		"l'utente non ha un nome utente valido"
#define		E_EMPTY_USR_LIST		"tabella utenti vuota"
#define		E_INVALID_USR_FULLNAME		"l'utente non ha un nome utente completo valido"
#define		E_WRITE_USR_RECORD_FILE		"problema scrittura record utente sul file"
#define		E_SETSOCKOPT			"problema setsockopt"
#define		E_CONNECT			"collegmento al server fallito"
#define		E_TSENDER_OP			"problema thread sender"
#define		E_TRECEIVER_OP			"problema thread receiver"
#define		E_STDIN				"problema lettura stdin"
#define		E_KILL				"impossibile eseguire kill"

// prototipi
void err_handler_argv (char *msg_err, char *arg_err, char x, char wr_to_log);
void err_handler (char *msg_err, char x, char wr_to_log);
