/*
 * jsonrpc-c.c
 *
 *  Created on: Oct 11, 2012
 *      Author: hmng
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsonrpc-c.h"

static void jrpc_procedure_destroy(jrpc_procedure_t *procedure);
static int send_response(jrpc_request_t *request, char *response);
static cJSON *create_json_error(int code, char* message, cJSON *id);
static cJSON *create_json_result(cJSON *result, cJSON *id);
static int validate_request(cJSON *root, jrpc_request_t *request);
static int exec_context(jrpc_context_t *ctx, procedure_list_t *procedure_list,
		jrpc_request_t *request);
static int invoke_procedure(jrpc_request_t *request, procedure_list_t *procedure_list);

static int send_response(jrpc_request_t *request, char *response) {
	if (request->debug_level)
		printf("JSON Response on fd %d:\n%s\n", request->fd, response);
	write(request->fd, response, strlen(response));
	write(request->fd, "\n", 1);
	return 0;
}

int send_error(jrpc_request_t *request, int code, char *message) {
	int return_value = 0;
	cJSON *result_root = create_json_error(code, message, request->id);
	char *str_result = cJSON_Print(result_root);
	return_value = send_response(request, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	free(message);
	return return_value;
}

int send_result(jrpc_request_t *request, cJSON *result) {
	int return_value = 0;
	cJSON *result_root = create_json_result(result, request->id);
	char *str_result = cJSON_Print(result_root);
	return_value = send_response(request, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	return return_value;
}

static cJSON *create_json_error(int code, char *message, cJSON *id) {
	cJSON *result_root = create_json_result(NULL, id);
	cJSON *error_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(error_root, "code", code);
	cJSON_AddStringToObject(error_root, "message", message);
	cJSON_AddItemToObject(result_root, "error", error_root);
	return result_root;
}

static cJSON *create_json_result(cJSON *result, cJSON *id) {
	cJSON *result_root = cJSON_CreateObject();
	cJSON_AddItemToObject(result_root, "jsonrpc",
			cJSON_CreateString(strdup(JRPC_VERSION)));
	if (result)
		cJSON_AddItemToObject(result_root, "result", result);
	if (!id)
		return result_root;

	if (id->type == cJSON_Number && !id->valueint) {
		cJSON_AddItemToObject(result_root, "id", cJSON_CreateNull());
		cJSON_Delete(id);
	} else {
		cJSON_AddItemToObject(result_root, "id", id);
	}
	return result_root;
}

int eval_request(jrpc_request_t *request, cJSON *root, procedure_list_t *procedure_list) {
	int validate = validate_request(root, request);
	char err_tbl[100];
	char *err;

	if (validate > 0) {
		if (request->debug_level)
			printf("Method Invoked: %s\n", request->method);
		return invoke_procedure(request, procedure_list);
	}
	/* error situation */
	request->id = cJSON_CreateNull();
	switch (validate) {
	case -1:
		if (!sprintf(err, "The JSON-RPC-c only support version %s.", JRPC_VERSION)) {
			perror("sprintf");
			exit(EXIT_FAILURE);
		}
		send_error(request, JRPC_INVALID_REQUEST, strdup(err));
		break;
	case -2:
		err = "The JSON sent has invalid params. Need Object or Array.";
		send_error(request, JRPC_INVALID_PARAMS, strdup(err));
		break;
	default:
		err = "The JSON sent is not a valid Request object.";
		send_error(request, JRPC_INVALID_REQUEST, strdup(err));
	}
	return -1;
}

static int validate_request(cJSON *root, jrpc_request_t *request) {
	/* Return 1 if valid request
	 * Return 0 if not following members of Request
	 * Return -1 if jsonrpc is not good version : JRPC_VERSION
	 * Return -2 if jsonrpc has invalid params
	 */
	cJSON *version, *method, *params, *id;

	version = cJSON_GetObjectItem(root, "jsonrpc");
	if (version == NULL)
		return 0;
	else if (version->type == cJSON_String &&
				strcmp(version->valuestring, JRPC_VERSION))
		return -1;

	method = cJSON_GetObjectItem(root, "method");
	if (method == NULL || method->type != cJSON_String)
		return 0;

	params = cJSON_GetObjectItem(root, "params");
	if (params != NULL && !(params->type == cJSON_Array ||
				params->type == cJSON_Object))
		return -2;

	id = cJSON_GetObjectItem(root, "id");
	if (id != NULL && !(id->type == cJSON_String || id->type == cJSON_Number
				|| id->type == cJSON_NULL))
		return 0;

	/* We have to copy ID because using it on the reply and
	* deleting the response Object will also delete ID
	*/
	if (id != NULL) {
		request->id = (id->type == cJSON_String) ? cJSON_CreateString(
				id->valuestring) : cJSON_CreateNumber(id->valueint);
		request->is_notification = 0;
	} else {
		request->id = NULL;
		request->is_notification = 1;
	}
	request->method = method->valuestring;
	request->params = params;
	return 1;
}

static int exec_context(jrpc_context_t *ctx, procedure_list_t *procedure_list,
		jrpc_request_t *request) {
	int i = procedure_list->count;
	while (i--) {
		if (!strcmp(procedure_list->procedures[i].name, request->method)) {
			ctx->data = procedure_list->procedures[i].data;
			ctx->result = procedure_list->procedures[i].function(ctx,
					request->params, request->id);
			return 1;
		}
	}
	return 0;
}

static int invoke_procedure(jrpc_request_t *request, procedure_list_t *procedure_list) {
	jrpc_context_t ctx;
	ctx.error_code = 0;
	ctx.error_message = NULL;
	ctx.result = NULL;
	ctx.data = NULL;

	int context = exec_context(&ctx, procedure_list, request);
	if (request->is_notification) {
		if (request->debug_level)
			printf("Don't response to notification.\n");
		return context;
	}

	if (!context) {
		return send_error(request, JRPC_METHOD_NOT_FOUND,
				strdup("Method not found."));
	}

	if (ctx.error_code) {
		return send_error(request, ctx.error_code, ctx.error_message);
	}
	return send_result(request, ctx.result);
}

static void jrpc_procedure_destroy(jrpc_procedure_t *procedure) {
	if (procedure->name){
		free(procedure->name);
		procedure->name = NULL;
	}
	if (procedure->data){
		free(procedure->data);
		procedure->data = NULL;
	}
}

int jrpc_register_procedure(procedure_list_t *procedure_list,
		jrpc_function function_pointer, char *name, void *data) {
	int i = procedure_list->count++;
	if (!procedure_list->procedures)
		procedure_list->procedures = malloc(sizeof(jrpc_procedure_t));
	else {
		jrpc_procedure_t *ptr = realloc(procedure_list->procedures,
				sizeof(jrpc_procedure_t) * procedure_list->count);
		if (!ptr)
			return -1;
		procedure_list->procedures = ptr;

	}
	if ((procedure_list->procedures[i].name = strdup(name)) == NULL)
		return -1;
	procedure_list->procedures[i].function = function_pointer;
	procedure_list->procedures[i].data = data;
	return 0;
}

int jrpc_deregister_procedure(procedure_list_t *procedure_list, char *name) {
	/* Search the procedure to deregister */
	int i;
	int found = 0;
	if (procedure_list->procedures){
		for (i = 0; i < procedure_list->count; i++){
			if (found) {
				procedure_list->procedures[i-1] = procedure_list->procedures[i];
			} else if(!strcmp(name, procedure_list->procedures[i].name)){
				found = 1;
				jrpc_procedure_destroy(&(procedure_list->procedures[i]));
			}
		}
		if (found){
			procedure_list->count--;
			if (procedure_list->count){
				jrpc_procedure_t *ptr = realloc(procedure_list->procedures,
					sizeof(jrpc_procedure_t) * procedure_list->count);
				if (!ptr){
					perror("realloc");
					return -1;
				}
				procedure_list->procedures = ptr;
			}else{
				procedure_list->procedures = NULL;
			}
		}
	} else {
		fprintf(stderr, "server : procedure '%s' not found\n", name);
		return -1;
	}
	return 0;
}

void jrpc_procedures_destroy(procedure_list_t *procedure_list) {
	int i;
	for (i = 0; i < procedure_list->count; i++){
		jrpc_procedure_destroy(&(procedure_list->procedures[i]));
	}
	procedure_list->count = 0;
	free(procedure_list->procedures);
	procedure_list->procedures = NULL;
}
