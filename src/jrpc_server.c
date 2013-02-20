/*
 * jrpc_server.c
 *
 * Created on: Feb 20, 2013
 *	Author: mathben
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

#include "jrpc_server.h"

static int __jrpc_server_start(jrpc_server_t *server);

void add_signal(jrpc_server_t *server, int signo, struct sigaction *action) {
	ev_signal *s = malloc(sizeof(*s));
	ev_signal_init(s, (void *)action->sa_handler, signo);
	ev_signal_start(server->loop, s);
}

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

static int send_response(jrpc_connection_t *conn, char *response) {
	int fd = conn->fd;
	if (conn->debug_level)
		printf("JSON Response:\n%s\n", response);
	write(fd, response, strlen(response));
	write(fd, "\n", 1);
	return 0;
}

static int send_error(jrpc_connection_t *conn, int code, char *message,
		cJSON *id) {
	int return_value = 0;
	cJSON *result_root = create_json_error(code, message, id);
	char *str_result = cJSON_Print(result_root);
	return_value = send_response(conn, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	free(message);
	return return_value;
}

static int send_result(jrpc_connection_t *conn, cJSON *result, cJSON *id) {
	int return_value = 0;
	cJSON *result_root = create_json_result(result, id);
	char *str_result = cJSON_Print(result_root);
	return_value = send_response(conn, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	return return_value;
}

static int invoke_procedure(jrpc_server_t *server, jrpc_connection_t *conn,
		jrpc_request *request) {
	jrpc_context ctx;
	ctx.error_code = 0;
	ctx.error_message = NULL;
	ctx.result = NULL;
	ctx.data = NULL;
	if (!find_context(&ctx, &server->procedure_list, request)) {
		return send_error(conn, JRPC_METHOD_NOT_FOUND,
				strdup("Method not found."), request->id);
	} else {
		if (ctx.error_code)
			return send_error(conn, ctx.error_code,
					ctx.error_message, request->id);
		else
			return send_result(conn, ctx.result, request->id);
	}
}

static int eval_request(jrpc_server_t *server, jrpc_connection_t *conn,
		cJSON *root) {
	jrpc_request request;
	if (validate_request(root, &request)) {
		if (server->debug_level)
			printf("Method Invoked: %s\n", request.method);
		return invoke_procedure(server, conn, &request);
	}
	send_error(conn, JRPC_INVALID_REQUEST,
			strdup("The JSON sent is not a valid Request object."),
			NULL);
	return -1;
}

static void close_connection(struct ev_loop *loop, ev_io *w) {
	ev_io_stop(loop, w);
	close(((jrpc_connection_t *) w)->fd);
	free(((jrpc_connection_t *) w)->buffer);
	free(((jrpc_connection_t *) w));
}

static void connection_cb(struct ev_loop *loop, ev_io *w, int revents) {
	jrpc_connection_t *conn;
	jrpc_server_t *server = (jrpc_server_t *) w->data;
	size_t bytes_read = 0;
	//get our 'subclassed' event watcher
	conn = (jrpc_connection_t *) w;
	int fd = conn->fd;

	if (server->debug_level)
		printf("callback from fd %d\n", fd);

	if (conn->pos == (conn->buffer_size - 1)) {
		char *new_buffer = realloc(conn->buffer, conn->buffer_size *= 2);
		if (new_buffer == NULL) {
			perror("Memory error");
			return close_connection(loop, w);
		}
		conn->buffer = new_buffer;
		memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos);
	}
	// can not fill the entire buffer, string must be NULL terminated
	int max_read_size = conn->buffer_size - conn->pos - 1;
	if ((bytes_read = read(fd, conn->buffer + conn->pos, max_read_size))
			== -1) {
		perror("read");
		return close_connection(loop, w);
	}
	if (!bytes_read) {
		// client closed the sending half of the connection
		if (server->debug_level)
			printf("Client closed connection.\n");
		return close_connection(loop, w);
	}

	cJSON *root;
	char *end_ptr;
	char *err = "Parse error. Invalid JSON was received by the server.";
	conn->pos += bytes_read;
	if ((root = cJSON_Parse_Stream(conn->buffer, &end_ptr)) == NULL) {
		// did we parse the all buffer? If so, just wait for more.
		// else there was an error before the buffer's end
		if (cJSON_GetErrorPtr() != (conn->buffer + conn->pos)) {
			if (server->debug_level) {
				printf("INVALID JSON Received:\n---\n%s\n---\n",
						conn->buffer);
			}
			send_error(conn, JRPC_PARSE_ERROR, strdup(err), NULL);
			return close_connection(loop, w);
		}
		/* do nothing */
		return;
	}

	if (server->debug_level) {
		char *str_result = cJSON_Print(root);
		printf("Valid JSON Received:\n%s\n", str_result);
		free(str_result);
	}

	if (root->type == cJSON_Object) {
		eval_request(server, conn, root);
	}
	//shift processed request, discarding it
	memmove(conn->buffer, end_ptr, strlen(end_ptr) + 2);

	conn->pos = strlen(end_ptr);
	memset(conn->buffer + conn->pos, 0,
			conn->buffer_size - conn->pos - 1);

	cJSON_Delete(root);
}

static void accept_cb(struct ev_loop *loop, ev_io *w, int revents) {
	char s[INET6_ADDRSTRLEN];
	jrpc_connection_t *connection_watcher;
	struct sockaddr_storage their_addr; // connector's address information

	connection_watcher = malloc(sizeof(jrpc_connection_t));
	socklen_t sin_size = sizeof their_addr;

	connection_watcher->fd = accept(w->fd, (struct sockaddr *) &their_addr,
			&sin_size);
	if (connection_watcher->fd == -1) {
		perror("accept");
		free(connection_watcher);
	} else {
		if (((jrpc_server_t *) w->data)->debug_level) {
			inet_ntop(their_addr.ss_family,
					get_in_addr((struct sockaddr *)
						&their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);
		}
		ev_io_init(&connection_watcher->io, connection_cb,
				connection_watcher->fd, EV_READ);
		//copy pointer to jrpc_server_t
		connection_watcher->io.data = w->data;
		connection_watcher->buffer_size = 1500;
		connection_watcher->buffer = malloc(1500);
		memset(connection_watcher->buffer, 0, 1500);
		connection_watcher->pos = 0;
		/* copy debug_level, jrpc_connection_t has no pointer to
		* jrpc_server_t
		*/
		connection_watcher->debug_level =
				((jrpc_server_t *) w->data)->debug_level;
		ev_io_start(loop, &connection_watcher->io);
	}
}

int jrpc_server_init(jrpc_server_t *server, int port_number) {
	return jrpc_server_init_with_ev_loop(server, port_number, EV_DEFAULT);
}

int jrpc_server_init_with_ev_loop(jrpc_server_t *server, int port_number,
		struct ev_loop *loop) {
	memset(server, 0, sizeof(jrpc_server_t));
	server->loop = loop;
	server->port_number = port_number;
	char *debug_level_env = getenv("JRPC_DEBUG");
	if (debug_level_env == NULL)
		server->debug_level = 0;
	else {
		server->debug_level = strtol(debug_level_env, NULL, 10);
		printf("JSONRPC-C Debug level %d\n", server->debug_level);
	}
	return __jrpc_server_start(server);
}

static int __jrpc_server_start(jrpc_server_t *server) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	int rv;
	char PORT[6];
	sprintf(PORT, "%d", server->port_number);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	/* loop through all the results and bind to the first we can */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(1);
	}
	if (server->debug_level)
		printf("server: waiting for connections...\n");

	ev_io_init(&server->listen_watcher, accept_cb, sockfd, EV_READ);
	server->listen_watcher.data = server;
	ev_io_start(server->loop, &server->listen_watcher);
	return 0;
}

void jrpc_server_run(jrpc_server_t *server) {
	ev_run(server->loop, 0);
}

int jrpc_server_stop(jrpc_server_t *server) {
	ev_break(server->loop, EVBREAK_ALL);
	return 0;
}

void jrpc_server_destroy(jrpc_server_t *server) {
	jrpc_procedures_destroy(&server->procedure_list);
}

