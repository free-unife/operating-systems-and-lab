#
# Makefile
#
# Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
# This work is free. You can redistribute it and/or modify it under the
# terms of the Do What The Fuck You Want To Public License, Version 2,
# as published by Sam Hocevar. See the LICENSE file for more details.
#

#
# Makefile
#
# Makefile per ChatFe con tre target
#

# macros
CC = gcc
SRC_DIR = ./src
BIN_DIR = ../bin
CFLAGS = -Wall -Wextra #-std=gnu11 -pedantic -g
LIBS = -pthread #-lpthread non funziona su sistemi con base ubuntu
SRV_NAME = chat-server
CLI_NAME = chat-client
SRV_OBJFILES = chat-server.o do_tdispatcher.o do_tmain.o do_tworker.o err_handler.o hash.o parse.o str_token_read.o timestamp_crt.o write_sock.o
CLI_OBJFILES = chat-client.o err_handler.o parse.o timestamp_crt.o write_sock.o
USR_FILE = user-file

# targets

default : chat # default target

%.o: %.c *.h # compilazione dei singoli file, tenendo conto delle modifiche di tutti gli header file (*.h) oltre che i .c file.
	@echo "$(CC) $< $(CFLAGS) $(LIBS)"
	@$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< # compilazione dei singoli object file

chat :  $(SRV_OBJFILES) $(CLI_OBJFILES) *.h
	@echo "$(CC) -o $(SRV_NAME) $(CFLAGS) $(LIBS)"
	@$(CC) $(CFLAGS) $(LIBS) $(SRV_OBJFILES) -o $(SRV_NAME) # linking degli eseguibili
	@echo "$(CC) -o $(CLI_NAME) $(CFLAGS) $(LIBS)"	
	@$(CC) $(CFLAGS) $(LIBS) $(CLI_OBJFILES) -o $(CLI_NAME)
	@echo "Compilazione terminata."

install :
	@echo "Installazione eseguibili in $(BIN_DIR) ..."
	@mkdir -p ../bin # se non esiste la directory ../bin la crea, se esiste ignora errore
	@install -Dm 500 $(SRV_NAME) $(CLI_NAME) $(BIN_DIR) # permesso rx all'utente possessore del file
	@install -Dm 600 $(USR_FILE) $(BIN_DIR) # permesso rw all'utente possessore del file
	@echo "Installazione eseguita."

clean :
	@echo "Pulizia degli object file in corso..."
	@rm -fv *.o $(SRV_NAME) $(CLI_NAME)
	@echo "Pulizia degli object file terminata."

# per proteggere i file con i nomi seguenti si usa la regola .PHONY:
.PHONY : chat install clean
