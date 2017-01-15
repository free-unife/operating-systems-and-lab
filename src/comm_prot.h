/*
 * comm_prot.h
 *
 * Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

/*
 * comm_prot.h
 *
 * Communication protocol macros and struct.
 *
 */

#define MSG_LOGIN	'L' // login
#define MSG_REGLOG	'R' // register and login
#define MSG_OK		'O' // ok
#define MSG_ERROR	'E' // error
#define MSG_SINGLE	'S' // to single user
#define MSG_BRDCAST	'B' // broadcasted
#define MSG_LIST	'I' // list
#define MSG_LOGOUT	'X' // logout

typedef struct
{
	char type; // message type
	char * sender; // message sender
	char * receiver; // message receiver
	unsigned int msglen; // message length
	char * msg; // message buffer
} msg_t;
