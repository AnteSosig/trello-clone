#include "model.h"
#include <string.h>
#include <cjson/cJSON.h>

int parse_user_from_json(const cJSON* json, User* user) {
    // Check if each field exists and is a string
    cJSON* first_name = cJSON_GetObjectItem(json, "first_name");
    cJSON* last_name = cJSON_GetObjectItem(json, "last_name");
    cJSON* email = cJSON_GetObjectItem(json, "email");
    cJSON* password = cJSON_GetObjectItem(json, "password");

    if (!cJSON_IsString(first_name) || !cJSON_IsString(last_name) ||
        !cJSON_IsString(email) || !cJSON_IsString(password)) {
        return 1;  // Validation failed
    }

    // Copy the values into the User struct, ensuring they fit the defined size
    strncpy(user->first_name, first_name->valuestring, sizeof(user->first_name) - 1);
    user->first_name[sizeof(user->first_name) - 1] = '\0';

    strncpy(user->last_name, last_name->valuestring, sizeof(user->last_name) - 1);
    user->last_name[sizeof(user->last_name) - 1] = '\0';

    strncpy(user->email, email->valuestring, sizeof(user->email) - 1);
    user->email[sizeof(user->email) - 1] = '\0';

    strncpy(user->password, password->valuestring, sizeof(user->password) - 1);
    user->password[sizeof(user->password) - 1] = '\0';

    return 0;  // Validation and population successful
}