#include "model.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

int parse_notification_from_json(const cJSON* json, Notification* notification) {
    // Clear the structure first
    memset(notification, 0, sizeof(Notification));

    // Get JSON objects
    cJSON* user_id = cJSON_GetObjectItem(json, "user_id");
    cJSON* type = cJSON_GetObjectItem(json, "type");
    cJSON* title = cJSON_GetObjectItem(json, "title");
    cJSON* message = cJSON_GetObjectItem(json, "message");
    cJSON* metadata = cJSON_GetObjectItem(json, "metadata");

    // Debug print the received values
    printf("Received notification JSON:\n");
    printf("User ID: %s\n", user_id ? user_id->valuestring : "NULL");
    printf("Type: %s\n", type ? type->valuestring : "NULL");
    printf("Title: %s\n", title ? title->valuestring : "NULL");
    printf("Message: %s\n", message ? message->valuestring : "NULL");

    // Validate required fields
    if (!user_id || !type || !title || !message ||
        !cJSON_IsString(user_id) || !cJSON_IsString(type) ||
        !cJSON_IsString(title) || !cJSON_IsString(message)) {
        printf("Missing or invalid required fields\n");
        return 1;
    }

    // Validate string lengths
    if (strlen(user_id->valuestring) >= MAX_STRING_LENGTH) {
        printf("User ID string too long: %zu chars (max: %d)\n",
            strlen(user_id->valuestring), MAX_STRING_LENGTH);
        return 1;
    }
    if (strlen(title->valuestring) >= MAX_STRING_LENGTH) {
        printf("Title string too long: %zu chars (max: %d)\n",
            strlen(title->valuestring), MAX_STRING_LENGTH);
        return 1;
    }
    if (strlen(message->valuestring) >= MAX_MESSAGE_LENGTH) {
        printf("Message string too long: %zu chars (max: %d)\n",
            strlen(message->valuestring), MAX_MESSAGE_LENGTH);
        return 1;
    }

    // Copy basic fields
    strcpy(notification->user_id, user_id->valuestring);
    strcpy(notification->title, title->valuestring);
    strcpy(notification->message, message->valuestring);
    
    // Parse notification type
    notification->type = notification_type_from_string(type->valuestring);
    if (notification->type == 0) {
        printf("Invalid notification type: %s\n", type->valuestring);
        return 1;
    }

    // Handle optional metadata
    if (metadata && cJSON_IsString(metadata)) {
        if (strlen(metadata->valuestring) < MAX_METADATA_LENGTH) {
            strcpy(notification->metadata, metadata->valuestring);
        } else {
            printf("Metadata string too long: %zu chars (max: %d)\n",
                strlen(metadata->valuestring), MAX_METADATA_LENGTH);
            return 1;
        }
    }

    // Set default values
    notification->status = NOTIFICATION_UNREAD;
    notification->created_at = time(NULL);
    notification->read_at = 0;

    printf("Successfully parsed notification\n");
    return 0;
}

cJSON* notification_to_json(const Notification* notification) {
    cJSON* json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "notification_id", notification->notification_id);
    cJSON_AddStringToObject(json, "user_id", notification->user_id);
    cJSON_AddStringToObject(json, "type", notification_type_to_string(notification->type));
    cJSON_AddStringToObject(json, "title", notification->title);
    cJSON_AddStringToObject(json, "message", notification->message);
    cJSON_AddStringToObject(json, "metadata", notification->metadata);
    cJSON_AddNumberToObject(json, "status", notification->status);
    cJSON_AddNumberToObject(json, "created_at", (double)notification->created_at);
    
    if (notification->read_at > 0) {
        cJSON_AddNumberToObject(json, "read_at", (double)notification->read_at);
    }
    
    return json;
}

const char* notification_type_to_string(NotificationType type) {
    switch (type) {
        case NOTIFICATION_USER_REGISTERED:
            return "USER_REGISTERED";
        case NOTIFICATION_PROJECT_CREATED:
            return "PROJECT_CREATED";
        case NOTIFICATION_TASK_ASSIGNED:
            return "TASK_ASSIGNED";
        case NOTIFICATION_TASK_STATUS_CHANGED:
            return "TASK_STATUS_CHANGED";
        case NOTIFICATION_PROJECT_MEMBER_ADDED:
            return "PROJECT_MEMBER_ADDED";
        case NOTIFICATION_PROJECT_MEMBER_REMOVED:
            return "PROJECT_MEMBER_REMOVED";
        case NOTIFICATION_TASK_MEMBER_ADDED:
            return "TASK_MEMBER_ADDED";
        case NOTIFICATION_TASK_MEMBER_REMOVED:
            return "TASK_MEMBER_REMOVED";
        default:
            return "UNKNOWN";
    }
}

NotificationType notification_type_from_string(const char* type_str) {
    if (strcmp(type_str, "USER_REGISTERED") == 0) {
        return NOTIFICATION_USER_REGISTERED;
    } else if (strcmp(type_str, "PROJECT_CREATED") == 0) {
        return NOTIFICATION_PROJECT_CREATED;
    } else if (strcmp(type_str, "TASK_ASSIGNED") == 0) {
        return NOTIFICATION_TASK_ASSIGNED;
    } else if (strcmp(type_str, "TASK_STATUS_CHANGED") == 0) {
        return NOTIFICATION_TASK_STATUS_CHANGED;
    } else if (strcmp(type_str, "PROJECT_MEMBER_ADDED") == 0) {
        return NOTIFICATION_PROJECT_MEMBER_ADDED;
    } else if (strcmp(type_str, "PROJECT_MEMBER_REMOVED") == 0) {
        return NOTIFICATION_PROJECT_MEMBER_REMOVED;
    } else if (strcmp(type_str, "TASK_MEMBER_ADDED") == 0) {
        return NOTIFICATION_TASK_MEMBER_ADDED;
    } else if (strcmp(type_str, "TASK_MEMBER_REMOVED") == 0) {
        return NOTIFICATION_TASK_MEMBER_REMOVED;
    }
    return 0; // Invalid type
}
