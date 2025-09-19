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
    
    const char *var_name = "DBURI";
    char *dburi = getenv(var_name);
    printf("Bruhimic moram priznati\n");

    if (dburi == NULL) {
        fprintf(logger, "Error: MONGO_DB_URI environment variable is not set\n");
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

    // Update project status - when a task is added, project becomes active
    printf("Updating project status to active for project: %s\n", task->project_id);
    update_project_status_from_tasks(task->project_id);

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

    // Get the task to find its project_id for updating project status
    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    char* project_id_for_update = NULL;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        cJSON* task_json = cJSON_Parse(doc_str);
        if (task_json) {
            cJSON* project_id_json = cJSON_GetObjectItem(task_json, "project_id");
            if (project_id_json && cJSON_IsString(project_id_json)) {
                project_id_for_update = strdup(project_id_json->valuestring);
                printf("Found project_id for status update: %s\n", project_id_for_update);
            }
            cJSON_Delete(task_json);
        }
        bson_free(doc_str);
    }
    mongoc_cursor_destroy(cursor);

    bson_destroy(query);
    bson_destroy(update);
    Cleanup(repo);
    fclose(log);

    // Update project status based on tasks completion
    if (project_id_for_update) {
        printf("Updating project status for project: %s\n", project_id_for_update);
        update_project_status_from_tasks(project_id_for_update);
        free(project_id_for_update);
    }

    printf("=== Finished update_task_status ===\n\n");
    return 0;
}

char* get_user_tasks_by_project(const char* project_id, const char* user_id) {
    printf("\n=== Starting get_user_tasks_by_project ===\n");
    printf("Looking for tasks with project_id: %s for user: %s\n", project_id, user_id);
    
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

    // Create the query to find tasks where:
    // 1. project_id matches AND
    // 2. user is either the creator OR in the members array
    bson_t* query = bson_new();
    
    BSON_APPEND_UTF8(query, "project_id", project_id);
    
    // Create OR condition: creator_id = user_id OR members contains user_id
    bson_t or_array;
    BSON_APPEND_ARRAY_BEGIN(query, "$or", &or_array);
    
    bson_t creator_condition;
    BSON_APPEND_DOCUMENT_BEGIN(&or_array, "0", &creator_condition);
    BSON_APPEND_UTF8(&creator_condition, "creator_id", user_id);
    bson_append_document_end(&or_array, &creator_condition);
    
    bson_t members_condition;
    BSON_APPEND_DOCUMENT_BEGIN(&or_array, "1", &members_condition);
    BSON_APPEND_UTF8(&members_condition, "members", user_id);
    bson_append_document_end(&or_array, &members_condition);
    
    bson_append_array_end(query, &or_array);

    // Print the query for debugging
    char* query_str = bson_as_json(query, NULL);
    printf("MongoDB Query: %s\n", query_str);
    bson_free(query_str);

    // Count documents matching the query
    bson_error_t error;
    int64_t count = mongoc_collection_count_documents(repo->collection, query, NULL, NULL, NULL, &error);
    printf("Total matching documents for user: %lld\n", (long long)count);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    cJSON* tasks_array = cJSON_CreateArray();
    int found_count = 0;

    while (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        printf("\nFound user-accessible document %d: %s\n", found_count + 1, doc_str);
        
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

    printf("\nFound %d user-accessible tasks\n", found_count);

    char* result = cJSON_PrintUnformatted(tasks_array);
    printf("Returning JSON result: %s\n", result);

    cJSON_Delete(tasks_array);
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    printf("=== Finished get_user_tasks_by_project ===\n\n");
    return result;
}

int validate_project_member(const char* project_id, const char* member_id) {
    printf("\n=== Starting validate_project_member ===\n");
    printf("Validating if user %s is member of project %s\n", member_id, project_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return -1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return -1;
    }

    // Connect to projects database to check membership
    const char* db_name = "trello";
    const char* collection_name = "projects";
    mongoc_collection_t* projects_collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create query to find project by ID
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, project_id);
    BSON_APPEND_OID(query, "_id", &oid);

    // Print the query for debugging
    char* query_str = bson_as_json(query, NULL);
    printf("Project validation query: %s\n", query_str);
    bson_free(query_str);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(projects_collection, query, NULL, NULL);
    const bson_t* doc;
    int is_member = 0;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        printf("Found project: %s\n", doc_str);
        
        // Parse the project document to check members
        cJSON* project_json = cJSON_Parse(doc_str);
        if (project_json) {
            // Check if user is the moderator (equivalent to owner)
            cJSON* moderator = cJSON_GetObjectItem(project_json, "moderator");
            if (moderator && cJSON_IsString(moderator) && strcmp(moderator->valuestring, member_id) == 0) {
                printf("User is project moderator\n");
                is_member = 1;
            } else {
                // Check if user is in members array
                cJSON* members = cJSON_GetObjectItem(project_json, "members");
                if (members && cJSON_IsArray(members)) {
                    cJSON* member;
                    cJSON_ArrayForEach(member, members) {
                        if (cJSON_IsString(member) && strcmp(member->valuestring, member_id) == 0) {
                            printf("User found in project members\n");
                            is_member = 1;
                            break;
                        }
                    }
                }
            }
            cJSON_Delete(project_json);
        }
        bson_free(doc_str);
    } else {
        printf("Project not found\n");
    }

    bson_error_t error;
    if (mongoc_cursor_error(cursor, &error)) {
        printf("Cursor error: %s\n", error.message);
    }

    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(projects_collection);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    printf("User %s is %s member of project %s\n", member_id, is_member ? "a" : "NOT a", project_id);
    printf("is_member value: %d\n", is_member);
    printf("Returning: %d\n", is_member ? 0 : -1);
    printf("=== Finished validate_project_member ===\n\n");
    
    return is_member ? 0 : -1;
}

int can_user_update_task(const char* task_id, const char* user_id) {
    printf("\n=== Starting can_user_update_task ===\n");
    printf("Checking if user %s can update task %s\n", user_id, task_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return -1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return -1;
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create query to find task by ID
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, task_id);
    BSON_APPEND_OID(query, "_id", &oid);

    // Print the query for debugging
    char* query_str = bson_as_json(query, NULL);
    printf("Task query: %s\n", query_str);
    bson_free(query_str);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    int can_update = 0;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        printf("Found task: %s\n", doc_str);
        
        // Parse the task document to check creator and members
        cJSON* task_json = cJSON_Parse(doc_str);
        if (task_json) {
            // Check if user is the creator
            cJSON* creator = cJSON_GetObjectItem(task_json, "creator_id");
            if (creator && cJSON_IsString(creator) && strcmp(creator->valuestring, user_id) == 0) {
                printf("User is task creator\n");
                can_update = 1;
            } else {
                // Check if user is in members array
                cJSON* members = cJSON_GetObjectItem(task_json, "members");
                if (members && cJSON_IsArray(members)) {
                    cJSON* member;
                    cJSON_ArrayForEach(member, members) {
                        if (cJSON_IsString(member) && strcmp(member->valuestring, user_id) == 0) {
                            printf("User found in task members\n");
                            can_update = 1;
                            break;
                        }
                    }
                }
            }
            cJSON_Delete(task_json);
        }
        bson_free(doc_str);
    } else {
        printf("Task not found\n");
    }

    bson_error_t error;
    if (mongoc_cursor_error(cursor, &error)) {
        printf("Cursor error: %s\n", error.message);
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    printf("User %s %s update task %s\n", user_id, can_update ? "CAN" : "CANNOT", task_id);
    printf("=== Finished can_user_update_task ===\n\n");
    
    return can_update ? 0 : -1;
}

int has_unfinished_tasks(const char* project_id, const char* user_id) {
    printf("\n=== Starting has_unfinished_tasks ===\n");
    printf("Checking if user %s has unfinished tasks in project %s\n", user_id, project_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return 1; // Return error - assume user has unfinished tasks
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return 1; // Return error - assume user has unfinished tasks
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create query to find tasks where:
    // 1. project_id matches AND
    // 2. user is either the creator OR in the members array AND
    // 3. status is NOT completed (status != 2)
    bson_t* query = bson_new();
    
    // Add project_id condition
    BSON_APPEND_UTF8(query, "project_id", project_id);
    
    // Add status condition (not completed)
    bson_t ne_condition;
    BSON_APPEND_DOCUMENT_BEGIN(query, "status", &ne_condition);
    BSON_APPEND_INT32(&ne_condition, "$ne", STATUS_COMPLETED);
    bson_append_document_end(query, &ne_condition);
    
    // Create OR condition: creator_id = user_id OR members contains user_id
    bson_t or_array;
    BSON_APPEND_ARRAY_BEGIN(query, "$or", &or_array);
    
    bson_t creator_condition;
    BSON_APPEND_DOCUMENT_BEGIN(&or_array, "0", &creator_condition);
    BSON_APPEND_UTF8(&creator_condition, "creator_id", user_id);
    bson_append_document_end(&or_array, &creator_condition);
    
    bson_t members_condition;
    BSON_APPEND_DOCUMENT_BEGIN(&or_array, "1", &members_condition);
    BSON_APPEND_UTF8(&members_condition, "members", user_id);
    bson_append_document_end(&or_array, &members_condition);
    
    bson_append_array_end(query, &or_array);

    // Print the query for debugging
    char* query_str = bson_as_json(query, NULL);
    printf("MongoDB Query for unfinished tasks: %s\n", query_str);
    bson_free(query_str);

    // Count documents matching the query
    bson_error_t error;
    int64_t count = mongoc_collection_count_documents(repo->collection, query, NULL, NULL, NULL, &error);
    printf("Number of unfinished tasks for user: %lld\n", (long long)count);

    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    printf("User %s %s unfinished tasks in project %s\n", 
           user_id, (count > 0) ? "HAS" : "DOES NOT HAVE", project_id);
    printf("=== Finished has_unfinished_tasks ===\n\n");
    
    return (count > 0) ? 1 : 0; // Return 1 if user has unfinished tasks, 0 if not
}

int update_project_status_from_tasks(const char* project_id) {
    printf("\n=== Starting update_project_status_from_tasks ===\n");
    printf("Checking and updating project status for project %s\n", project_id);
    
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

    // Check if project has any tasks at all
    bson_t* all_tasks_query = bson_new();
    BSON_APPEND_UTF8(all_tasks_query, "project_id", project_id);
    
    bson_error_t error;
    int64_t total_tasks = mongoc_collection_count_documents(repo->collection, all_tasks_query, NULL, NULL, NULL, &error);
    printf("Total tasks in project: %lld\n", (long long)total_tasks);
    
    int new_project_status = 0; // 0 = PROJECT_ACTIVE, 1 = PROJECT_COMPLETED

    if (total_tasks == 0) {
        printf("Project has no tasks - setting as completed\n");
        new_project_status = 1; // PROJECT_COMPLETED
    } else {
        // Check for unfinished tasks (status != 2)
        bson_t* unfinished_query = bson_new();
        BSON_APPEND_UTF8(unfinished_query, "project_id", project_id);
        
        bson_t ne_condition;
        BSON_APPEND_DOCUMENT_BEGIN(unfinished_query, "status", &ne_condition);
        BSON_APPEND_INT32(&ne_condition, "$ne", STATUS_COMPLETED);
        bson_append_document_end(unfinished_query, &ne_condition);

        int64_t unfinished_tasks = mongoc_collection_count_documents(repo->collection, unfinished_query, NULL, NULL, NULL, &error);
        printf("Unfinished tasks in project: %lld\n", (long long)unfinished_tasks);

        if (unfinished_tasks > 0) {
            printf("Project has unfinished tasks - setting as active\n");
            new_project_status = 0; // PROJECT_ACTIVE
        } else {
            printf("All tasks completed - setting project as completed\n");
            new_project_status = 1; // PROJECT_COMPLETED
        }

        bson_destroy(unfinished_query);
    }

    bson_destroy(all_tasks_query);

    // Now update the project status in the projects database
    const char* projects_db_name = "trello";
    const char* projects_collection_name = "projects";
    mongoc_collection_t* projects_collection = mongoc_client_get_collection(repo->client, projects_db_name, projects_collection_name);

    bson_t* project_query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, project_id);
    BSON_APPEND_OID(project_query, "_id", &oid);

    bson_t* update = bson_new();
    bson_t set;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set);
    BSON_APPEND_INT32(&set, "status", new_project_status);
    bson_append_document_end(update, &set);

    if (!mongoc_collection_update_one(projects_collection, project_query, update, NULL, NULL, &error)) {
        printf("Error updating project status: %s\n", error.message);
        bson_destroy(project_query);
        bson_destroy(update);
        mongoc_collection_destroy(projects_collection);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    printf("Project status updated successfully to %d\n", new_project_status);
    bson_destroy(project_query);
    bson_destroy(update);
    mongoc_collection_destroy(projects_collection);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished update_project_status_from_tasks ===\n\n");
    return 0;
}

int get_task_project_id(const char* task_id, char* project_id_out) {
    printf("\n=== Starting get_task_project_id ===\n");
    printf("Getting project ID for task: %s\n", task_id);
    
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

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    int found = 0;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        cJSON* task_json = cJSON_Parse(doc_str);
        if (task_json) {
            cJSON* project_id_json = cJSON_GetObjectItem(task_json, "project_id");
            if (project_id_json && cJSON_IsString(project_id_json)) {
                strncpy(project_id_out, project_id_json->valuestring, MAX_STRING_LENGTH - 1);
                project_id_out[MAX_STRING_LENGTH - 1] = '\0';
                printf("Found project ID: %s\n", project_id_out);
                found = 1;
            }
            cJSON_Delete(task_json);
        }
        bson_free(doc_str);
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished get_task_project_id ===\n\n");
    
    return found ? 0 : 1;
}

int get_task_status(const char* task_id) {
    printf("\n=== Starting get_task_status ===\n");
    printf("Getting status for task: %s\n", task_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return -1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return -1;
    }

    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, task_id);
    BSON_APPEND_OID(query, "_id", &oid);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    int status = -1;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        cJSON* task_json = cJSON_Parse(doc_str);
        if (task_json) {
            cJSON* status_json = cJSON_GetObjectItem(task_json, "status");
            if (status_json && cJSON_IsNumber(status_json)) {
                status = status_json->valueint;
                printf("Found task status: %d\n", status);
            }
            cJSON_Delete(task_json);
        }
        bson_free(doc_str);
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished get_task_status ===\n\n");
    
    return status;
}

int add_member_to_task(const char* task_id, const char* member_id) {
    printf("\n=== Starting add_member_to_task ===\n");
    printf("Adding member %s to task %s\n", member_id, task_id);
    
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

    // First, get the task's project ID to validate membership using same connection
    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Get project ID from task
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, task_id);
    BSON_APPEND_OID(query, "_id", &oid);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    char project_id[MAX_STRING_LENGTH] = {0};
    int found_task = 0;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        cJSON* task_json = cJSON_Parse(doc_str);
        if (task_json) {
            cJSON* project_id_json = cJSON_GetObjectItem(task_json, "project_id");
            if (project_id_json && cJSON_IsString(project_id_json)) {
                strncpy(project_id, project_id_json->valuestring, MAX_STRING_LENGTH - 1);
                project_id[MAX_STRING_LENGTH - 1] = '\0';
                found_task = 1;
                printf("Found project ID: %s\n", project_id);
            }
            cJSON_Delete(task_json);
        }
        bson_free(doc_str);
    }

    mongoc_cursor_destroy(cursor);

    if (!found_task) {
        printf("Error: Could not find task or get project ID\n");
        bson_destroy(query);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    // Validate project membership using same connection
    const char* projects_db_name = "trello";
    const char* projects_collection_name = "projects";
    mongoc_collection_t* projects_collection = mongoc_client_get_collection(repo->client, projects_db_name, projects_collection_name);

    bson_t* project_query = bson_new();
    bson_oid_t project_oid;
    bson_oid_init_from_string(&project_oid, project_id);
    BSON_APPEND_OID(project_query, "_id", &project_oid);

    mongoc_cursor_t* project_cursor = mongoc_collection_find_with_opts(projects_collection, project_query, NULL, NULL);
    const bson_t* project_doc;
    int is_member = 0;

    if (mongoc_cursor_next(project_cursor, &project_doc)) {
        char* project_doc_str = bson_as_json(project_doc, NULL);
        printf("Found project: %s\n", project_doc_str);
        
        cJSON* project_json = cJSON_Parse(project_doc_str);
        if (project_json) {
            // Check if user is the moderator
            cJSON* moderator = cJSON_GetObjectItem(project_json, "moderator");
            if (moderator && cJSON_IsString(moderator) && strcmp(moderator->valuestring, member_id) == 0) {
                printf("User is project moderator\n");
                is_member = 1;
            } else {
                // Check if user is in members array
                cJSON* members = cJSON_GetObjectItem(project_json, "members");
                if (members && cJSON_IsArray(members)) {
                    cJSON* member;
                    cJSON_ArrayForEach(member, members) {
                        if (cJSON_IsString(member) && strcmp(member->valuestring, member_id) == 0) {
                            printf("User found in project members\n");
                            is_member = 1;
                            break;
                        }
                    }
                }
            }
            cJSON_Delete(project_json);
        }
        bson_free(project_doc_str);
    } else {
        printf("Project not found\n");
    }

    mongoc_cursor_destroy(project_cursor);
    mongoc_collection_destroy(projects_collection);
    bson_destroy(project_query);

    if (!is_member) {
        printf("Error: User %s is not a member of project %s\n", member_id, project_id);
        bson_destroy(query);
        Cleanup(repo);
        fclose(log);
        return 2; // Special error code for "not a project member"
    }

    // Now add member to task using same connection
    bson_t* update = bson_new();
    bson_t add_to_set;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$addToSet", &add_to_set);
    BSON_APPEND_UTF8(&add_to_set, "members", member_id);
    bson_append_document_end(update, &add_to_set);

    bson_error_t error;
    bson_t reply;
    bool update_result = mongoc_collection_update_one(repo->collection, query, update, NULL, &reply, &error);
    if (!update_result) {
        printf("Error adding member to task: %s\n", error.message);
        bson_destroy(query);
        bson_destroy(update);
        bson_destroy(&reply);
        Cleanup(repo);
        fclose(log);
        return 1;
    }
    bson_destroy(&reply);

    printf("Member %s added to task %s successfully\n", member_id, task_id);
    bson_destroy(query);
    bson_destroy(update);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished add_member_to_task ===\n\n");
    
    return 0;
}

int remove_member_from_task(const char* task_id, const char* member_id) {
    printf("\n=== Starting remove_member_from_task ===\n");
    printf("Removing member %s from task %s\n", member_id, task_id);
    
    // First, check task status - cannot remove from finished tasks
    int task_status = get_task_status(task_id);
    if (task_status == STATUS_COMPLETED) {
        printf("Error: Cannot remove member from completed task (status: %d)\n", task_status);
        return 2; // Special error code for "task is finished"
    }
    if (task_status == -1) {
        printf("Error: Could not get task status\n");
        return 1;
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

    // Use MongoDB's $pull to remove member
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, task_id);
    BSON_APPEND_OID(query, "_id", &oid);

    bson_t* update = bson_new();
    bson_t pull;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$pull", &pull);
    BSON_APPEND_UTF8(&pull, "members", member_id);
    bson_append_document_end(update, &pull);

    bson_error_t error;
    bson_t reply;
    bool update_result = mongoc_collection_update_one(repo->collection, query, update, NULL, &reply, &error);
    if (!update_result) {
        printf("Error removing member from task: %s\n", error.message);
        bson_destroy(query);
        bson_destroy(update);
        bson_destroy(&reply);
        Cleanup(repo);
        fclose(log);
        return 1;
    }
    bson_destroy(&reply);

    printf("Member %s removed from task %s successfully\n", member_id, task_id);
    bson_destroy(query);
    bson_destroy(update);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished remove_member_from_task ===\n\n");
    
    return 0;
}

int repo() {
    printf("Initializing MongoDB...\n");
    mongoc_init();
    printf("MongoDB initialized.\n");
    return 0;
}
