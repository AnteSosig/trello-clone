#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"

#define PORT 8082

// Structure to store incoming JSON data
struct ConnectionInfo {
    char* json_data;
    size_t json_size;
};

static enum MHD_Result answer_to_connection(void* cls, struct MHD_Connection* connection,
    const char* url, const char* method, const char* version,
    const char* upload_data, size_t* upload_data_size, void** con_cls) {

    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response* response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    if (*con_cls == NULL) {
        struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
        if (!conn_info) return MHD_NO;
        *con_cls = (void*)conn_info;
        return MHD_YES;
    }

    struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);

    if (*upload_data_size > 0) {
        char* new_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
        if (!new_data) {
            free(conn_info->json_data);
            free(conn_info);
            return MHD_NO;
        }
        conn_info->json_data = new_data;
        if (conn_info->json_data) {
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';
        }
        *upload_data_size = 0;
        return MHD_YES;
    }

    // Handle task creation
    if (strcmp(url, "/tasks") == 0 && strcmp(method, "POST") == 0) {
        printf("Handling POST request for /tasks\n");

        if (!conn_info->json_data) {
            const char* error_response = "{\"error\": \"No data received\"}";
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

        cJSON* json = cJSON_Parse(conn_info->json_data);
        if (!json) {
            const char* error_response = "{\"error\": \"Invalid JSON format\"}";
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

        Task task;
        if (parse_task_from_json(json, &task) == 0) {
            if (add_task(&task) == 0) {
                const char* success_response = "{\"status\": \"success\"}";
                struct MHD_Response* response = MHD_create_response_from_buffer(
                    strlen(success_response),
                    (void*)success_response,
                    MHD_RESPMEM_PERSISTENT
                );
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                MHD_add_response_header(response, "Content-Type", "application/json");
                int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                return ret;
            }
        }

        const char* error_response = "{\"error\": \"Failed to create task\"}";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(error_response),
            (void*)error_response,
            MHD_RESPMEM_PERSISTENT
        );
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        cJSON_Delete(json);
        return ret;
    }

    // Get tasks by project
    if (strncmp(url, "/tasks/project/", 14) == 0 && strcmp(method, "GET") == 0) {
        const char* project_id = url + 14;  // Skip "/tasks/project/"
        
        // Skip any leading forward slash
        while (*project_id == '/') {
            project_id++;
        }
        
        printf("Extracted project_id: %s\n", project_id);
        
        char* tasks = get_tasks_by_project(project_id);
        
        if (tasks) {
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(tasks),
                (void*)tasks,
                MHD_RESPMEM_MUST_FREE
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
        
        const char* error_response = "{\"error\": \"Failed to fetch tasks\"}";
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

    // Update task status
    if (strncmp(url, "/tasks/status/", 13) == 0 && strcmp(method, "PATCH") == 0) {
        const char* task_id = url + 13;  // Skip "/tasks/status/"
        // Skip any leading forward slash
        while (*task_id == '/') {
            task_id++;
        }
        printf("Processing status update for task ID: %s\n", task_id);

        if (!conn_info->json_data) {
            const char* error_response = "{\"error\": \"No data received\"}";
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

        cJSON* json = cJSON_Parse(conn_info->json_data);
        if (!json) {
            const char* error_response = "{\"error\": \"Invalid JSON format\"}";
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

        cJSON* status_json = cJSON_GetObjectItem(json, "status");
        if (!status_json || !cJSON_IsString(status_json)) {
            const char* error_response = "{\"error\": \"Invalid status value\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        TaskStatus new_status;
        if (strcmp(status_json->valuestring, "pending") == 0) {
            new_status = STATUS_PENDING;
        }
        else if (strcmp(status_json->valuestring, "in_progress") == 0) {
            new_status = STATUS_IN_PROGRESS;
        }
        else if (strcmp(status_json->valuestring, "completed") == 0) {
            new_status = STATUS_COMPLETED;
        }
        else {
            const char* error_response = "{\"error\": \"Invalid status value\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        if (update_task_status(task_id, new_status) == 0) {
            const char* success_response = "{\"status\": \"success\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(success_response),
                (void*)success_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        const char* error_response = "{\"error\": \"Failed to update task status\"}";
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
        return ret;
    }

    // Update task members
    if (strncmp(url, "/tasks/members/", 14) == 0 && strcmp(method, "PATCH") == 0) {
        const char* task_id = url + 14;  // Skip "/tasks/members/"
        // Skip any leading forward slash
        while (*task_id == '/') {
            task_id++;
        }
        printf("Processing members update for task ID: %s\n", task_id);

        if (!conn_info->json_data) {
            const char* error_response = "{\"error\": \"No data received\"}";
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

        cJSON* json = cJSON_Parse(conn_info->json_data);
        if (!json) {
            const char* error_response = "{\"error\": \"Invalid JSON format\"}";
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

        cJSON* members_array = cJSON_GetObjectItem(json, "members");
        if (!members_array || !cJSON_IsArray(members_array)) {
            const char* error_response = "{\"error\": \"Invalid members array\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(error_response),
                (void*)error_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        int member_count = cJSON_GetArraySize(members_array);
        const char** members = malloc(member_count * sizeof(char*));
        for (int i = 0; i < member_count; i++) {
            cJSON* member = cJSON_GetArrayItem(members_array, i);
            if (cJSON_IsString(member)) {
                members[i] = member->valuestring;
            }
        }

        if (update_task_members(task_id, members, member_count) == 0) {
            const char* success_response = "{\"status\": \"success\"}";
            struct MHD_Response* response = MHD_create_response_from_buffer(
                strlen(success_response),
                (void*)success_response,
                MHD_RESPMEM_PERSISTENT
            );
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            free(members);
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            cJSON_Delete(json);
            return ret;
        }

        free(members);
        const char* error_response = "{\"error\": \"Failed to update task members\"}";
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
        return ret;
    }

    // Handle 404 for unmatched routes
    const char* not_found = "404 - Not Found";
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(not_found),
        (void*)not_found,
        MHD_RESPMEM_PERSISTENT
    );
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    struct MHD_Daemon* daemon;
    
    char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;

    if (repo() != 0) {
        printf("Failed to initialize repository\n");
        return 1;
    }

    daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY,
        port,
        NULL,
        NULL,
        &answer_to_connection,
        NULL,
        MHD_OPTION_END
    );

    if (NULL == daemon) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d\n", port);
    printf("Press Enter to stop the server...\n");
    (void)getchar();  // Explicitly ignore return value

    MHD_stop_daemon(daemon);
    return 0;
}