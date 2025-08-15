#ifndef REPO_H
#define REPO_H

#include "model.h"

int adduser(User* user);
int repo();
int check_activation(const char* link);
int parse_credentials_from_json(const cJSON* json, char role[]);
int find_users(const char* name, User users[], int size, int* number_of_results);

#endif