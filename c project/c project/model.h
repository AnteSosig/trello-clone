#ifndef MODEL_H
#define MODEL_H

#include <cjson/cJSON.h>

typedef struct {
    char moderator[20];
    char project[20];
    char members[50][20];  // Array to store 50 member names (max 20 chars per member)
} Project;

int parse_project_from_json(const cJSON* json, Project* project);

#endif
