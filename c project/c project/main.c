#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"

#define PORT 8080

// Structure to store incoming JSON data
struct ConnectionInfo {
    char* json_data;
    size_t json_size;
};

int answer_to_connection(void* cls, struct MHD_Connection* connection,
    const char* url, const char* method, const char* version,
    const char* upload_data, size_t* upload_data_size, void** con_cls) {

    // Handle "/newproject" POST request
    if (strcmp(url, "/newproject") == 0 && strcmp(method, "POST") == 0) {

        printf("Handling POST request for /newproject\n");

        if (*con_cls == NULL) {
            // Allocate memory for incoming data storage
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);

        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
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

            // Assuming the parsed data is for a Project struct
            Project project;
            if (parse_project_from_json(json, &project) == 0) {
                // Call your addproject function (adjust it as necessary)
                if (addproject(&project) == 0) {
                    const char* response_str = "Project data received";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
                        (void*)response_str, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                    MHD_destroy_response(response);
                }
                else {
                    const char* error_response = "Project not added";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                        (void*)error_response, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                    MHD_destroy_response(response);
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

            // Clean up
            cJSON_Delete(json);
            free(conn_info->json_data);
            free(conn_info);
            *con_cls = NULL;
            return MHD_YES;
        }
    }
    // Handle GET request for /projects
    // Handle GET request for /projects
    // Handle GET request for /projects
    // Handle GET request for /projects
    if (strcmp(url, "/projects") == 0 && strcmp(method, "GET") == 0) {
        printf("Handling GET request for /projects\n");

        // Fetch projects from your data source (e.g., MongoDB or local data)
        cJSON* projects = cJSON_CreateArray();

        // Example project data, replace this with actual MongoDB fetching logic
        cJSON* project = cJSON_CreateObject();
        cJSON_AddStringToObject(project, "project", "Project A");
        cJSON_AddStringToObject(project, "moderator", "Moderator A");
        cJSON_AddStringToObject(project, "members", "Member 1, Member 2");
        cJSON_AddItemToArray(projects, project);

        project = cJSON_CreateObject();
        cJSON_AddStringToObject(project, "project", "Project B");
        cJSON_AddStringToObject(project, "moderator", "Moderator B");
        cJSON_AddStringToObject(project, "members", "Member 3, Member 4");
        cJSON_AddItemToArray(projects, project);

        // Convert the projects array to a JSON string
        char* response_data = cJSON_PrintUnformatted(projects);

        // Log the raw response for debugging
        printf("Raw response data: %s\n", response_data);  // Log the raw string data

        // Ensure it's a valid string before proceeding
        if (response_data == NULL) {
            const char* error_response = "Error generating JSON response";
            struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                (void*)error_response, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "text/plain");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }

        // Double-check the content length and print to ensure correct string length
        printf("Response length: %zu\n", strlen(response_data));

        // Check if the content is a valid JSON string
        if (strlen(response_data) == 0) {
            const char* error_response = "Empty response generated";
            struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                (void*)error_response, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "text/plain");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }

        // Create a response
        struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_data),
            (void*)response_data, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");  // Set the content type

        // Queue the response
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);

        // Clean up
        free(response_data);
        cJSON_Delete(projects);
        return ret;
    }






    // Handle other routes or return 404
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

    // Set the PORT from environment variable, fallback to default PORT 8080
    char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;

    // Start the HTTP server
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
        &answer_to_connection, NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d\n", port);
    repo();

    // Run indefinitely (could also add logic to handle graceful shutdowns)
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
