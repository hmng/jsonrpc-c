/*
 * jsonrpc-c.h
 *
 *  Created on: Oct 11, 2012
 *      Author: hmng
 */

#ifndef JSONRCPC_H_
#define JSONRPCC_H_

#include "cJSON.h"

/*
 *
 * http://www.jsonrpc.org/specification
 *
 * code		message		meaning
 * -32700	Parse error	Invalid JSON was received by the server.
 * An error occurred on the server while parsing the JSON text.
 * -32600	Invalid Request	The JSON sent is not a valid Request object.
 * -32601	Method not found	The method does not exist / is not available.
 * -32602	Invalid params	Invalid method parameter(s).
 * -32603	Internal error	Internal JSON-RPC error.
 * -32000 to -32099	Server error	Reserved for implementation-defined server-errors.
 */

#define JRPC_PARSE_ERROR -32700
#define JRPC_INVALID_REQUEST -32600
#define JRPC_METHOD_NOT_FOUND -32601
#define JRPC_INVALID_PARAMS -32602
#define JRPC_INTERNAL_ERROR -32603

#define JRPC_VERSION "2.0"

typedef struct {
	void *data;
	int error_code;
	char *error_message;
	cJSON *result;
} jrpc_context_t;

typedef cJSON* (*jrpc_function)(jrpc_context_t *context, cJSON *params, cJSON *id);

typedef struct {
	char *name;
	jrpc_function function;
	void *data;
} jrpc_procedure_t;

typedef struct {
	char *method;
	cJSON *params;
	cJSON *id;
	int is_notification;
	int fd; // file descriptor
	int debug_level;
} jrpc_request_t;

typedef struct {
	jrpc_procedure_t *procedures;
	int count;
} procedure_list_t;

int send_error(jrpc_request_t *request, int code, char *message);
int send_result(jrpc_request_t *request, cJSON *result);
int eval_request(jrpc_request_t *request, cJSON *root, procedure_list_t *procedure_list);
int jrpc_register_procedure(procedure_list_t *procedure_list,
		jrpc_function function_pointer, char *name, void *data);
int jrpc_deregister_procedure(procedure_list_t *procedure_list, char *name);
void jrpc_procedures_destroy(procedure_list_t *procedure_list);

#endif
