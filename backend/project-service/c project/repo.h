#ifndef REPO_H
#define REPO_H

#include "model.h"

int addproject(Project* project);
int repo();
char* get_all_projects(void);
int update_project_members(const char* project_id, const char** members, int member_count);
char* get_project_by_id(const char* project_id);

#endif