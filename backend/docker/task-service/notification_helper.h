#ifndef NOTIFICATION_HELPER_H
#define NOTIFICATION_HELPER_H

// Function to send a generic notification
int send_notification(const char* user_id, const char* type, const char* title, 
                     const char* message, const char* metadata);

// Function to send notification when a member is added to a task
int send_task_member_added_notification(const char* user_id, const char* task_name, 
                                       const char* task_id, const char* project_id);

// Function to send notification when a member is removed from a task
int send_task_member_removed_notification(const char* user_id, const char* task_name, 
                                         const char* task_id, const char* project_id);

// Function to send notification when a task status changes
int send_task_status_changed_notification(const char* user_id, const char* task_name, 
                                         const char* task_id, const char* project_id, 
                                         const char* old_status, const char* new_status);

// Function to send notification when a task status changes (with enum parameters)
int send_task_status_changed_notification_with_enum(const char* user_id, const char* task_name, 
                                                    const char* task_id, const char* project_id, 
                                                    int old_status, int new_status);

#endif
