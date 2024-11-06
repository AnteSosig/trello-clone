#include "model.h"
#include <string.h>
#include <cjson/cJSON.h>

int parse_project_from_json(const cJSON* json, Project* project) {
    // Check if each field exists and is a string
    cJSON* moderator = cJSON_GetObjectItem(json, "moderator");
    cJSON* project_name = cJSON_GetObjectItem(json, "project");
    cJSON* members = cJSON_GetObjectItem(json, "members");

    if (!cJSON_IsString(moderator) || !cJSON_IsString(project_name) || !cJSON_IsArray(members)) {
        return 1;  // Validation failed
    }

    // Copy moderator and project name values into the Project struct
    strncpy(project->moderator, moderator->valuestring, sizeof(project->moderator) - 1);
    project->moderator[sizeof(project->moderator) - 1] = '\0';

    strncpy(project->project, project_name->valuestring, sizeof(project->project) - 1);
    project->project[sizeof(project->project) - 1] = '\0';

    // Copy members into the members array
    int member_count = cJSON_GetArraySize(members);
    for (int i = 0; i < member_count && i < 50; ++i) {
        cJSON* member = cJSON_GetArrayItem(members, i);
        if (cJSON_IsString(member)) {
            strncpy(project->members[i], member->valuestring, sizeof(project->members[i]) - 1);
            project->members[i][sizeof(project->members[i]) - 1] = '\0';
        }
    }

    return 0;  // Validation and population successful
}
