#ifndef __DAGAME_MYCLIENT_H__
#define __DAGAME_MYCLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../lib/game_client.h"

#define STRLEN 81


//Connection and Disconnection
int startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h);
void disconnect (Client *C);
int doConnect(Client *C, char* cmd);

// Command Line Interface
int docmd(Client *C, char* cmd);
void * shell(void *arg);
void usage(char *pgm);
char* prompt(int menu); 

//TODO Implement these?
static int update_handler(Proto_Session *s );
static int update_event_handler(Proto_Session *s);

#endif
