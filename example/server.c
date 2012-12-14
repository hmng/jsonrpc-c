/*
 * server.c
 *
 *  Created on: Oct 9, 2012
 *      Author: hmng
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ev.h>
#include "jsonrpc-c.h"

#define PORT 1234  // the port users will be connecting to

struct ev_loop *loop;

cJSON * say_hello(jrpc_context * ctx, cJSON * params, cJSON *id) {
	return cJSON_CreateString("Hello!");
}

cJSON * exit_server(jrpc_context * ctx, cJSON * params, cJSON *id) {
	ev_break(loop, EVBREAK_ALL);
	return cJSON_CreateString("Bye!");
}

int main(void) {
	loop = EV_DEFAULT;
	struct jrpc_server my_server;
	jrpc_server_init(&my_server, loop, PORT);
	jrpc_server_start(&my_server);
	jrpc_register_procedure(&my_server, say_hello, "sayHello", NULL );
	jrpc_register_procedure(&my_server, exit_server, "exit", NULL );
	ev_run(loop, 0);
	return 0;
}
