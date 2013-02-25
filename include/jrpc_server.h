/*
 * jrpc_server.h
 *
 * Created on: Feb 20, 2013
 *	Author: mathben
 */

#ifndef JRPC_SERVER_H_
#define JRPC_SERVER_H_

#include <ev.h>
#include <signal.h>
#include "jsonrpc-c.h"

typedef struct {
	int port_number;
	struct ev_loop *loop;
	ev_io listen_watcher;
	procedure_list_t procedure_list;
	int debug_level;
} jrpc_server_t;

typedef struct {
	struct ev_io io;
	int fd;
	int pos;
	unsigned int buffer_size;
	char * buffer;
	int debug_level;
} jrpc_connection_t;

void add_signal(jrpc_server_t *server, int signo, struct sigaction *action);

int jrpc_server_init(jrpc_server_t *server, int port_number);

int jrpc_server_init_with_ev_loop(jrpc_server_t *server, int port_number,
		struct ev_loop *loop);

void jrpc_server_run(jrpc_server_t *server);

int jrpc_server_stop(jrpc_server_t *server);

void jrpc_server_destroy(jrpc_server_t *server);

#endif
