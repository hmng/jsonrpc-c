/*
 * server.c
 *
 *  Created on: Oct 9, 2012
 *      Author: hmng
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "jrpc_server.h"

#define PORT 1234  // the port users will be connecting to

jrpc_server_t my_server;

void handle_kill_signal() {
	jrpc_server_stop(&my_server);
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
}

cJSON * say_hello(jrpc_context_t *ctx, cJSON *params, cJSON *id) {
	char hello_name[100];
	cJSON *name = NULL;
	if (params && params->type == cJSON_Array) {
		name = cJSON_GetArrayItem(params, 0);
	}
	if (name) {
		if (!sprintf(hello_name, "Hello %s!", name->valuestring))
				perror("sprintf");
		return cJSON_CreateString(hello_name);
	}
	return cJSON_CreateString("Hello!");
}

cJSON * exit_server(jrpc_context_t *ctx, cJSON *params, cJSON *id) {
	jrpc_server_stop(&my_server);
	return cJSON_CreateString("Bye!");
}

int main(void) {
	struct sigaction action;

	//setenv("JRPC_DEBUG", "1", 1); /* uncomment to active debug */

	jrpc_server_init(&my_server, PORT);
	jrpc_register_procedure(&my_server.procedure_list, say_hello, "sayHello", NULL);
	jrpc_register_procedure(&my_server.procedure_list, exit_server, "exit", NULL);

	// Add signal handler to terminate server
	action.sa_handler = handle_kill_signal;
	add_signal(&my_server, SIGINT, &action);
	add_signal(&my_server, SIGTERM, &action);
	add_signal(&my_server, SIGHUP, &action);

	jrpc_server_run(&my_server);

	jrpc_server_destroy(&my_server);
	if (my_server.debug_level)
		printf("close jrpc-server\n");
	return 0;
}
