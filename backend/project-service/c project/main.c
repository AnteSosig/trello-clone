#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, OPTIONS");
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

        int member_count = cJSON_GetArraySize(members);
        const char** member_strings = malloc(member_count * sizeof(char*));
        for (int i = 0; i < member_count; i++) {
            cJSON* member = cJSON_GetArrayItem(members, i);
            member_strings[i] = member->valuestring;
        }

        int update_result = update_project_members(project_id, member_strings, member_count);
        free(member_strings);
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

    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}