#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"
#include "algs.h"
#include "decode.h"
#include "jwt_middleware.h"

#define PORT 8081

// Structure to store incoming JSON data
struct ConnectionInfo {
    char* json_data;
    size_t json_size;
};

int answer_to_connection(void* cls, struct MHD_Connection* connection,
    const char* url, const char* method, const char* version,
    const char* upload_data, size_t* upload_data_size, void** con_cls) {

    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response* response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Accept, Authorization");
        MHD_add_response_header(response, "Access-Control-Max-Age", "86400");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Handle "/newproject" POST request
    if (strcmp(url, "/newproject") == 0 && strcmp(method, "POST") == 0) {
        printf("Handling POST request for /newproject\n");
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check permission to create projects (only MANAGER allowed)
        if (check_permission(&auth, PERM_CREATE_PROJECTS) != 0) {
            return send_forbidden_response(connection, "Only managers can create projects");
        }

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);

        if (*upload_data_size > 0) {
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }
        else {
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                return MHD_YES;
            }

            Project project;
            if (parse_project_from_json(json, &project) == 0) {
                const char *auth = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
                fprintf(stderr, "token %s\n", auth);

                int pure_token_length = strlen(auth) - 7;
                char pure_token[pure_token_length + 1];
                for (int i = 0; i < pure_token_length; ++i) {
                    pure_token[i] = auth[i + 7];
                }
                pure_token[pure_token_length] = '\0';
                fprintf(stderr, "pure token %s\n", pure_token);

                const char *var_name = "HMAC_KEY";
                char *hmac_key = getenv(var_name);
                fprintf(stderr, "key %s\n", hmac_key);

                struct l8w8jwt_decoding_params params;
                l8w8jwt_decoding_params_init(&params);

                params.alg = L8W8JWT_ALG_HS512;

                params.jwt = pure_token;
                params.jwt_length = pure_token_length;

                params.verification_key = (unsigned char*)hmac_key;
                params.verification_key_length = strlen(hmac_key);

                params.validate_iss = "Trello Clone"; 
                printf("KREK CAMUSKI\n");

                params.validate_exp = 1; 
                params.exp_tolerance_seconds = 60;

                params.validate_iat = 1;
                params.iat_tolerance_seconds = 60;

                enum l8w8jwt_validation_result validation_result;

                struct l8w8jwt_claim* claims = NULL;
                size_t claims_len = 0;

                int decode_result = l8w8jwt_decode(&params, &validation_result, &claims, &claims_len);
                printf("decode_result=%d, validation_result=%d, claims_len=%zu\n", decode_result, validation_result, claims_len);
                printf("KREK CRNI\n");

                const char *claim_ptr;
                if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID) {

                    for (size_t i = 0; i < claims_len; ++i) {
                        if (strcmp(claims[i].key, "sub") == 0) {
                            claim_ptr = claims[i].value;
                        }
                    }

                }
                else {
                    const char* error_response = "Bad token";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                        (void*)error_response, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                    MHD_destroy_response(response);
                    return ret;
                }

                project.moderator[0] = '\0';
                snprintf(project.moderator, sizeof(project.moderator), "%s", claim_ptr);
                fprintf(stderr, "niger: %s\n", project.moderator);

                if (addproject(&project) == 0) {
                    cJSON* response_json = cJSON_CreateObject();
                    cJSON_AddStringToObject(response_json, "moderator", project.moderator);
                    cJSON_AddStringToObject(response_json, "project", project.project);

                    cJSON* members_array = cJSON_CreateArray();
                    for (int i = 0; project.members[i][0] != '\0' && i < 50; i++) {
                        cJSON_AddItemToArray(members_array, cJSON_CreateString(project.members[i]));
                    }
                    cJSON_AddItemToObject(response_json, "members", members_array);

                    char* response_str = cJSON_PrintUnformatted(response_json);

                    struct MHD_Response* response = MHD_create_response_from_buffer(
                        strlen(response_str),
                        (void*)response_str,
                        MHD_RESPMEM_MUST_FREE
                    );

                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    MHD_add_response_header(response, "Content-Type", "application/json");

                    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                    MHD_destroy_response(response);
                    cJSON_Delete(response_json);
                    return ret;
                }
            }
            else {
                const char* error_response = "Invalid project data";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
            }

            cJSON_Delete(json);
            free(conn_info->json_data);
            free(conn_info);
            *con_cls = NULL;
            return MHD_YES;
        }
    }

    // Handle GET request for projects
    if (strcmp(url, "/projects") == 0 && strcmp(method, "GET") == 0) {
        printf("Handling GET request for /projects\n");
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check permission to read projects
        if (check_permission(&auth, PERM_READ_PROJECTS) != 0) {
            return send_forbidden_response(connection, "Cannot access projects");
        }

        // Get projects based on user role and ID
        char* response_data = get_projects_by_user_role(auth.user_id, auth.role);
        if (response_data == NULL) {
            const char* error_response = "Error fetching projects from database";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "text/plain");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }

        printf("Raw response data: %s\n", response_data);
        printf("Response length: %zu\n", strlen(response_data));

        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(response_data),
            (void*)response_data,
            MHD_RESPMEM_MUST_FREE
        );

        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    // In your answer_to_connection function, update the single project handler:
    if (strncmp(url, "/projects/", 10) == 0 && strcmp(method, "GET") == 0) {
        printf("Handling GET request for single project\n");
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check permission to read projects
        if (check_permission(&auth, PERM_READ_PROJECTS) != 0) {
            return send_forbidden_response(connection, "Cannot access project");
        }

        const char* project_id = url + 10;
        while (*project_id == '/') {
            project_id++;
        }

        printf("Attempting to fetch project with ID: %s\n", project_id);

        if (strlen(project_id) != 24) {
            const char* error_response = "{\"error\": \"Invalid project ID format\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }

        // Check if user has access to this specific project
        if (check_user_project_access(auth.user_id, auth.role, project_id) != 0) {
            const char* error_response = "{\"error\": \"Access denied to this project\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
            MHD_destroy_response(response);
            return ret;
        }

        char* project_data = get_project_by_id(project_id);
        if (project_data == NULL) {
            const char* error_response = "{\"error\": \"Project not found\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
            MHD_destroy_response(response);
            return ret;
        }

        size_t response_len = strlen(project_data);
        struct MHD_Response* response = MHD_create_response_from_buffer(
            response_len,
            (void*)project_data,
            MHD_RESPMEM_MUST_FREE
        );

        if (response == NULL) {
            free(project_data);
            return MHD_NO;
        }

        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // In the PATCH handler
    if (strncmp(url, "/updateproject/", 14) == 0 && strcmp(method, "PATCH") == 0) {
        printf("Handling PATCH request for project update\n");
        printf("URL: %s\n", url);
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check permission to update projects (only MANAGER allowed)
        if (check_permission(&auth, PERM_UPDATE_PROJECTS) != 0) {
            return send_forbidden_response(connection, "Only managers can update projects");
        }

        // Skip the "/updateproject/" prefix and any leading slash
        const char* project_id = url + 14;
        while (*project_id == '/') {
            project_id++;
        }

        printf("Project ID after cleanup: %s\n", project_id);

        if (strlen(project_id) != 24) {
            printf("Invalid project ID length: %zu\n", strlen(project_id));
            const char* error_response = "Invalid project ID length";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            if (conn_info == NULL) {
                printf("Failed to allocate connection info\n");
                return MHD_NO;
            }
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);

        if (*upload_data_size > 0) {
            printf("Receiving data chunk of size: %zu\n", *upload_data_size);
            conn_info->json_data = realloc(conn_info->json_data,
                conn_info->json_size + *upload_data_size + 1);
            if (conn_info->json_data == NULL) {
                printf("Failed to allocate memory for JSON data\n");
                return MHD_NO;
            }
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }

        if (conn_info->json_data == NULL) {
            printf("No JSON data received\n");
            const char* error_response = "No data received";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(conn_info->json_data);
            free(conn_info);
            *con_cls = NULL;
            return ret;
        }

        printf("Received JSON data: %s\n", conn_info->json_data);


        if (*upload_data_size > 0) {
            conn_info->json_data = realloc(conn_info->json_data,
                conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }

        cJSON* json = cJSON_Parse(conn_info->json_data);
        if (json == NULL) {
            const char* error_response = "Invalid JSON format";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }

        cJSON* members = cJSON_GetObjectItem(json, "members");
        if (!members || !cJSON_IsArray(members)) {
            const char* error_response = "Invalid members array";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        // Get current project to compare members
        char* current_project_json = get_project_by_id(project_id);
        if (current_project_json == NULL) {
            const char* error_response = "{\"status\":\"error\",\"message\":\"Project not found\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        cJSON* current_project = cJSON_Parse(current_project_json);
        if (current_project == NULL) {
            const char* error_response = "{\"status\":\"error\",\"message\":\"Failed to parse current project\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            free(current_project_json);
            return ret;
        }

        // Get current members
        cJSON* current_members = cJSON_GetObjectItem(current_project, "members");
        if (!current_members || !cJSON_IsArray(current_members)) {
            const char* error_response = "{\"status\":\"error\",\"message\":\"Invalid current project members\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            cJSON_Delete(current_project);
            free(current_project_json);
            return ret;
        }

        // Prepare new member list
        int new_member_count = cJSON_GetArraySize(members);
        const char** new_member_strings = malloc(new_member_count * sizeof(char*));
        for (int i = 0; i < new_member_count; i++) {
            cJSON* member = cJSON_GetArrayItem(members, i);
            new_member_strings[i] = member->valuestring;
        }

        // Find removed members by comparing current vs new member lists
        int current_member_count = cJSON_GetArraySize(current_members);
        const char** removed_members = malloc(current_member_count * sizeof(char*));
        int removed_count = 0;

        for (int i = 0; i < current_member_count; i++) {
            cJSON* current_member = cJSON_GetArrayItem(current_members, i);
            if (!cJSON_IsString(current_member)) continue;
            
            const char* current_username = current_member->valuestring;
            bool found_in_new = false;
            
            // Check if this current member is still in the new member list
            for (int j = 0; j < new_member_count; j++) {
                if (strcmp(current_username, new_member_strings[j]) == 0) {
                    found_in_new = true;
                    break;
                }
            }
            
            // If not found in new list, this member is being removed
            if (!found_in_new) {
                removed_members[removed_count] = current_username;
                removed_count++;
                printf("Member being removed: %s\n", current_username);
            }
        }

        // Check if any removed members have unfinished tasks
        if (removed_count > 0) {
            printf("Checking %d removed members for unfinished tasks...\n", removed_count);
            int has_unfinished = check_members_unfinished_tasks(project_id, removed_members, removed_count);
            
            if (has_unfinished) {
                const char* error_response = "{\"status\":\"error\",\"message\":\"Cannot remove members who have unfinished tasks\"}";
                struct MHD_Response* response = MHD_create_response_from_buffer(
                    strlen(error_response),
                    (void*)error_response,
                    MHD_RESPMEM_PERSISTENT
                );
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                MHD_add_response_header(response, "Content-Type", "application/json");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                
                // Cleanup
                free(new_member_strings);
                free(removed_members);
                cJSON_Delete(json);
                cJSON_Delete(current_project);
                free(current_project_json);
                return ret;
            }
        }

        // Proceed with the update if validation passed
        int update_result = update_project_members(project_id, new_member_strings, new_member_count);
        
        // Cleanup
        free(new_member_strings);
        free(removed_members);
        cJSON_Delete(current_project);
        free(current_project_json);
        cJSON_Delete(json);

        if (update_result == 0) {
            const char* success_response = "{\"status\":\"success\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(success_response),
                (void*)success_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
        else {
            const char* error_response = "{\"status\":\"error\",\"message\":\"Failed to update project\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // Handle DELETE request for project deletion
    if (strncmp(url, "/deleteproject/", 14) == 0 && strcmp(method, "DELETE") == 0) {
        printf("Handling DELETE request for project deletion\n");
        printf("URL: %s\n", url);
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check permission to delete projects (only MANAGER allowed)
        if (check_permission(&auth, PERM_DELETE_PROJECTS) != 0) {
            return send_forbidden_response(connection, "Only managers can delete projects");
        }

        // Skip the "/deleteproject/" prefix and any leading slash
        const char* project_id = url + 14;
        while (*project_id == '/') {
            project_id++;
        }

        printf("Project ID to delete: %s\n", project_id);

        if (strlen(project_id) != 24) {
            printf("Invalid project ID length: %zu\n", strlen(project_id));
            const char* error_response = "{\"status\":\"error\",\"message\":\"Invalid project ID length\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }

        // Verify user has access to this specific project
        if (check_user_project_access(auth.user_id, auth.role, project_id) != 0) {
            return send_forbidden_response(connection, "Access denied to this project");
        }

        // Attempt to delete the project (includes task completion validation)
        int delete_result = delete_project(project_id);
        
        if (delete_result == 0) {
            const char* success_response = "{\"status\":\"success\",\"message\":\"Project deleted successfully\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(success_response),
                (void*)success_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
        else {
            const char* error_response = "{\"status\":\"error\",\"message\":\"Cannot delete project - it has unfinished tasks or an error occurred\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // Handle 404 for unmatched routes
    const char* not_found = "404 - Not Found";
    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
        (void*)not_found, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {

    const char *var_name = "HMAC_KEY";
    char *hmac_key = getenv(var_name);
    if (!hmac_key) {
        fprintf(stderr, "Cannot find HMAC key\n");
        return 1;
    }

    struct MHD_Daemon* daemon;

    char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
        &answer_to_connection, NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d\n", port);
    repo();

    pause();

    MHD_stop_daemon(daemon);
    return 0;
}