/*
 * jsonrpc-c.c
 *
 *  Created on: Oct 11, 2012
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

#include "jsonrpc-c.h"

static void jrpc_procedure_destroy(jrpc_procedure *procedure);

cJSON *create_json_error(int code, char* message, cJSON *id) {
	cJSON *result_root = cJSON_CreateObject();
	cJSON *error_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(error_root, "code", code);
	cJSON_AddStringToObject(error_root, "message", message);
	cJSON_AddItemToObject(error_root, "id", id);
	cJSON_AddItemToObject(result_root, "error", error_root);
	return result_root;
}

cJSON *create_json_result(cJSON *result, cJSON *id) {
	cJSON *result_root = cJSON_CreateObject();
	if (result)
		cJSON_AddItemToObject(result_root, "result", result);
	if (id)
		cJSON_AddItemToObject(result_root, "id", id);
	return result_root;
}

int validate_request(cJSON *root, jrpc_request *request) {
	/* Return 1 if valid request */
	cJSON *method, *params, *id;

	method = cJSON_GetObjectItem(root, "method");
	if (method == NULL || method->type != cJSON_String)
		return 0;

	params = cJSON_GetObjectItem(root, "params");
	if (params != NULL && !(params->type == cJSON_Array ||
				params->type == cJSON_Object))
		return 0;

	id = cJSON_GetObjectItem(root, "id");
	if (id != NULL && !(id->type == cJSON_String || id->type == cJSON_Number))
		return 0;

	/* We have to copy ID because using it on the reply and
	* deleting the response Object will also delete ID
	*/
	if (id != NULL)
		request->id = (id->type == cJSON_String) ? cJSON_CreateString(
				id->valuestring) : cJSON_CreateNumber(id->valueint);
	else
		request->id = NULL;
	request->method = method->valuestring;
	request->params = params;
	return 1;
}

int find_context(jrpc_context *ctx, procedure_list_t *procedure_list,
		jrpc_request *request) {
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

static void jrpc_procedure_destroy(jrpc_procedure *procedure) {
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
		procedure_list->procedures = malloc(sizeof(jrpc_procedure));
	else {
		jrpc_procedure * ptr = realloc(procedure_list->procedures,
				sizeof(jrpc_procedure) * procedure_list->count);
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
				jrpc_procedure_destroy( &(procedure_list->procedures[i]) );
			}
		}
		if (found){
			procedure_list->count--;
			if (procedure_list->count){
				jrpc_procedure * ptr = realloc(procedure_list->procedures,
					sizeof(jrpc_procedure) * procedure_list->count);
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
		jrpc_procedure_destroy( &(procedure_list->procedures[i]) );
	}
	procedure_list->count = 0;
	free(procedure_list->procedures);
	procedure_list->procedures = NULL;
}
