#ifndef NOTIFICATION_HELPER_H
#define NOTIFICATION_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

// Function to send notification to notification service
int send_notification(const char* user_id, const char* type, const char* title, 
                     const char* message, const char* metadata);

// Helper functions for project member notifications
int send_member_added_notification(const char* user_id, const char* project_name, const char* project_id);
int send_member_removed_notification(const char* user_id, const char* project_name, const char* project_id);

#ifdef __cplusplus
}
#endif

#endif
