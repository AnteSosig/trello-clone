#ifndef REPO_H
#define REPO_H

#include "model.h"

int add_task(Task* task);
char* get_tasks_by_project(const char* project_id);
int update_task_members(const char* task_id, const char** members, int member_count);
int update_task_status(const char* task_id, TaskStatus status);
int validate_project_member(const char* project_id, const char* member_id);
int repo(void);

#endif