#ifndef MODEL_H
#define MODEL_H

#include <cjson/cJSON.h>

#define MAX_STRING_LENGTH 100
#define MAX_MEMBERS 50
#define MAX_DESCRIPTION_LENGTH 500

#ifdef STATUS_PENDING
#undef STATUS_PENDING
#endif

typedef int TaskStatus;
#define STATUS_PENDING 0
#define STATUS_IN_PROGRESS 1
#define STATUS_COMPLETED 2

typedef struct Task {
    char task_id[MAX_STRING_LENGTH];
    char project_id[MAX_STRING_LENGTH];
    char name[MAX_STRING_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    char members[MAX_MEMBERS][MAX_STRING_LENGTH];
    int member_count;
    TaskStatus status;
    char creator_id[MAX_STRING_LENGTH];
} Task;

int parse_task_from_json(const cJSON* json, Task* task);

#endif