#ifndef REPO_H
#define REPO_H

#include "model.h"

int addproject(Project* project);
int repo();
char* get_all_projects(void);
// Add this new function declaration
int update_project_members(const char* project_id, const char** members, int member_count);

#endif