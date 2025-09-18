#ifndef MODEL_H
#define MODEL_H

#include <cjson/cJSON.h>

#define MAX_STRING_LENGTH 100  // Increased from 20 to 100
#define MAX_MEMBERS 50
#define MAX_DATE_LENGTH 10  // For date format "YYYY-MM-DD"

typedef int ProjectStatus;
#define PROJECT_ACTIVE 0      // Project has unfinished tasks or no tasks
#define PROJECT_COMPLETED 1   // All tasks are completed

typedef struct {
    char moderator[MAX_STRING_LENGTH];
    char project[MAX_STRING_LENGTH];
    char members[MAX_MEMBERS][MAX_STRING_LENGTH];
    char estimated_completion_date[MAX_DATE_LENGTH + 1];  // +1 for null terminator
    int min_members;
    int max_members;
    int current_member_count;
    char creator_id[MAX_STRING_LENGTH];
    ProjectStatus status;  // 0 = active, 1 = completed
} Project;

int parse_project_from_json(const cJSON* json, Project* project);

#endif