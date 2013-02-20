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
#define JRPC_INVALID_PARAMS -32603
#define JRPC_INTERNAL_ERROR -32693

typedef struct {
	void *data;
	int error_code;
	char *error_message;
	cJSON *result;
} jrpc_context;

typedef cJSON* (*jrpc_function)(jrpc_context *context, cJSON *params, cJSON *id);

typedef struct {
	char *name;
	jrpc_function function;
	void *data;
} jrpc_procedure;

typedef struct {
	char *method;
	cJSON *params;
	cJSON *id;
} jrpc_request;

typedef struct {
	jrpc_procedure *procedures;
	int count;
} procedure_list_t;

cJSON *create_json_error(int code, char* message, cJSON *id);

cJSON *create_json_result(cJSON *result, cJSON *id);

int validate_request(cJSON *root, jrpc_request *request);

int find_context(jrpc_context *ctx, procedure_list_t *procedure_list,
		jrpc_request *request);

int jrpc_register_procedure(procedure_list_t *procedure_list,
		jrpc_function function_pointer, char *name, void *data);

int jrpc_deregister_procedure(procedure_list_t *procedure_list, char *name);

void jrpc_procedures_destroy(procedure_list_t *procedure_list);

#endif
