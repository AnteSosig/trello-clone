#ifndef REPO_H
#define REPO_H

#include "model.h"

int add_task(Task* task);
char* get_tasks_by_project(const char* project_id);
char* get_user_tasks_by_project(const char* project_id, const char* user_id);
int update_task_members(const char* task_id, const char** members, int member_count);
int update_task_status(const char* task_id, TaskStatus status);
int validate_project_member(const char* project_id, const char* member_id);
int can_user_update_task(const char* task_id, const char* user_id);
int has_unfinished_tasks(const char* project_id, const char* user_id);
int update_project_status_from_tasks(const char* project_id);
int add_member_to_task(const char* task_id, const char* member_id);
int remove_member_from_task(const char* task_id, const char* member_id);
int get_task_project_id(const char* task_id, char* project_id_out);
int get_task_status(const char* task_id);
int repo(void);

#endif