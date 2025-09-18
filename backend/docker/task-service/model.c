#include "model.h"
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

int parse_task_from_json(const cJSON* json, Task* task) {
    // Clear the structure first
    memset(task, 0, sizeof(Task));

    // Get JSON objects
    cJSON* project_id = cJSON_GetObjectItem(json, "project_id");
    cJSON* name = cJSON_GetObjectItem(json, "name");
    cJSON* description = cJSON_GetObjectItem(json, "description");
    cJSON* members = cJSON_GetObjectItem(json, "members");
    cJSON* creator_id = cJSON_GetObjectItem(json, "creator_id");
    cJSON* status_str = cJSON_GetObjectItem(json, "status");

    // Debug print the received values
    printf("Received JSON:\n");
    printf("Project ID: %s\n", project_id ? project_id->valuestring : "NULL");
    printf("Name: %s\n", name ? name->valuestring : "NULL");
    printf("Description: %s\n", description ? description->valuestring : "NULL");
    printf("Creator ID: %s\n", creator_id ? creator_id->valuestring : "NULL");

    // Validate required fields
    if (!project_id || !name || !description || !members || !creator_id ||
        !cJSON_IsString(project_id) || !cJSON_IsString(name) ||
        !cJSON_IsString(description) || !cJSON_IsArray(members) ||
        !cJSON_IsString(creator_id)) {
        printf("Missing or invalid required fields\n");
        if (!project_id) printf("project_id is NULL\n");
        if (!name) printf("name is NULL\n");
        if (!description) printf("description is NULL\n");
        if (!members) printf("members is NULL\n");
        if (!creator_id) printf("creator_id is NULL\n");
        return 1;
    }

    // Validate string lengths
    if (strlen(project_id->valuestring) >= MAX_STRING_LENGTH) {
        printf("Project ID string too long: %zu chars (max: %d)\n",
            strlen(project_id->valuestring), MAX_STRING_LENGTH);
        return 1;
    }
    if (strlen(name->valuestring) >= MAX_STRING_LENGTH) {
        printf("Name string too long: %zu chars (max: %d)\n",
            strlen(name->valuestring), MAX_STRING_LENGTH);
        return 1;
    }
    if (strlen(description->valuestring) >= MAX_DESCRIPTION_LENGTH) {
        printf("Description string too long: %zu chars (max: %d)\n",
            strlen(description->valuestring), MAX_DESCRIPTION_LENGTH);
        return 1;
    }

    // Copy basic fields
    strncpy(task->project_id, project_id->valuestring, MAX_STRING_LENGTH - 1);
    strncpy(task->name, name->valuestring, MAX_STRING_LENGTH - 1);
    strncpy(task->description, description->valuestring, MAX_DESCRIPTION_LENGTH - 1);
    strncpy(task->creator_id, creator_id->valuestring, MAX_STRING_LENGTH - 1);

    // Set initial status to pending (regardless of what was sent)
    task->status = STATUS_PENDING;

    // Process members array
    int member_count = cJSON_GetArraySize(members);
    task->member_count = 0;

    printf("Processing %d members\n", member_count);

    for (int i = 0; i < member_count && i < MAX_MEMBERS; i++) {
        cJSON* member = cJSON_GetArrayItem(members, i);
        if (!cJSON_IsString(member)) {
            printf("Member %d is not a string\n", i);
            continue;
        }

        if (strlen(member->valuestring) >= MAX_STRING_LENGTH) {
            printf("Member name too long at index %d\n", i);
            continue;
        }

        strncpy(task->members[task->member_count],
            member->valuestring,
            MAX_STRING_LENGTH - 1);
        task->member_count++;
        printf("Added member: %s\n", member->valuestring);
    }

    printf("Successfully parsed task with %d members\n", task->member_count);
    return 0;
}