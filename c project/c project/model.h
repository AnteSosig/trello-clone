#ifndef MODEL_H
#define MODEL_H

#include <cjson/cJSON.h>

#define MAX_STRING_LENGTH 100  // Increased from 20 to 100
#define MAX_MEMBERS 50
#define MAX_DATE_LENGTH 10  // For date format "YYYY-MM-DD"

typedef struct {
    char moderator[MAX_STRING_LENGTH];
    char project[MAX_STRING_LENGTH];
    char members[MAX_MEMBERS][MAX_STRING_LENGTH];
    char estimated_completion_date[MAX_DATE_LENGTH + 1];  // +1 for null terminator
    int min_members;
    int max_members;
    int current_member_count;
} Project;

int parse_project_from_json(const cJSON* json, Project* project);

#endif