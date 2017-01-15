# operating-systems-and-lab

A chat written in C related to "Sistemi operativi e laboratorio" course in UNIFE (University of Ferrara), year 2013-2014

## Structure

This repository contains a chat called chatFe written in C, which was
an assignment to write as part of the exam. Here, on GitHub, there are 
other *chatFe*s made by students of this same course.  It's not up to me to say 
if they are better or worse than mine. Anyway this is what I think of my 
implementation:

- The good:
    - It works.
    - It helped me a lot in the future in:
        - programming
        - LaTex.

- The bad:
    - Comments and report are written in italian language.
    - Quite inefficient.
    - No encryption.

- The ugly:
    - Code is very messy
        - Long functions.
        - Unecessary controls.
        - No C standard is used.
    - Too many files
        - fragmentation.
    - Bad indentation.

## Building and executing

### Building

    $ cd src
    $ make
    $ make install

Compile and copy the executable files in the `./bin` directory.

### Executing

    $ cd bin

server help:

    Uso: ./chat-server USER-FILE LOG-FILE

client help:

    Uso: ./chat-client [-h] [-r "Name Surname email"] username

    ChatFe client
    =============
            DESCRIZIONE
                    Client per collegarsi ad un server ChatFe.
            OPZIONI
                    -r
                            Registra un nuovo utente e connessione al server.
                            Sintassi: ./chat-client -r "Name Surname Email" username
                    -h
                            Stampa questo messaggio di aiuto

            COMANDI
                    #dest receiver:text\n
                            Invia un messaggio `text` al destinatario `receiver`.
                    #dest:text\n
                            Invia un messaggio `text` in broadcast.
                    #ls\n
                            Visualizza la lista degli utenti collegati.
                    #logout\n
                            Disconnessione dal server e uscita dal client.

                    N.B. Tutti i comandi sono preceduti dal carattere `#` che non potrà essere usato come parte del messaggio.

            VISUALIZZAZIONE MESSAGGI
                    OK
                            Messaggio di ok.
                    ERR
                            Messaggio di errore.
                    Lista utenti collegati
                            Visualizza gli utenti collegati in quel momento al server
                    sender:receiver:message
                            Visualizzazione messaggio `msg` inviato dall'utente `sender` e ricevuto da `receiver`.
                    sender:*:message
                            Visualizzazione messaggio `msg` inviato dall'utente `sender` e ricevuto da tutti (messaggio broadcast).
    
### Customization

Once you study the format from the report
you can use *chatFe* via `telnet`. You can also
change listening addresses and ports of client and server
by editing `chat-client.h` and `do_tmain.h` (as well as `do_tmain.c`)
respectively.

## Full list of the makefile targets

- `default`
    - Same as `chat`.
- `chat`
    - Compiles source files into `chat-client` and `chat-server`.
- `install`
    - Copies `chat-client` and `chat-server` into `../bin`.
- `clean`
    - Removes object and executable files.

## Report

You can find a pdf report in italian language under the `doc` directory.
The original source of this file is written in LaTeX and it can be compiled 
with the following command, to obtain `relazione_progetto.pdf`:

    $ pdflatex relazione_progetto.tex

## Final remarks

About any information not reported in this readme, I can assert one statement:

![message.jpg](message.jpg)

Please, don't use this programs nor serious nor for production applications.

## License

Copyright © 2017 Franco Masotti <franco.masotti@student.unife.it>
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the LICENSE file for more details.

Every file in this repository is covered by the WTFPL. I decided not to use
the GPL because all these are implemetations of well known situations,
so the copyleft clause and others clauses are not necessary here.

