#ifndef MODEL_H
#define MODEL_H

#include <cjson/cJSON.h>

typedef struct {
    char username[20];
    char first_name[20];
    char last_name[20];
    char email[40];
    char password[50];
    int active;
} User;

int parse_user_from_json(const cJSON* json, User* user);

#endif