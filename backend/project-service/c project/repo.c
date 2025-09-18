#include <mongoc/mongoc.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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

    // Create MongoDB client
    mongoc_client_t* client = mongoc_client_new(dburi);
    if (!client) {
        fprintf(logger, "Error: Failed to create MongoDB client\n");
        printf("Error: Failed to create MongoDB client\n");
        mongoc_cleanup();
        return NULL;
    }
    printf("MongoDB client created successfully.\n");

    // Test connection
    bson_error_t error;
    if (!mongoc_client_get_server_status(client, NULL, NULL, &error)) {
        fprintf(logger, "Error: Failed to connect to MongoDB: %s\n", error.message);
        printf("Error: Failed to connect to MongoDB: %s\n", error.message);
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }
    printf("Connected to MongoDB server successfully.\n");

    // Allocate memory for the Repository struct
    Repository* repo = (Repository*)malloc(sizeof(Repository));
    if (repo == NULL) {
        fprintf(logger, "Error: Failed to allocate memory for repository\n");
        printf("Error: Failed to allocate memory for repository\n");
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }
    printf("Repository struct allocated successfully.\n");

    // Set up the Repository struct fields
    repo->client = client;
    repo->logger = logger;

    return repo;
}

// Cleanup function to release resources
void Cleanup(Repository* repo) {
    if (repo) {
        printf("Cleaning up MongoDB client and repository...\n");
        mongoc_client_destroy(repo->client);
        free(repo);
        printf("Cleanup completed.\n");
    }
}

int addproject(Project* project) {
    printf("Adding project to MongoDB...\n");
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

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    if (repo->collection == NULL) {
        printf("Error: Could not get collection from database.\n");
        fclose(log);
        Cleanup(repo);
        return 1;
    }

    // Create the main document
    // In your addproject function, update the BSON document creation:
    bson_t* doc = bson_new();
    BSON_APPEND_UTF8(doc, "moderator", project->moderator);
    BSON_APPEND_UTF8(doc, "project", project->project);
    BSON_APPEND_UTF8(doc, "estimated_completion_date", project->estimated_completion_date);
    BSON_APPEND_INT32(doc, "min_members", project->min_members);
    BSON_APPEND_INT32(doc, "max_members", project->max_members);
    BSON_APPEND_INT32(doc, "current_member_count", project->current_member_count);
    BSON_APPEND_INT32(doc, "status", PROJECT_ACTIVE); // New projects start as active

    // Create members array
    bson_t members_array;
    BSON_APPEND_ARRAY_BEGIN(doc, "members", &members_array);

    for (int i = 0; i < project->current_member_count; i++) {
        char str_idx[16];
        snprintf(str_idx, sizeof(str_idx), "%d", i);
        BSON_APPEND_UTF8(&members_array, str_idx, project->members[i]);
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
    }
    else {
        fprintf(repo->logger, "Document inserted successfully.\n");
        printf("Document inserted successfully.\n");
    }

    bson_destroy(doc);
    Cleanup(repo);
    fclose(log);
    return 0;
}
char* get_all_projects() {
    printf("Fetching all projects from MongoDB...\n");
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

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    if (repo->collection == NULL) {
        printf("Error: Could not get collection from database.\n");
        fclose(log);
        Cleanup(repo);
        return NULL;
    }

    bson_t* query = bson_new();
    mongoc_cursor_t* cursor;
    const bson_t* doc;
    bson_error_t error;
    cJSON* projects_array = cJSON_CreateArray();

    if (!projects_array) {
        printf("Error: Failed to create JSON array\n");
        bson_destroy(query);
        fclose(log);
        Cleanup(repo);
        return NULL;
    }

    cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);

    while (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_json(doc, NULL);
        if (!doc_str) {
            printf("Error: Failed to convert BSON to JSON string\n");
            continue;
        }

        printf("Raw document: %s\n", doc_str); // Debug print

        cJSON* project_json = cJSON_Parse(doc_str);
        if (!project_json) {
            printf("Error: Failed to parse document JSON\n");
            bson_free(doc_str);
            continue;
        }

        // Add the project to the array
        cJSON_AddItemToArray(projects_array, project_json);
        bson_free(doc_str);
    }

    if (mongoc_cursor_error(cursor, &error)) {
        printf("Cursor error: %s\n", error.message);
        cJSON_Delete(projects_array);
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        Cleanup(repo);
        fclose(log);
        return NULL;
    }

    // Convert the final array to string
    char* result = cJSON_PrintUnformatted(projects_array);
    if (!result) {
        printf("Error: Failed to convert final JSON to string\n");
    }
    else {
        printf("Final JSON result: %s\n", result); // Debug print
    }

    // Cleanup
    cJSON_Delete(projects_array);
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    return result;
}
char* get_project_by_id(const char* project_id) {
    printf("Fetching project by ID from MongoDB...\n");
    printf("Project ID received: %s\n", project_id);

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

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    if (repo->collection == NULL) {
        printf("Error: Could not get collection from database.\n");
        fclose(log);
        Cleanup(repo);
        return NULL;
    }

    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_error_t error;

    if (!bson_oid_is_valid(project_id, strlen(project_id))) {
        printf("Error: Invalid ObjectId format: %s\n", project_id);
        bson_destroy(query);
        Cleanup(repo);
        fclose(log);
        return NULL;
    }

    bson_oid_init_from_string(&oid, project_id);
    BSON_APPEND_OID(query, "_id", &oid);

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    char* result = NULL;

    if (mongoc_cursor_next(cursor, &doc)) {
        char* temp = bson_as_json(doc, NULL);
        if (temp) {
            result = strdup(temp);  // Create a copy of the JSON string
            bson_free(temp);
            printf("Found project: %s\n", result);
        }
    }
    else {
        printf("No project found with ID: %s\n", project_id);
    }

    // Cleanup
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(repo->collection);
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);

    return result;
}
int update_project_members(const char* project_id, const char** members, int member_count) {
    printf("Updating project members in MongoDB...\n");
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

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create query to find the project by ID
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_error_t error;

    // Try to convert the string ID to ObjectId
    bson_oid_init_from_string(&oid, project_id);
    BSON_APPEND_OID(query, "_id", &oid);

    // Create update document
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
    BSON_APPEND_INT32(&set, "current_member_count", member_count);
    bson_append_document_end(update, &set);

    // Print the update document for debugging
    char* json_str = bson_as_json(update, NULL);
    printf("Update document: %s\n", json_str);
    bson_free(json_str);

    // Perform the update
    if (!mongoc_collection_update_one(repo->collection, query, update, NULL, NULL, &error)) {
        printf("Error updating project: %s\n", error.message);
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

/**
 * Get projects filtered by user role and ID
 * MANAGER: Gets all projects (or projects they moderate)
 * USER: Gets only projects they are members of
 */
char* get_projects_by_user_role(const char* user_id, const char* role) {
    printf("Filtering projects for user: %s with role: %s\n", user_id, role);
    
    // For now, MANAGERs get all projects, USERs get filtered projects
    if (strcmp(role, "MANAGER") == 0) {
        // Managers see all projects
        return get_all_projects();
    } else if (strcmp(role, "USER") == 0) {
        // SAFE FALLBACK: Get all projects and filter in code
        // This avoids the MongoDB cursor crash while still providing filtering
        printf("Getting all projects and filtering for user: %s\n", user_id);
        
        char* all_projects = get_all_projects();
        if (all_projects == NULL) {
            return NULL;
        }
        
        // Parse JSON and filter projects where user is a member
        cJSON* json_array = cJSON_Parse(all_projects);
        if (json_array == NULL) {
            printf("Error: Failed to parse projects JSON\n");
            free(all_projects);
            return NULL;
        }
        
        cJSON* filtered_array = cJSON_CreateArray();
        int array_size = cJSON_GetArraySize(json_array);
        int found_count = 0;
        
        printf("Checking %d projects for user membership\n", array_size);
        
        for (int i = 0; i < array_size; i++) {
            cJSON* project = cJSON_GetArrayItem(json_array, i);
            cJSON* members = cJSON_GetObjectItem(project, "members");
            
            if (cJSON_IsArray(members)) {
                int members_count = cJSON_GetArraySize(members);
                bool user_is_member = false;
                
                for (int j = 0; j < members_count; j++) {
                    cJSON* member = cJSON_GetArrayItem(members, j);
                    if (cJSON_IsString(member) && strcmp(member->valuestring, user_id) == 0) {
                        user_is_member = true;
                        printf("User %s found in project %d\n", user_id, i);
                        break;
                    }
                }
                
                if (user_is_member) {
                    cJSON* project_copy = cJSON_Duplicate(project, 1);
                    cJSON_AddItemToArray(filtered_array, project_copy);
                    found_count++;
                }
            }
        }
        
        char* filtered_json = cJSON_Print(filtered_array);
        printf("Found %d projects where user %s is a member\n", found_count, user_id);
        
        // Cleanup
        cJSON_Delete(json_array);
        cJSON_Delete(filtered_array);
        free(all_projects);
        
        return filtered_json;
    }
    
    // Unknown role
    return NULL;
}

/**
 * Check if user has access to a specific project
 * MANAGER: Access to all projects
 * USER: Access only to projects they're members of
 */
int check_user_project_access(const char* user_id, const char* role, const char* project_id) {
    printf("Checking access for user: %s (role: %s) to project: %s\n", user_id, role, project_id);
    
    // Managers have access to all projects
    if (strcmp(role, "MANAGER") == 0) {
        return 0; // Access granted
    }
    
    // For users, check if they're members of the specific project
    if (strcmp(role, "USER") == 0) {
        printf("Checking if user %s is a member of project %s\n", user_id, project_id);
        
        // Use the existing get_project_by_id function to fetch the project
        char* project_json = get_project_by_id(project_id);
        if (project_json == NULL) {
            printf("Project not found or error fetching project\n");
            return -1;
        }
        
        // Parse the project JSON
        cJSON* project = cJSON_Parse(project_json);
        if (project == NULL) {
            printf("Error parsing project JSON\n");
            free(project_json);
            return -1;
        }
        
        // Get the members array
        cJSON* members = cJSON_GetObjectItem(project, "members");
        if (!cJSON_IsArray(members)) {
            printf("No members array found in project\n");
            cJSON_Delete(project);
            free(project_json);
            return -1;
        }
        
        // Check if user is in the members array
        int members_count = cJSON_GetArraySize(members);
        bool user_is_member = false;
        
        for (int i = 0; i < members_count; i++) {
            cJSON* member = cJSON_GetArrayItem(members, i);
            if (cJSON_IsString(member) && strcmp(member->valuestring, user_id) == 0) {
                user_is_member = true;
                printf("Access granted: User %s is a member of project %s\n", user_id, project_id);
                break;
            }
        }
        
        if (!user_is_member) {
            printf("Access denied: User %s is not a member of project %s\n", user_id, project_id);
        }
        
        // Cleanup
        cJSON_Delete(project);
        free(project_json);
        
        return user_is_member ? 0 : -1;
    }
    
    return -1; // Access denied
}

int check_members_unfinished_tasks(const char* project_id, const char** removed_members, int removed_count) {
    printf("\n=== Starting check_members_unfinished_tasks ===\n");
    printf("Checking %d removed members for unfinished tasks in project %s\n", removed_count, project_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return 1; // Return error - assume there are unfinished tasks
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return 1; // Return error - assume there are unfinished tasks
    }

    // Connect to tasks database
    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    mongoc_collection_t* tasks_collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    for (int i = 0; i < removed_count; i++) {
        const char* user_id = removed_members[i];
        printf("Checking user: %s\n", user_id);
        
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
        BSON_APPEND_INT32(&ne_condition, "$ne", 2); // STATUS_COMPLETED = 2
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
        printf("MongoDB Query for user %s: %s\n", user_id, query_str);
        bson_free(query_str);

        // Count documents matching the query
        bson_error_t error;
        int64_t count = mongoc_collection_count_documents(tasks_collection, query, NULL, NULL, NULL, &error);
        printf("Number of unfinished tasks for user %s: %lld\n", user_id, (long long)count);

        bson_destroy(query);

        if (count > 0) {
            printf("User %s has %lld unfinished tasks - cannot remove from project\n", user_id, (long long)count);
            mongoc_collection_destroy(tasks_collection);
            Cleanup(repo);
            fclose(log);
            printf("=== Finished check_members_unfinished_tasks (BLOCKED) ===\n\n");
            return 1; // Return 1 if any user has unfinished tasks
        }
    }

    printf("All removed members have no unfinished tasks - removal allowed\n");
    mongoc_collection_destroy(tasks_collection);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished check_members_unfinished_tasks (ALLOWED) ===\n\n");
    
    return 0; // Return 0 if all users can be safely removed
}

int check_project_tasks_completion(const char* project_id) {
    printf("\n=== Starting check_project_tasks_completion ===\n");
    printf("Checking completion status for project %s\n", project_id);
    
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return PROJECT_ACTIVE; // Return active if error - safer default
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        fclose(log);
        return PROJECT_ACTIVE; // Return active if error - safer default
    }

    // Connect to tasks database
    const char* db_name = "tasks";
    const char* collection_name = "tasks";
    mongoc_collection_t* tasks_collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // First, check if project has any tasks at all
    bson_t* all_tasks_query = bson_new();
    BSON_APPEND_UTF8(all_tasks_query, "project_id", project_id);
    
    bson_error_t error;
    int64_t total_tasks = mongoc_collection_count_documents(tasks_collection, all_tasks_query, NULL, NULL, NULL, &error);
    printf("Total tasks in project: %lld\n", (long long)total_tasks);
    
    if (total_tasks == 0) {
        printf("Project has no tasks - can be deleted\n");
        bson_destroy(all_tasks_query);
        mongoc_collection_destroy(tasks_collection);
        Cleanup(repo);
        fclose(log);
        printf("=== Finished check_project_tasks_completion (NO TASKS) ===\n\n");
        return PROJECT_COMPLETED; // No tasks means project can be considered complete
    }

    // Check for unfinished tasks (status != 2)
    bson_t* unfinished_query = bson_new();
    BSON_APPEND_UTF8(unfinished_query, "project_id", project_id);
    
    bson_t ne_condition;
    BSON_APPEND_DOCUMENT_BEGIN(unfinished_query, "status", &ne_condition);
    BSON_APPEND_INT32(&ne_condition, "$ne", 2); // STATUS_COMPLETED = 2
    bson_append_document_end(unfinished_query, &ne_condition);

    int64_t unfinished_tasks = mongoc_collection_count_documents(tasks_collection, unfinished_query, NULL, NULL, NULL, &error);
    printf("Unfinished tasks in project: %lld\n", (long long)unfinished_tasks);

    bson_destroy(all_tasks_query);
    bson_destroy(unfinished_query);
    mongoc_collection_destroy(tasks_collection);
    Cleanup(repo);
    fclose(log);

    if (unfinished_tasks > 0) {
        printf("Project has %lld unfinished tasks - cannot be deleted\n", (long long)unfinished_tasks);
        printf("=== Finished check_project_tasks_completion (ACTIVE) ===\n\n");
        return PROJECT_ACTIVE;
    } else {
        printf("All tasks are completed - project can be deleted\n");
        printf("=== Finished check_project_tasks_completion (COMPLETED) ===\n\n");
        return PROJECT_COMPLETED;
    }
}

int update_project_status(const char* project_id, ProjectStatus status) {
    printf("\n=== Starting update_project_status ===\n");
    printf("Updating project %s status to %d\n", project_id, status);
    
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

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create query to find the project by ID
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, project_id);
    BSON_APPEND_OID(query, "_id", &oid);

    // Create update document
    bson_t* update = bson_new();
    bson_t set;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set);
    BSON_APPEND_INT32(&set, "status", status);
    bson_append_document_end(update, &set);

    bson_error_t error;
    if (!mongoc_collection_update_one(repo->collection, query, update, NULL, NULL, &error)) {
        printf("Error updating project status: %s\n", error.message);
        bson_destroy(query);
        bson_destroy(update);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    printf("Project status updated successfully\n");
    bson_destroy(query);
    bson_destroy(update);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished update_project_status ===\n\n");
    return 0;
}

int delete_project(const char* project_id) {
    printf("\n=== Starting delete_project ===\n");
    printf("Attempting to delete project %s\n", project_id);
    
    // First check if project can be deleted
    int completion_status = check_project_tasks_completion(project_id);
    if (completion_status != PROJECT_COMPLETED) {
        printf("Project cannot be deleted - has unfinished tasks\n");
        printf("=== Finished delete_project (BLOCKED) ===\n\n");
        return 1; // Cannot delete
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

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    // Create query to find the project by ID
    bson_t* query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, project_id);
    BSON_APPEND_OID(query, "_id", &oid);

    bson_error_t error;
    bool delete_result = mongoc_collection_delete_one(repo->collection, query, NULL, NULL, &error);
    if (!delete_result) {
        printf("Error deleting project: %s\n", error.message);
        bson_destroy(query);
        Cleanup(repo);
        fclose(log);
        return 1;
    }

    printf("Project deleted successfully\n");
    bson_destroy(query);
    Cleanup(repo);
    fclose(log);
    printf("=== Finished delete_project (SUCCESS) ===\n\n");
    return 0;
}

int repo() {
    printf("Initializing MongoDB...\n");
    mongoc_init();
    printf("MongoDB initialized.\n");

    getchar();
    mongoc_cleanup();
    printf("MongoDB cleanup completed.\n");

    return 0;
}
