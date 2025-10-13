#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"
#include "jwt_middleware.h"

#define PORT 8083

// Global repository instance
static NotificationRepository* repo = NULL;

// Structure to store incoming JSON data
struct ConnectionInfo {
    char* json_data;
    size_t json_size;
};

static enum MHD_Result answer_to_connection(void* cls, struct MHD_Connection* connection,
    const char* url, const char* method, const char* version,
    const char* upload_data, size_t* upload_data_size, void** con_cls) {

    printf("Incoming request: %s %s\n", method, url);

    // Handle OPTIONS requests for CORS
    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response* response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
        MHD_add_response_header(response, "Access-Control-Max-Age", "86400");
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

    // Handle incoming data for POST/PATCH requests
    if (*upload_data_size > 0) {
        char* new_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
        if (!new_data) {
            return MHD_NO;
        }
        conn_info->json_data = new_data;
        memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
        conn_info->json_size += *upload_data_size;
        conn_info->json_data[conn_info->json_size] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }

    // Route handling
    struct MHD_Response* response = NULL;
    int status_code = MHD_HTTP_NOT_FOUND;
    char* response_data = NULL;

    // POST /notifications - Create a new notification
    if (strcmp(url, "/notifications") == 0 && strcmp(method, "POST") == 0) {
        printf("Handling POST request for /notifications\n");
        
        // Check if this is an internal service request (no auth header = internal service)
        const char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        bool is_internal_request = (auth_header == NULL);
        
        if (!is_internal_request) {
            // Authenticate external requests
            AuthContext auth;
            if (authenticate_request(connection, &auth) != 0) {
                status_code = MHD_HTTP_UNAUTHORIZED;
                response_data = create_error_json("Unauthorized", auth.error_message);
                goto create_response;
            }
            
            // Check permission to create notifications (only MANAGER allowed)
            if (check_permission(&auth, PERM_CREATE_NOTIFICATIONS) != 0) {
                status_code = MHD_HTTP_FORBIDDEN;
                response_data = create_error_json("Forbidden", "Only managers can create notifications");
                goto create_response;
            }
        } else {
            printf("Internal service request detected - skipping authentication\n");
        }

        if (!conn_info->json_data) {
            status_code = MHD_HTTP_BAD_REQUEST;
            response_data = create_error_json("Bad Request", "No JSON data provided");
            goto create_response;
        }

        cJSON* json = cJSON_Parse(conn_info->json_data);
        if (!json) {
            status_code = MHD_HTTP_BAD_REQUEST;
            response_data = create_error_json("Bad Request", "Invalid JSON format");
            goto create_response;
        }

        Notification notification;
        if (parse_notification_from_json(json, &notification) != 0) {
            status_code = MHD_HTTP_BAD_REQUEST;
            response_data = create_error_json("Bad Request", "Invalid notification data");
            cJSON_Delete(json);
            goto create_response;
        }

        if (add_notification(repo, &notification) != 0) {
            status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            response_data = create_error_json("Internal Server Error", "Failed to create notification");
            cJSON_Delete(json);
            goto create_response;
        }

        cJSON* response_json = notification_to_json(&notification);
        response_data = cJSON_Print(response_json);
        status_code = MHD_HTTP_CREATED;

        cJSON_Delete(json);
        cJSON_Delete(response_json);
    }
    // GET /notifications?user_id=xxx - Get notifications for a user
    else if (strcmp(url, "/notifications") == 0 && strcmp(method, "GET") == 0) {
        printf("Handling GET request for /notifications\n");
        
        // Debug: Check Authorization header
        const char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        printf("DEBUG: Authorization header: %s\n", auth_header ? auth_header : "NULL");
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            printf("DEBUG: Authentication failed: %s\n", auth.error_message);
            status_code = MHD_HTTP_UNAUTHORIZED;
            response_data = create_error_json("Unauthorized", auth.error_message);
            goto create_response;
        }

        printf("DEBUG: Authentication successful - User: %s, Role: %s\n", auth.user_id, auth.role);

        const char* user_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user_id");
        const char* unread_only = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "unread_only");

        // If no user_id is provided, default to authenticated user's ID
        if (!user_id) {
            user_id = auth.user_id;
        }

        // Users can only access their own notifications, managers can access any
        if (strcmp(auth.role, "USER") == 0 && strcmp(user_id, auth.user_id) != 0) {
            status_code = MHD_HTTP_FORBIDDEN;
            response_data = create_error_json("Forbidden", "Users can only access their own notifications");
            goto create_response;
        }

        printf("DEBUG: Querying notifications for user: %s, unread_only: %s\n", user_id, unread_only ? unread_only : "false");

        if (unread_only && strcmp(unread_only, "true") == 0) {
            printf("DEBUG: Calling get_unread_notifications_by_user\n");
            response_data = get_unread_notifications_by_user(repo, user_id);
        } else {
            printf("DEBUG: Calling get_notifications_by_user\n");
            response_data = get_notifications_by_user(repo, user_id);
        }

        if (!response_data) {
            printf("DEBUG: Database query returned NULL, returning empty array\n");
            status_code = MHD_HTTP_OK;
            response_data = strdup("[]");
        } else {
            printf("DEBUG: Database query successful, response length: %zu\n", strlen(response_data));
            printf("DEBUG: Response content: %s\n", response_data); // Show full response
            status_code = MHD_HTTP_OK;
        }
    }
    // PATCH /notifications/{id}/read - Mark notification as read
    else if (strncmp(url, "/notifications/", 15) == 0 && strstr(url, "/read") != NULL && strcmp(method, "PATCH") == 0) {
        printf("Handling PATCH request for marking notification as read\n");
        
        // Authenticate the request
        AuthContext auth;
        if (authenticate_request(connection, &auth) != 0) {
            status_code = MHD_HTTP_UNAUTHORIZED;
            response_data = create_error_json("Unauthorized", auth.error_message);
            goto create_response;
        }

        // Extract notification ID from URL
        char* notification_id = malloc(256);
        if (!notification_id) {
            status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            response_data = create_error_json("Internal Server Error", "Memory allocation failed");
            goto create_response;
        }

        // Parse URL to get notification ID (format: /notifications/{id}/read)
        const char* id_start = url + 15; // Skip "/notifications/"
        const char* id_end = strstr(id_start, "/read");
        if (!id_end || id_end <= id_start) {
            free(notification_id);
            status_code = MHD_HTTP_BAD_REQUEST;
            response_data = create_error_json("Bad Request", "Invalid notification ID in URL");
            goto create_response;
        }

        size_t id_len = id_end - id_start;
        if (id_len >= 256) {
            free(notification_id);
            status_code = MHD_HTTP_BAD_REQUEST;
            response_data = create_error_json("Bad Request", "Notification ID too long");
            goto create_response;
        }

        strncpy(notification_id, id_start, id_len);
        notification_id[id_len] = '\0';

        if (mark_notification_as_read(repo, notification_id) != 0) {
            status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            response_data = create_error_json("Internal Server Error", "Failed to mark notification as read");
        } else {
            status_code = MHD_HTTP_OK;
            response_data = strdup("{\"message\":\"Notification marked as read successfully\"}");
        }

        free(notification_id);
    }
    // Health check endpoint
    else if (strcmp(url, "/health") == 0 && strcmp(method, "GET") == 0) {
        status_code = MHD_HTTP_OK;
        response_data = strdup("{\"status\":\"healthy\",\"service\":\"notification-service\"}");
    }
    else {
        status_code = MHD_HTTP_NOT_FOUND;
        response_data = create_error_json("Not Found", "Endpoint not found");
    }

create_response:
    if (!response_data) {
        response_data = create_error_json("Internal Server Error", "Unknown error occurred");
        status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }

    response = MHD_create_response_from_buffer(strlen(response_data), response_data, MHD_RESPMEM_MUST_COPY);
    
    // Add CORS headers
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Content-Type", "application/json");
    
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    free(response_data);

    return ret;
}

static void request_completed(void* cls, struct MHD_Connection* connection, void** con_cls, enum MHD_RequestTerminationCode toe) {
    struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
    
    if (conn_info) {
        if (conn_info->json_data) {
            free(conn_info->json_data);
        }
        free(conn_info);
    }
    
    *con_cls = NULL;
}

int main() {
    // Force flush stdout immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    printf("=== NOTIFICATION SERVICE STARTING ===\n");
    
    // Get port from environment
    const char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;
    
    printf("Starting notification service on port %d\n", port);

    // Initialize repository
    FILE* logger = fopen("notification_service.log", "w");
    if (!logger) {
        printf("Warning: Could not open log file\n");
        logger = stdout;
    }

    repo = notification_repo_new(logger);
    if (!repo) {
        fprintf(stderr, "Failed to initialize notification repository\n");
        if (logger != stdout) fclose(logger);
        return 1;
    }

    // Start HTTP server
    struct MHD_Daemon* daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY,
        port,
        NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_END
    );

    if (!daemon) {
        fprintf(stderr, "Failed to start HTTP server\n");
        notification_repo_destroy(repo);
        if (logger != stdout) fclose(logger);
        return 1;
    }

    printf("Notification service started successfully on port %d\n", port);
    printf("Server running...\n");
    
    // Keep the server running
    while (1) {
        sleep(1);
    }

    MHD_stop_daemon(daemon);
    notification_repo_destroy(repo);
    if (logger != stdout) fclose(logger);
    
    printf("Notification service stopped\n");
    return 0;
}
