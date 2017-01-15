/*
 * write_sock.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * write_sock.h
 *
 * Conitene le macro che vengono usate dalla funzione write_sock.
 *
 */

// macro per i messaggi (WR_SOCK)
#define WR_SOCK_OK ":O::::"
#define WR_SOCK_ERR ":E::::"
#define WR_SOCK_LIST ":I::::" // usata solo dal client
#define WR_SOCK_LIST_SRV ":I:::" // usata solo dal server
#define WR_SOCK_LIST_QUERY ":I:::" // usata solo dal client
#define WR_SOCK_LOGIN ":L:::"
#define WR_SOCK_LOGOUT ":X::::"
#define WR_SOCK_REGLOG ":R:::"
//#define WR_SOCK_KEEPALIVE ":A"

void write_sock (char *type_macro, char *msg_to_send, int csd_local);
