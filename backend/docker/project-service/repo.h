#ifndef REPO_H
#define REPO_H

#include "model.h"

int addproject(Project* project);
int repo();
char* get_all_projects(void);
char* get_projects_by_user_role(const char* user_id, const char* role);
int update_project_members(const char* project_id, const char** members, int member_count);
char* get_project_by_id(const char* project_id);
int check_user_project_access(const char* user_id, const char* role, const char* project_id);
int check_members_unfinished_tasks(const char* project_id, const char** removed_members, int removed_count);
int check_project_tasks_completion(const char* project_id);
int update_project_status(const char* project_id, ProjectStatus status);
int delete_project(const char* project_id);

#endif