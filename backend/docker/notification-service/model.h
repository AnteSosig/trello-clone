#ifndef MODEL_H
#define MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cjson/cJSON.h>
#include <time.h>

#define MAX_STRING_LENGTH 100
#define MAX_MESSAGE_LENGTH 500
#define MAX_METADATA_LENGTH 1000

// Notification types for the different triggers
typedef enum {
    NOTIFICATION_USER_REGISTERED = 1,
    NOTIFICATION_PROJECT_CREATED = 2,
    NOTIFICATION_TASK_ASSIGNED = 3,
    NOTIFICATION_TASK_STATUS_CHANGED = 4,
    NOTIFICATION_PROJECT_MEMBER_ADDED = 5,
    NOTIFICATION_PROJECT_MEMBER_REMOVED = 6,
    NOTIFICATION_TASK_MEMBER_ADDED = 7,
    NOTIFICATION_TASK_MEMBER_REMOVED = 8
} NotificationType;

// Notification status
typedef enum {
    NOTIFICATION_UNREAD = 0,
    NOTIFICATION_READ = 1
} NotificationStatus;

typedef struct Notification {
    char notification_id[MAX_STRING_LENGTH];
    char user_id[MAX_STRING_LENGTH];          // Who receives the notification
    NotificationType type;
    char title[MAX_STRING_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
    char metadata[MAX_METADATA_LENGTH];       // JSON string with additional data
    NotificationStatus status;
    time_t created_at;
    time_t read_at;                          // 0 if not read yet
} Notification;

// Function declarations
int parse_notification_from_json(const cJSON* json, Notification* notification);
cJSON* notification_to_json(const Notification* notification);
const char* notification_type_to_string(NotificationType type);
NotificationType notification_type_from_string(const char* type_str);

#ifdef __cplusplus
}
#endif

#endif
