#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include "model.h"
#include "repo.h"

typedef struct {
    mongoc_client_t* client;
    mongoc_collection_t* collection;
    FILE* logger;
} Repository;

Repository* New(FILE* logger) {
    printf("Initializing MongoDB client...\n");
    const char* dburi = "mongodb://localhost:27017/";

    if (dburi == NULL) {
        fprintf(logger, "Error: MONGO_DB_URI environment variable is not set\n");
        printf("Error: MONGO_DB_URI environment variable is not set\n");
        return NULL;
    }

    mongoc_client_t* client = mongoc_client_new(dburi);
    if (!client) {
        fprintf(logger, "Error: Failed to create MongoDB client\n");
        printf("Error: Failed to create MongoDB client\n");
        mongoc_cleanup();
        return NULL;
    }
    printf("MongoDB client created successfully.\n");

    bson_error_t error;
    if (!mongoc_client_get_server_status(client, NULL, NULL, &error)) {
        fprintf(logger, "Error: Failed to connect to MongoDB: %s\n", error.message);
        printf("Error: Failed to connect to MongoDB: %s\n", error.message);
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }
    printf("Connected to MongoDB server successfully.\n");

    Repository* repo = (Repository*)malloc(sizeof(Repository));
    if (repo == NULL) {
        fprintf(logger, "Error: Failed to allocate memory for repository\n");
        printf("Error: Failed to allocate memory for repository\n");
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }

    repo->client = client;
    repo->logger = logger;
    return repo;
}

void Cleanup(Repository* repo) {
    if (repo) {
        printf("Cleaning up MongoDB client and repository...\n");
        if (repo->collection) {
            mongoc_collection_destroy(repo->collection);
        }
        mongoc_client_destroy(repo->client);
        free(repo);
        printf("Cleanup completed.\n");
    }
}

int add_task(Task* task) {
    printf("Adding task to MongoDB...\n");
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return 1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        printf("Error initializing repository.\n");
        fclose(log);
        return 1;
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    bson_t* doc = bson_new();
    bson_oid_t oid;
    bson_oid_init(&oid, NULL);
    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "project_id", task->project_id);
    BSON_APPEND_UTF8(doc, "name", task->name);
    BSON_APPEND_UTF8(doc, "description", task->description);
    BSON_APPEND_UTF8(doc, "creator_id", task->creator_id);
    BSON_APPEND_INT32(doc, "status", task->status);

    // Add members array
    bson_t members_array;
    BSON_APPEND_ARRAY_BEGIN(doc, "members", &members_array);
    for (int i = 0; i < task->member_count; i++) {
        char str_idx[16];
        snprintf(str_idx, sizeof(str_idx), "%d", i);
        BSON_APPEND_UTF8(&members_array, str_idx, task->members[i]);
    }
    bson_append_array_end(doc, &members_array);

    // Print the BSON document for debugging
    char* json_str = bson_as_json(doc, NULL);
    printf("Document to be inserted: %s\n", json_str);
    bson_free(json_str);

    bson_error_t error;
    if (!mongoc_collection_insert_one(repo->collection, doc, NULL, NULL, &error)) {
        fprintf(repo->logger, "Error: Insert failed: %s\n", error.message);
        printf("Error: Insert failed: %s\n", error.message);
        bson_destroy(doc);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    printf("Task inserted successfully.\n");
    bson_destroy(doc);
    Cleanup(repo);
    fclose(log);
    return 0;
}

char* get_tasks_by_project(const char* project_id) {
    printf("\n=== Starting get_tasks_by_project ===\n");
    printf("Looking for tasks with project_id: %s\n", project_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return NULL;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return NULL;
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create the query
    bson_t* query = bson_new();
    BSON_APPEND_UTF8(query, "project_id", project_id);

    // Print the query for debugging
    char* query_str = bson_as_json(query, NULL);
    printf("MongoDB Query: %s\n", query_str);
    bson_free(query_str);

    // Count documents matching the query
    bson_error_t error;
    int64_t count = mongoc_collection_count_documents(repo->collection, query, NULL, NULL, NULL, &error);
    printf("Total matching documents before cursor: %lld\n", (long long)count);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    cJSON* tasks_array = cJSON_CreateArray();
    int found_count = 0;

    while (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        printf("\nFound document %d: %s\n", found_count + 1, doc_str);
        
        cJSON* task_json = cJSON_Parse(doc_str);
        if (task_json) {
            cJSON_AddItemToArray(tasks_array, task_json);
            found_count++;
        } else {
            printf("Failed to parse document to JSON!\n");
        }
        bson_free(doc_str);
    }

    // Check for cursor errors
    if (mongoc_cursor_error(cursor, &error)) {
        printf("Cursor error: %s\n", error.message);
    }

    printf("\nFound %d tasks\n", found_count);

    char* result = cJSON_PrintUnformatted(tasks_array);
    printf("Returning JSON result: %s\n", result);

    cJSON_Delete(tasks_array);
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    printf("=== Finished get_tasks_by_project ===\n\n");
    return result;
}

int update_task_members(const char* task_id, const char** members, int member_count) {
    printf("\n=== Starting update_task_members ===\n");
    // Skip any leading forward slash
    while (*task_id == '/') {
        task_id++;
    }
    printf("Updating members for task ID: %s\n", task_id);
    printf("Number of members to update: %d\n", member_count);
    for (int i = 0; i < member_count; i++) {
        printf("Member %d: %s\n", i, members[i]);
    }

    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return 1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return 1;
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, task_id);
    BSON_APPEND_OID(query, "_id", &oid);

    bson_t* update = bson_new();
    bson_t set;
    bson_t members_array;

    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set);
    BSON_APPEND_ARRAY_BEGIN(&set, "members", &members_array);

    for (int i = 0; i < member_count; i++) {
        char str_idx[16];
        snprintf(str_idx, sizeof(str_idx), "%d", i);
        BSON_APPEND_UTF8(&members_array, str_idx, members[i]);
    }

    bson_append_array_end(&set, &members_array);
    bson_append_document_end(update, &set);

    bson_error_t error;
    if (!mongoc_collection_update_one(repo->collection, query, update, NULL, NULL, &error)) {
        printf("Error updating task members: %s\n", error.message);
        bson_destroy(query);
        bson_destroy(update);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    bson_destroy(query);
    bson_destroy(update);
    Cleanup(repo);
    fclose(log);
    return 0;
}

int update_task_status(const char* task_id, TaskStatus status) {
    printf("\n=== Starting update_task_status ===\n");
    // Skip any leading forward slash
    while (*task_id == '/') {
        task_id++;
    }
    printf("Updating task status for task ID: %s to status: %d\n", task_id, status);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return 1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return 1;
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, task_id);
    BSON_APPEND_OID(query, "_id", &oid);

    bson_t* update = bson_new();
    bson_t set;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set);
    BSON_APPEND_INT32(&set, "status", status);
    bson_append_document_end(update, &set);

    // Print the query and update for debugging
    char* query_str = bson_as_json(query, NULL);
    char* update_str = bson_as_json(update, NULL);
    printf("Query: %s\n", query_str);
    printf("Update: %s\n", update_str);
    bson_free(query_str);
    bson_free(update_str);

    bson_error_t error;
    bool result = mongoc_collection_update_one(repo->collection, query, update, NULL, NULL, &error);
    
    if (!result) {
        printf("Error updating task status: %s\n", error.message);
        bson_destroy(query);
        bson_destroy(update);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    printf("Task status updated successfully\n");
    bson_destroy(query);
    bson_destroy(update);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished update_task_status ===\n\n");
    return 0;
}

int validate_project_member(const char* project_id, const char* member_id) {
    return 0;
}

int repo() {
    printf("Initializing MongoDB...\n");
    mongoc_init();
    printf("MongoDB initialized.\n");
    return 0;
}
