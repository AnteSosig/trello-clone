#include <cassandra.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uuid/uuid.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"

NotificationRepository* notification_repo_new(FILE* logger) {
    const char *hosts_var = "CASSANDRA_HOSTS";
    const char *keyspace_var = "CASSANDRA_KEYSPACE";
    
    char *cassandra_hosts = getenv(hosts_var);
    char *keyspace_name = getenv(keyspace_var);
    
    if (cassandra_hosts == NULL) {
        fprintf(logger, "Error: CASSANDRA_HOSTS environment variable is not set\n");
        printf("Error: CASSANDRA_HOSTS environment variable is not set\n");
        return NULL;
    }
    
    if (keyspace_name == NULL) {
        fprintf(logger, "Error: CASSANDRA_KEYSPACE environment variable is not set\n");
        printf("Error: CASSANDRA_KEYSPACE environment variable is not set\n");
        return NULL;
    }

    // Create cluster
    CassCluster* cluster = cass_cluster_new();
    cass_cluster_set_contact_points(cluster, cassandra_hosts);
    cass_cluster_set_port(cluster, 9042);

    // Create session
    CassSession* session = cass_session_new();
    CassFuture* connect_future = cass_session_connect(session, cluster);

    // Wait for connection
    if (cass_future_error_code(connect_future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(connect_future, &message, &message_length);
        fprintf(logger, "Error: Unable to connect to Cassandra: %.*s\n", (int)message_length, message);
        printf("Error: Unable to connect to Cassandra: %.*s\n", (int)message_length, message);
        
        cass_future_free(connect_future);
        cass_session_free(session);
        cass_cluster_free(cluster);
        return NULL;
    }
    
    cass_future_free(connect_future);
    printf("Connected to Cassandra successfully.\n");

    // Allocate repository
    NotificationRepository* repo = (NotificationRepository*)malloc(sizeof(NotificationRepository));
    if (repo == NULL) {
        fprintf(logger, "Error: Failed to allocate memory for repository\n");
        printf("Error: Failed to allocate memory for repository\n");
        cass_session_free(session);
        cass_cluster_free(cluster);
        return NULL;
    }

    repo->session = session;
    repo->cluster = cluster;
    repo->logger = logger;

    // Create keyspace and table
    if (create_keyspace_and_table(repo) != 0) {
        fprintf(logger, "Error: Failed to create keyspace and table\n");
        printf("Error: Failed to create keyspace and table\n");
        notification_repo_destroy(repo);
        return NULL;
    }

    printf("Notification repository initialized successfully.\n");
    return repo;
}

void notification_repo_destroy(NotificationRepository* repo) {
    if (repo) {
        if (repo->session) {
            CassFuture* close_future = cass_session_close(repo->session);
            cass_future_wait(close_future);
            cass_future_free(close_future);
            cass_session_free(repo->session);
        }
        if (repo->cluster) {
            cass_cluster_free(repo->cluster);
        }
        free(repo);
    }
}

int create_keyspace_and_table(NotificationRepository* repo) {
    const char *keyspace_name = getenv("CASSANDRA_KEYSPACE");
    
    // Create keyspace
    char keyspace_query[512];
    snprintf(keyspace_query, sizeof(keyspace_query),
        "CREATE KEYSPACE IF NOT EXISTS %s "
        "WITH REPLICATION = { 'class': 'SimpleStrategy', 'replication_factor': 1 }",
        keyspace_name);

    CassStatement* statement = cass_statement_new(keyspace_query, 0);
    CassFuture* future = cass_session_execute(repo->session, statement);

    if (cass_future_error_code(future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        fprintf(repo->logger, "Error creating keyspace: %.*s\n", (int)message_length, message);
        printf("Error creating keyspace: %.*s\n", (int)message_length, message);
        
        cass_future_free(future);
        cass_statement_free(statement);
        return -1;
    }

    cass_future_free(future);
    cass_statement_free(statement);

    // Use keyspace
    char use_query[256];
    snprintf(use_query, sizeof(use_query), "USE %s", keyspace_name);
    
    statement = cass_statement_new(use_query, 0);
    future = cass_session_execute(repo->session, statement);

    if (cass_future_error_code(future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        fprintf(repo->logger, "Error using keyspace: %.*s\n", (int)message_length, message);
        printf("Error using keyspace: %.*s\n", (int)message_length, message);
        
        cass_future_free(future);
        cass_statement_free(statement);
        return -1;
    }

    cass_future_free(future);
    cass_statement_free(statement);

    // Create table with composite partition key and clustering columns
    // Partition key: (user_id, created_date) - buckets notifications by user and date
    // Clustering columns: created_at (first), notification_id (second) - orders within partition
    const char* table_query = 
        "CREATE TABLE IF NOT EXISTS notifications ("
        "user_id TEXT,"
        "created_date DATE,"
        "created_at TIMESTAMP,"
        "notification_id UUID,"
        "type INT,"
        "title TEXT,"
        "message TEXT,"
        "metadata TEXT,"
        "status INT,"
        "read_at TIMESTAMP,"
        "PRIMARY KEY ((user_id, created_date), created_at, notification_id)"
        ") WITH CLUSTERING ORDER BY (created_at DESC, notification_id ASC)";

    statement = cass_statement_new(table_query, 0);
    future = cass_session_execute(repo->session, statement);

    if (cass_future_error_code(future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        fprintf(repo->logger, "Error creating table: %.*s\n", (int)message_length, message);
        printf("Error creating table: %.*s\n", (int)message_length, message);
        
        cass_future_free(future);
        cass_statement_free(statement);
        return -1;
    }

    cass_future_free(future);
    cass_statement_free(statement);

    printf("Keyspace and table created successfully.\n");
    return 0;
}

char* generate_notification_id(void) {
    uuid_t uuid;
    char* uuid_str = malloc(37); // UUID string length + null terminator
    
    if (uuid_str == NULL) {
        return NULL;
    }
    
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);
    
    return uuid_str;
}

int add_notification(NotificationRepository* repo, Notification* notification) {
    // Generate notification ID
    char* uuid_str = generate_notification_id();
    if (uuid_str == NULL) {
        fprintf(repo->logger, "Error: Failed to generate notification ID\n");
        return -1;
    }
    
    strcpy(notification->notification_id, uuid_str);
    
    // Calculate created_date from created_at (number of days since epoch)
    // Cassandra DATE type is stored as days since epoch (1970-01-01)
    cass_uint32_t created_date = (cass_uint32_t)(notification->created_at / 86400); // 86400 seconds per day
    
    const char* query = 
        "INSERT INTO notifications (user_id, created_date, created_at, notification_id, type, title, message, metadata, status, read_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    CassStatement* statement = cass_statement_new(query, 10);
    
    // Convert UUID string to CassUuid
    CassUuid cass_uuid;
    if (cass_uuid_from_string(uuid_str, &cass_uuid) != CASS_OK) {
        fprintf(repo->logger, "Error: Failed to parse UUID\n");
        free(uuid_str);
        cass_statement_free(statement);
        return -1;
    }
    
    cass_statement_bind_string(statement, 0, notification->user_id);
    cass_statement_bind_uint32(statement, 1, created_date);
    cass_statement_bind_int64(statement, 2, notification->created_at * 1000); // Convert to milliseconds
    cass_statement_bind_uuid(statement, 3, cass_uuid);
    cass_statement_bind_int32(statement, 4, notification->type);
    cass_statement_bind_string(statement, 5, notification->title);
    cass_statement_bind_string(statement, 6, notification->message);
    cass_statement_bind_string(statement, 7, notification->metadata);
    cass_statement_bind_int32(statement, 8, notification->status);
    
    if (notification->read_at > 0) {
        cass_statement_bind_int64(statement, 9, notification->read_at * 1000);
    } else {
        cass_statement_bind_null(statement, 9);
    }

    CassFuture* future = cass_session_execute(repo->session, statement);

    if (cass_future_error_code(future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        fprintf(repo->logger, "Error inserting notification: %.*s\n", (int)message_length, message);
        printf("Error inserting notification: %.*s\n", (int)message_length, message);
        
        cass_future_free(future);
        cass_statement_free(statement);
        free(uuid_str);
        return -1;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    free(uuid_str);

    printf("Notification added successfully with ID: %s\n", notification->notification_id);
    return 0;
}

char* get_notifications_by_user(NotificationRepository* repo, const char* user_id) {
    printf("DEBUG: get_notifications_by_user called for user_id: %s\n", user_id);
    
    if (!repo) {
        printf("DEBUG: Repository is NULL\n");
        return NULL;
    }
    
    if (!repo->session) {
        printf("DEBUG: Cassandra session is NULL\n");
        return NULL;
    }
    
    // Query notifications for the last 90 days
    // Calculate date range in days since epoch
    time_t now = time(NULL);
    cass_uint32_t end_date = (cass_uint32_t)(now / 86400);
    cass_uint32_t start_date = end_date - 90; // 90 days back
    
    const char* query = "SELECT * FROM notifications WHERE user_id = ? AND created_date >= ? AND created_date <= ?";
    
    CassStatement* statement = cass_statement_new(query, 3);
    cass_statement_bind_string(statement, 0, user_id);
    cass_statement_bind_uint32(statement, 1, start_date);
    cass_statement_bind_uint32(statement, 2, end_date);

    printf("DEBUG: Executing Cassandra query for user: '%s' (length: %zu) from date %u to %u\n", 
           user_id, strlen(user_id), start_date, end_date);
    CassFuture* future = cass_session_execute(repo->session, statement);

    if (cass_future_error_code(future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        fprintf(repo->logger, "Error querying notifications: %.*s\n", (int)message_length, message);
        printf("DEBUG: Cassandra query failed: %.*s\n", (int)message_length, message);
        
        cass_future_free(future);
        cass_statement_free(statement);
        return NULL;
    }

    printf("DEBUG: Cassandra query executed successfully\n");

    const CassResult* result = cass_future_get_result(future);
    cJSON* json_array = cJSON_CreateArray();

    printf("DEBUG: Got result from Cassandra\n");
    CassIterator* iterator = cass_iterator_from_result(result);

    int row_count = 0;
    printf("DEBUG: Starting to iterate through results\n");
    while (cass_iterator_next(iterator)) {
        row_count++;
        printf("DEBUG: Processing row %d\n", row_count);
        const CassRow* row = cass_iterator_get_row(iterator);
        Notification notification = {0};
        
        // New column order: user_id, created_date, created_at, notification_id, type, title, message, metadata, status, read_at
        
        // user_id (column 0)
        const char* user_id_val;
        size_t user_id_len;
        cass_value_get_string(cass_row_get_column(row, 0), &user_id_val, &user_id_len);
        strncpy(notification.user_id, user_id_val, user_id_len);
        notification.user_id[user_id_len] = '\0';
        
        // created_date (column 1) - skip, not stored in struct
        
        // created_at (column 2)
        cass_int64_t created_at_ms;
        cass_value_get_int64(cass_row_get_column(row, 2), &created_at_ms);
        notification.created_at = created_at_ms / 1000;
        
        // notification_id (column 3)
        CassUuid uuid;
        cass_value_get_uuid(cass_row_get_column(row, 3), &uuid);
        cass_uuid_string(uuid, notification.notification_id);
        
        // type (column 4)
        cass_value_get_int32(cass_row_get_column(row, 4), (cass_int32_t*)&notification.type);
        
        // title (column 5)
        const char* title_val;
        size_t title_len;
        cass_value_get_string(cass_row_get_column(row, 5), &title_val, &title_len);
        strncpy(notification.title, title_val, title_len);
        notification.title[title_len] = '\0';
        
        // message (column 6)
        const char* message_val;
        size_t message_len;
        cass_value_get_string(cass_row_get_column(row, 6), &message_val, &message_len);
        strncpy(notification.message, message_val, message_len);
        notification.message[message_len] = '\0';
        
        // metadata (column 7)
        const char* metadata_val;
        size_t metadata_len;
        cass_value_get_string(cass_row_get_column(row, 7), &metadata_val, &metadata_len);
        strncpy(notification.metadata, metadata_val, metadata_len);
        notification.metadata[metadata_len] = '\0';
        
        // status (column 8)
        cass_value_get_int32(cass_row_get_column(row, 8), (cass_int32_t*)&notification.status);
        
        // read_at (column 9)
        if (!cass_value_is_null(cass_row_get_column(row, 9))) {
            cass_int64_t read_at_ms;
            cass_value_get_int64(cass_row_get_column(row, 9), &read_at_ms);
            notification.read_at = read_at_ms / 1000;
        }

        cJSON* notification_json = notification_to_json(&notification);
        cJSON_AddItemToArray(json_array, notification_json);
    }

    cass_iterator_free(iterator);
    cass_result_free(result);
    cass_future_free(future);
    cass_statement_free(statement);

    char* json_string = cJSON_Print(json_array);
    cJSON_Delete(json_array);

    // Ensure we always return a valid JSON array, even if empty
    if (!json_string) {
        json_string = strdup("[]");
    }

    return json_string;
}

char* get_unread_notifications_by_user(NotificationRepository* repo, const char* user_id) {
    // Query notifications for the last 90 days
    // Calculate date range in days since epoch
    time_t now = time(NULL);
    cass_uint32_t end_date = (cass_uint32_t)(now / 86400);
    cass_uint32_t start_date = end_date - 90; // 90 days back
    
    const char* query = "SELECT * FROM notifications WHERE user_id = ? AND created_date >= ? AND created_date <= ? AND status = 0 ALLOW FILTERING";
    
    CassStatement* statement = cass_statement_new(query, 3);
    cass_statement_bind_string(statement, 0, user_id);
    cass_statement_bind_uint32(statement, 1, start_date);
    cass_statement_bind_uint32(statement, 2, end_date);

    CassFuture* future = cass_session_execute(repo->session, statement);

    if (cass_future_error_code(future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        fprintf(repo->logger, "Error querying unread notifications: %.*s\n", (int)message_length, message);
        
        cass_future_free(future);
        cass_statement_free(statement);
        return NULL;
    }

    const CassResult* result = cass_future_get_result(future);
    cJSON* json_array = cJSON_CreateArray();

    CassIterator* iterator = cass_iterator_from_result(result);

    while (cass_iterator_next(iterator)) {
        const CassRow* row = cass_iterator_get_row(iterator);
        Notification notification = {0};
        
        // New column order: user_id, created_date, created_at, notification_id, type, title, message, metadata, status, read_at
        
        // user_id (column 0)
        const char* user_id_val;
        size_t user_id_len;
        cass_value_get_string(cass_row_get_column(row, 0), &user_id_val, &user_id_len);
        strncpy(notification.user_id, user_id_val, user_id_len);
        notification.user_id[user_id_len] = '\0';
        
        // created_date (column 1) - skip, not stored in struct
        
        // created_at (column 2)
        cass_int64_t created_at_ms;
        cass_value_get_int64(cass_row_get_column(row, 2), &created_at_ms);
        notification.created_at = created_at_ms / 1000;
        
        // notification_id (column 3)
        CassUuid uuid;
        cass_value_get_uuid(cass_row_get_column(row, 3), &uuid);
        cass_uuid_string(uuid, notification.notification_id);
        
        // type (column 4)
        cass_value_get_int32(cass_row_get_column(row, 4), (cass_int32_t*)&notification.type);
        
        // title (column 5)
        const char* title_val;
        size_t title_len;
        cass_value_get_string(cass_row_get_column(row, 5), &title_val, &title_len);
        strncpy(notification.title, title_val, title_len);
        notification.title[title_len] = '\0';
        
        // message (column 6)
        const char* message_val;
        size_t message_len;
        cass_value_get_string(cass_row_get_column(row, 6), &message_val, &message_len);
        strncpy(notification.message, message_val, message_len);
        notification.message[message_len] = '\0';
        
        // metadata (column 7)
        const char* metadata_val;
        size_t metadata_len;
        cass_value_get_string(cass_row_get_column(row, 7), &metadata_val, &metadata_len);
        strncpy(notification.metadata, metadata_val, metadata_len);
        notification.metadata[metadata_len] = '\0';
        
        // status (column 8)
        cass_value_get_int32(cass_row_get_column(row, 8), (cass_int32_t*)&notification.status);
        
        // read_at (column 9)
        if (!cass_value_is_null(cass_row_get_column(row, 9))) {
            cass_int64_t read_at_ms;
            cass_value_get_int64(cass_row_get_column(row, 9), &read_at_ms);
            notification.read_at = read_at_ms / 1000;
        }

        cJSON* notification_json = notification_to_json(&notification);
        cJSON_AddItemToArray(json_array, notification_json);
    }

    cass_iterator_free(iterator);
    cass_result_free(result);
    cass_future_free(future);
    cass_statement_free(statement);

    char* json_string = cJSON_Print(json_array);
    cJSON_Delete(json_array);

    // Ensure we always return a valid JSON array, even if empty
    if (!json_string) {
        json_string = strdup("[]");
    }

    return json_string;
}

int mark_notification_as_read(NotificationRepository* repo, const char* notification_id) {
    // With the new primary key structure, we need to find the notification first to get all key components
    // Query for the last 90 days to find the notification
    time_t now = time(NULL);
    cass_uint32_t end_date = (cass_uint32_t)(now / 86400);
    cass_uint32_t start_date = end_date - 90; // 90 days back
    
    CassUuid uuid;
    if (cass_uuid_from_string(notification_id, &uuid) != CASS_OK) {
        fprintf(repo->logger, "Error: Invalid notification ID format\n");
        return -1;
    }
    
    // First, find the notification to get the full primary key
    const char* select_query = "SELECT user_id, created_date, created_at FROM notifications WHERE created_date >= ? AND created_date <= ? AND notification_id = ? ALLOW FILTERING";
    
    CassStatement* select_statement = cass_statement_new(select_query, 3);
    cass_statement_bind_uint32(select_statement, 0, start_date);
    cass_statement_bind_uint32(select_statement, 1, end_date);
    cass_statement_bind_uuid(select_statement, 2, uuid);
    
    CassFuture* select_future = cass_session_execute(repo->session, select_statement);
    
    if (cass_future_error_code(select_future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(select_future, &message, &message_length);
        fprintf(repo->logger, "Error finding notification: %.*s\n", (int)message_length, message);
        
        cass_future_free(select_future);
        cass_statement_free(select_statement);
        return -1;
    }
    
    const CassResult* result = cass_future_get_result(select_future);
    CassIterator* iterator = cass_iterator_from_result(result);
    
    if (!cass_iterator_next(iterator)) {
        fprintf(repo->logger, "Error: Notification not found\n");
        cass_iterator_free(iterator);
        cass_result_free(result);
        cass_future_free(select_future);
        cass_statement_free(select_statement);
        return -1;
    }
    
    const CassRow* row = cass_iterator_get_row(iterator);
    
    // Extract primary key components
    const char* user_id_val;
    size_t user_id_len;
    cass_value_get_string(cass_row_get_column(row, 0), &user_id_val, &user_id_len);
    char user_id[256];
    strncpy(user_id, user_id_val, user_id_len);
    user_id[user_id_len] = '\0';
    
    cass_uint32_t created_date;
    cass_value_get_uint32(cass_row_get_column(row, 1), &created_date);
    
    cass_int64_t created_at_ms;
    cass_value_get_int64(cass_row_get_column(row, 2), &created_at_ms);
    
    cass_iterator_free(iterator);
    cass_result_free(result);
    cass_future_free(select_future);
    cass_statement_free(select_statement);
    
    // Now update the notification with the full primary key
    const char* update_query = "UPDATE notifications SET status = 1, read_at = ? WHERE user_id = ? AND created_date = ? AND created_at = ? AND notification_id = ?";
    
    CassStatement* update_statement = cass_statement_new(update_query, 5);
    
    cass_statement_bind_int64(update_statement, 0, now * 1000); // Convert to milliseconds
    cass_statement_bind_string(update_statement, 1, user_id);
    cass_statement_bind_uint32(update_statement, 2, created_date);
    cass_statement_bind_int64(update_statement, 3, created_at_ms);
    cass_statement_bind_uuid(update_statement, 4, uuid);

    CassFuture* update_future = cass_session_execute(repo->session, update_statement);

    if (cass_future_error_code(update_future) != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(update_future, &message, &message_length);
        fprintf(repo->logger, "Error marking notification as read: %.*s\n", (int)message_length, message);
        
        cass_future_free(update_future);
        cass_statement_free(update_statement);
        return -1;
    }

    cass_future_free(update_future);
    cass_statement_free(update_statement);

    printf("Notification %s marked as read successfully.\n", notification_id);
    return 0;
}
