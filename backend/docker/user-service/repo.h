#ifndef REPO_H
#define REPO_H

#include "model.h"

int adduser(User* user);
int repo();
int check_activation(const char* link);
int parse_credentials_from_json(const cJSON* json, char role[]);
int find_users(const char* name, User users[], int size, int* number_of_results);
int changepassword(const char* username, const char* new_password, const char* old_password);
int find_user_and_send_magic(const char* username);
int check_magic_link(const char* link, char** username);
int cheeky(const char* username, char** role);

#endif