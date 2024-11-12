#include "model.h"
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

int parse_project_from_json(const cJSON* json, Project* project) {
    // Clear the entire structure first
    memset(project, 0, sizeof(Project));

    // Get JSON objects
    cJSON* moderator = cJSON_GetObjectItem(json, "moderator");
    cJSON* project_name = cJSON_GetObjectItem(json, "project");
    cJSON* members = cJSON_GetObjectItem(json, "members");
    cJSON* completion_date = cJSON_GetObjectItem(json, "estimated_completion_date");
    cJSON* min_members = cJSON_GetObjectItem(json, "min_members");
    cJSON* max_members = cJSON_GetObjectItem(json, "max_members");

    // Debug print the received values
    printf("Received JSON:\n");
    printf("Moderator: %s (length: %zu)\n", moderator ? moderator->valuestring : "NULL",
        moderator ? strlen(moderator->valuestring) : 0);
    printf("Project: %s (length: %zu)\n", project_name ? project_name->valuestring : "NULL",
        project_name ? strlen(project_name->valuestring) : 0);
    printf("Completion Date: %s (length: %zu)\n", completion_date ? completion_date->valuestring : "NULL",
        completion_date ? strlen(completion_date->valuestring) : 0);
    printf("Min Members: %d\n", min_members ? min_members->valueint : -1);
    printf("Max Members: %d\n", max_members ? max_members->valueint : -1);

    // Validate required fields
    if (!moderator || !project_name || !members || !completion_date ||
        !min_members || !max_members ||
        !cJSON_IsString(moderator) || !cJSON_IsString(project_name) ||
        !cJSON_IsArray(members) || !cJSON_IsString(completion_date) ||
        !cJSON_IsNumber(min_members) || !cJSON_IsNumber(max_members)) {
        printf("Missing or invalid required fields\n");
        if (!moderator) printf("moderator is NULL\n");
        if (!project_name) printf("project_name is NULL\n");
        if (!members) printf("members is NULL\n");
        if (!completion_date) printf("completion_date is NULL\n");
        if (!min_members) printf("min_members is NULL\n");
        if (!max_members) printf("max_members is NULL\n");
        return 1;
    }

    // Validate string lengths with detailed output
    if (strlen(moderator->valuestring) >= MAX_STRING_LENGTH) {
        printf("Moderator string too long: %zu chars (max: %d)\n",
            strlen(moderator->valuestring), MAX_STRING_LENGTH);
        return 1;
    }
    if (strlen(project_name->valuestring) >= MAX_STRING_LENGTH) {
        printf("Project name string too long: %zu chars (max: %d)\n",
            strlen(project_name->valuestring), MAX_STRING_LENGTH);
        return 1;
    }
    // Changed >= to > for date length check
    if (strlen(completion_date->valuestring) > MAX_DATE_LENGTH) {
        printf("Completion date string too long: %zu chars (max: %d)\n",
            strlen(completion_date->valuestring), MAX_DATE_LENGTH);
        return 1;
    }

    // Rest of your code remains the same...

    // Validate member limits
    int min = min_members->valueint;
    int max = max_members->valueint;
    if (min < 1 || max > MAX_MEMBERS || min > max) {
        printf("Invalid member limits (min: %d, max: %d)\n", min, max);
        return 1;
    }

    // Copy basic fields
    strncpy(project->moderator, moderator->valuestring, MAX_STRING_LENGTH - 1);
    strncpy(project->project, project_name->valuestring, MAX_STRING_LENGTH - 1);
    strncpy(project->estimated_completion_date, completion_date->valuestring, MAX_DATE_LENGTH);
    project->min_members = min;
    project->max_members = max;

    // Process members array
    int member_count = cJSON_GetArraySize(members);
    project->current_member_count = 0;

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

        if (member->valuestring[0] != '\0') {
            strncpy(project->members[project->current_member_count],
                member->valuestring,
                MAX_STRING_LENGTH - 1);
            project->current_member_count++;
        }
    }

    // Validate member count against limits
    if (project->current_member_count < project->min_members) {
        printf("Not enough members (current: %d, minimum: %d)\n",
            project->current_member_count, project->min_members);
        return 1;
    }

    return 0;
}