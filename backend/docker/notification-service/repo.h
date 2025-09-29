#ifndef REPO_H
#define REPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "model.h"
#include <cassandra.h>

// Repository structure for Cassandra
typedef struct {
    CassSession* session;
    CassCluster* cluster;
    FILE* logger;
} NotificationRepository;

// Repository functions
NotificationRepository* notification_repo_new(FILE* logger);
void notification_repo_destroy(NotificationRepository* repo);

// CRUD operations
int add_notification(NotificationRepository* repo, Notification* notification);
char* get_notifications_by_user(NotificationRepository* repo, const char* user_id);
char* get_unread_notifications_by_user(NotificationRepository* repo, const char* user_id);
int mark_notification_as_read(NotificationRepository* repo, const char* notification_id);
int delete_notification(NotificationRepository* repo, const char* notification_id);
int get_notification_count_by_user(NotificationRepository* repo, const char* user_id);

// Helper functions
int create_keyspace_and_table(NotificationRepository* repo);
char* generate_notification_id(void);

#ifdef __cplusplus
}
#endif

#endif
