#include "notification_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

// Structure to hold response data
struct NotificationResponse {
    char *memory;
    size_t size;
};

// Callback function to write response data
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct NotificationResponse *mem) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    
    if (!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

int send_notification(const char* user_id, const char* type, const char* title, 
                     const char* message, const char* metadata) {
    CURL *curl;
    CURLcode res;
    struct NotificationResponse response = {0};
    
    // Initialize curl
    curl = curl_easy_init();
    if (!curl) {
        printf("Failed to initialize CURL\n");
        return -1;
    }
    
    // Create JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON *user_id_json = cJSON_CreateString(user_id);
    cJSON *type_json = cJSON_CreateString(type);
    cJSON *title_json = cJSON_CreateString(title);
    cJSON *message_json = cJSON_CreateString(message);
    
    cJSON_AddItemToObject(json, "user_id", user_id_json);
    cJSON_AddItemToObject(json, "type", type_json);
    cJSON_AddItemToObject(json, "title", title_json);
    cJSON_AddItemToObject(json, "message", message_json);
    
    if (metadata && strlen(metadata) > 0) {
        cJSON *metadata_json = cJSON_CreateString(metadata);
        cJSON_AddItemToObject(json, "metadata", metadata_json);
    }
    
    char *json_string = cJSON_Print(json);
    
    // Get notification service URL from environment
    const char* notification_host = getenv("NOTIFICATION_HOST");
    if (!notification_host) {
        notification_host = "notification-service"; // Default Docker service name
    }
    
    const char* notification_port = getenv("NOTIFICATION_PORT");
    if (!notification_port) {
        notification_port = "8083"; // Default port
    }
    
    // Build URL
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%s/notifications", notification_host, notification_port);
    
    printf("=== PROJECT SERVICE: Sending notification ===\n");
    printf("URL: %s\n", url);
    printf("JSON Payload: %s\n", json_string);
    printf("=== END PROJECT SERVICE NOTIFICATION ===\n");
    
    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    // Add Authorization header with manager token if available
    const char* hmac_key = getenv("HMAC_KEY");
    if (hmac_key) {
        // For now, we'll skip JWT generation and assume the notification service
        // will accept requests from internal services without authentication
        // In production, you'd want to generate a service-to-service JWT here
    }
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    // Check response
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (res != CURLE_OK) {
        printf("CURL request failed: %s\n", curl_easy_strerror(res));
    } else if (response_code >= 200 && response_code < 300) {
        printf("Notification sent successfully (HTTP %ld)\n", response_code);
    } else {
        printf("Notification request failed with HTTP %ld\n", response_code);
        if (response.memory) {
            printf("Response: %s\n", response.memory);
        }
    }
    
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    cJSON_Delete(json);
    free(json_string);
    if (response.memory) {
        free(response.memory);
    }
    
    return (res == CURLE_OK && response_code >= 200 && response_code < 300) ? 0 : -1;
}

int send_member_added_notification(const char* user_id, const char* project_name, const char* project_id) {
    char title[256];
    char message[512];
    char metadata[256];
    
    snprintf(title, sizeof(title), "Added to Project");
    snprintf(message, sizeof(message), "You have been added to the project '%s'", project_name);
    snprintf(metadata, sizeof(metadata), "{\"project_id\":\"%s\",\"project_name\":\"%s\"}", project_id, project_name);
    
    return send_notification(user_id, "PROJECT_MEMBER_ADDED", title, message, metadata);
}

int send_member_removed_notification(const char* user_id, const char* project_name, const char* project_id) {
    char title[256];
    char message[512];
    char metadata[256];
    
    snprintf(title, sizeof(title), "Removed from Project");
    snprintf(message, sizeof(message), "You have been removed from the project '%s'", project_name);
    snprintf(metadata, sizeof(metadata), "{\"project_id\":\"%s\",\"project_name\":\"%s\"}", project_id, project_name);
    
    return send_notification(user_id, "PROJECT_MEMBER_REMOVED", title, message, metadata);
}

int send_project_created_notification(const char* user_id, const char* project_name, const char* project_id) {
    char title[256];
    char message[512];
    char metadata[256];
    
    snprintf(title, sizeof(title), "Added to New Project");
    snprintf(message, sizeof(message), "You have been added to the new project '%s'", project_name);
    snprintf(metadata, sizeof(metadata), "{\"project_id\":\"%s\",\"project_name\":\"%s\"}", project_id, project_name);
    
    return send_notification(user_id, "PROJECT_CREATED", title, message, metadata);
}
