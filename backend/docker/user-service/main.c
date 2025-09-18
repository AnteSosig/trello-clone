#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"
#include "algs.h"
#include "encode.h"
#include "decode.h"
#include "jwt_middleware.h"
#include "password_validator.h"

#define PORT 8080


struct ConnectionInfo {
    char* json_data;
    size_t json_size;
};

struct Usersearch {
    User* users;
    int size;
    int* number_of_results;
};

int parse_parameters(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    int* is_valid = (int*)cls;

    if (key && value) {
        if (strcmp(key, "link") == 0) {
            int activation_return = check_activation(value);
            printf("returned: %d\n", activation_return);
            if (!activation_return) {
                *is_valid = 1;
            }
        }
    }

    return MHD_YES;
}

int parse_magic_link(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    char** username = (char**)cls;

    if (key && value) {
        if (strcmp(key, "link") == 0) {
            fprintf(stderr, "I love rozen maiden3\n");
            int activation_return = check_magic_link(value, username);
            printf("returned: %d\n", activation_return);
            fprintf(stderr, "returned: %d\n", activation_return);
            fprintf(stderr, "I love rozen maiden4\n");
        }
    }

    return MHD_YES;
}

int parse_recovery_link(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    int* is_valid = (int*)cls;

    if (key && value) {
        if (strcmp(key, "link") == 0) {
            int activation_return = check_recovery_link(value);
            printf("returned: %d\n", activation_return);
            fprintf(stderr, "returned: %d\n", activation_return);
            fprintf(stderr, "Cajevi\n");
            if (!activation_return) {
                *is_valid = 1;
            }
        }
    }

    return MHD_YES;
}

int return_recovery_link(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    char *recovery_link = (char*)cls;

    if (key && value) {
        if (strcmp(key, "link") == 0) {
            snprintf(recovery_link, 41, "%s", value);
            fprintf(stderr, "recovery link %s\n", recovery_link);
            fprintf(stderr, "Herbal\n");
        }
    }

    return MHD_YES;
}

int parse_search_parameters(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    struct Usersearch* userstruct = (struct Usersearch*)cls;
    printf("number of prefilled results: %d\n", *userstruct->number_of_results);

    if (key && value) {
        if (strcmp(key, "name") == 0) {
            printf("%s", value);
            printf("\n");

            int suiseiseki = find_users(value, userstruct->users, userstruct->size, userstruct->number_of_results);
            printf("number of results: %d\n", *userstruct->number_of_results);
            printf("size: %d\n", userstruct->size);

            if (!suiseiseki) {
                if (*userstruct->number_of_results > 0) {
                    printf("First name: %s\n", userstruct->users[0].first_name);
                }
            }
            else {
                printf("Finding users failed\n");
            }
        }
    }
    return MHD_YES;
}

int answer_to_connection(void* cls, struct MHD_Connection* connection,
    const char* url, const char* method, const char* version,
    const char* upload_data, size_t* upload_data_size, void** con_cls) {

    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response* response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Handle "/newuser" POST request
    if (strcmp(url, "/newuser") == 0 && strcmp(method, "POST") == 0) {

        printf("ZLAJAAAAA.\n");

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        printf("KAKIMIIIICS.\n");

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            printf("KRKAAAAAAN.\n");
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                printf("GIMBAAAAAN.\n");
                return MHD_YES;
            }

            User user;
            int parse_result = parse_user_from_json(json, &user);
            if (parse_result == 0) {
                if (adduser(&user) == 0) {
                    const char* response_str = "User data received";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
                        (void*)response_str, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                    MHD_destroy_response(response);
                    printf("KKKKKKKKKKKKK.\n");
                } else {
                    const char* error_response = "User not added";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                        (void*)error_response, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                    MHD_destroy_response(response);
                }
            } else if (parse_result == PASSWORD_TOO_SHORT || parse_result == PASSWORD_TOO_COMMON || parse_result == PASSWORD_INVALID_CHARS) {
                // Handle password validation errors specifically
                const char* error_message = get_password_error_message(parse_result);
                cJSON* error_json = cJSON_CreateObject();
                cJSON_AddStringToObject(error_json, "error", "Password validation failed");
                cJSON_AddStringToObject(error_json, "message", error_message);
                char* error_response = cJSON_Print(error_json);
                
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_MUST_FREE);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                MHD_add_response_header(response, "Content-Type", "application/json");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(error_json);
                printf("Password validation failed: %s\n", error_message);
            } else {
                const char* error_response = "Missing or invalid fields";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
            }

            // Clean up
            cJSON_Delete(json);
            free(conn_info->json_data);
            free(conn_info);
            *con_cls = NULL;
            printf("LLLLLLLLLLLLL.\n");
            return MHD_YES;
        }
    }

    if (strcmp(url, "/profile") == 0 && strcmp(method, "GET") == 0) {
        AuthContext auth;
        
        // Authenticate the request
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check permission to read user data
        if (check_permission(&auth, PERM_READ_USERS) != 0) {
            return send_forbidden_response(connection, "Cannot access user profile");
        }
        
        // Create response with user information
        cJSON* profile = cJSON_CreateObject();
        cJSON_AddStringToObject(profile, "user_id", auth.user_id);
        cJSON_AddStringToObject(profile, "role", auth.role);
        cJSON_AddStringToObject(profile, "status", "authenticated");
        
        char* profile_json = cJSON_Print(profile);
        cJSON_Delete(profile);
        
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(profile_json),
            profile_json,
            MHD_RESPMEM_MUST_COPY
        );
        
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");
        
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        free(profile_json);
        
        return ret;
    }

    if (strcmp(url, "/admin/users") == 0 && strcmp(method, "GET") == 0) {
        AuthContext auth;
        
        // Authenticate the request
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Check admin permission
        if (check_permission(&auth, PERM_ADMIN_ONLY) != 0) {
            return send_forbidden_response(connection, "Admin access required");
        }
        
        // Create admin response
        cJSON* admin_data = cJSON_CreateObject();
        cJSON_AddStringToObject(admin_data, "message", "Admin access granted");
        cJSON_AddStringToObject(admin_data, "admin_user", auth.user_id);
        cJSON_AddStringToObject(admin_data, "role", auth.role);
        
        char* admin_json = cJSON_Print(admin_data);
        cJSON_Delete(admin_data);
        
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(admin_json),
            admin_json,
            MHD_RESPMEM_MUST_COPY
        );
        
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");
        
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        free(admin_json);
        
        return ret;
    }

    if (strcmp(url, "/auth/verify") == 0 && strcmp(method, "GET") == 0) {
        AuthContext auth;
        
        // Authenticate the request
        if (authenticate_request(connection, &auth) != 0) {
            return send_unauthorized_response(connection, auth.error_message);
        }
        
        // Create verification response
        cJSON* verify_data = cJSON_CreateObject();
        cJSON_AddStringToObject(verify_data, "valid", "true");
        cJSON_AddStringToObject(verify_data, "user_id", auth.user_id);
        cJSON_AddStringToObject(verify_data, "role", auth.role);
        
        char* verify_json = cJSON_Print(verify_data);
        cJSON_Delete(verify_data);
        
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(verify_json),
            verify_json,
            MHD_RESPMEM_MUST_COPY
        );
        
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Content-Type", "application/json");
        
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        free(verify_json);
        
        return ret;
    }

    if (strcmp(url, "/activate") == 0) {

        printf("Received GET request for URL path: %s\n", url);
        int is_valid = 0;

        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_parameters, &is_valid);
        printf("is valid: %d\n", is_valid);
        // Parse query string parameters and check them on the fly
        if (is_valid) {
            struct MHD_Response* response;
            const char* response_text = "Account activated";
            response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);

            return ret;
        }
        else {
            struct MHD_Response* response;
            const char* response_text = "Account activation failed";
            response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            return ret;
        }
    }

    if (strcmp(url, "/login") == 0 && strcmp(method, "POST") == 0) {

        printf("ZLAJAAAAA.\n");

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        printf("KAKIMIIIICS.\n");

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            printf("KRKAAAAAAN.\n");
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                printf("GIMBAAAAAN.\n");
                return MHD_YES;
            }

            cJSON* username_or_email = cJSON_GetObjectItem(json, "username_or_email");
            char role[10];
            if (!parse_credentials_from_json(json, role)) {
                printf("%s", role);

                const char *var_name = "HMAC_KEY";
                char *hmac_key = getenv(var_name);

                char* jwt;
                size_t jwt_length;

                struct l8w8jwt_encoding_params params;
                l8w8jwt_encoding_params_init(&params);

                params.alg = L8W8JWT_ALG_HS512;

                params.sub = username_or_email->valuestring;
                params.iss = "Trello Clone";
                params.aud = role;

                params.iat = l8w8jwt_time(NULL);
                params.exp = l8w8jwt_time(NULL) + 600; /* Set to expire after 10 minutes (600 seconds). */

                params.secret_key = (unsigned char*)hmac_key;
                params.secret_key_length = strlen(params.secret_key);

                params.out = &jwt;
                params.out_length = &jwt_length;

                int r = l8w8jwt_encode(&params);

                printf("\n l8w8jwt example HS512 token: %s \n", r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");

                char response_str[512];
                snprintf(response_str, sizeof(response_str), "{\"token\": \"%s\", \"role\": \"%s\", \"expires\": \"%d\"}", jwt, role, 600);
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
                    (void*)response_str, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                printf("KKKKKKKKKKKKK.\n");

                /* Always free the output jwt string! */
                l8w8jwt_free(jwt);
            }
            else {
                const char* error_response = "Invalid info";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
            }

            // Clean up
            cJSON_Delete(json);
            free(conn_info->json_data);
            free(conn_info);
            *con_cls = NULL;
            printf("LLLLLLLLLLLLL.\n");
            return MHD_YES;
        }
    }

    if (strcmp(url, "/finduser") == 0 && strcmp(method, "GET") == 0) {

        printf("oof\n");
        int size = 4;
        int number_of_results = 0;
        User* users = malloc(sizeof(User) * size);
        struct Usersearch foundusers = {
            .users = users,
            .size = size,
            .number_of_results = &number_of_results
        };
        printf("number of prefilled results: %d\n", *foundusers.number_of_results);

        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_search_parameters, &foundusers);

        if (!foundusers.number_of_results) {
            printf("work pls\n");
        }
        else {
            printf("number of results: %d\n", *foundusers.number_of_results);
        }

        char* response_str = (char*)malloc(1024);
        memset(response_str, 0, 1024);
        response_str[0] = '[';
        int len = 1;
        for (int i = 0; i < *foundusers.number_of_results; ++i) {
            const char* username = foundusers.users[i].username;
            const char* first_name = foundusers.users[i].first_name;
            const char* last_name = foundusers.users[i].last_name;
            char user[1024];
            snprintf(user, sizeof(user), "{\"username\": \"%s\", \"first_name\": \"%s\", \"last_name\": \"%s\"},", username, first_name, last_name);
            int userlen = strlen(user);
            for (int j = 0; j < userlen; ++j) {
                response_str[len + j] = user[j];
            }
            len = strlen(response_str);
        }
        if (strlen(response_str) == 1) {
            response_str[len] = ']';
        }
        else {
            response_str[len - 1] = ']';
        }
        printf("%s\n", response_str);

        struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
            (void*)response_str, MHD_RESPMEM_MUST_COPY);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);

        free(users);
        free(response_str);

        return ret;
    }

    if (strcmp(url, "/changepassword") == 0 && strcmp(method, "POST") == 0) {

        printf("ZLAJAAAAA.\n");

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        printf("KAKIMIIIICS.\n");

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            printf("KRKAAAAAAN.\n");
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                printf("GIMBAAAAAN.\n");
                return MHD_YES;
            }

            const char *auth = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

            if (!auth) {
                const char* not_found = "403 - Forbidden";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
                MHD_destroy_response(response);
                return ret;
            }

            const char *var_name = "HMAC_KEY";
            char *hmac_key = getenv(var_name);

            struct l8w8jwt_decoding_params params;
            l8w8jwt_decoding_params_init(&params);

            params.alg = L8W8JWT_ALG_HS512;

            params.jwt = auth;
            params.jwt_length = strlen(auth);

            params.verification_key = (unsigned char*)hmac_key;
            params.verification_key_length = strlen(hmac_key);

            params.validate_iss = "Trello Clone"; 
            printf("KREK CAMUSKI\n");

            params.validate_exp = 1; 
            params.exp_tolerance_seconds = 60;

            params.validate_iat = 1;
            params.iat_tolerance_seconds = 60;

            enum l8w8jwt_validation_result validation_result;

            struct l8w8jwt_claim* claims = NULL;
            size_t claims_len = 0;

            int decode_result = l8w8jwt_decode(&params, &validation_result, &claims, &claims_len);
            printf("decode_result=%d, validation_result=%d, claims_len=%zu\n", decode_result, validation_result, claims_len);
            printf("KREK CRNI\n");

            if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID) 
            {
                printf("\n Example HS512 token validation successful! \n");

                for (size_t i = 0; i < claims_len; ++i) {
                    printf("Claim %zu: %.*s = %.*s\n", i,
                    (int)claims[i].key_length, claims[i].key,
                    (int)claims[i].value_length, claims[i].value);
                }
            }
            else
            {
                printf("\n Example HS512 token validation failed! \n");
                for (size_t i = 0; i < claims_len; ++i) {
                    free(claims[i].key);
                    free(claims[i].value);
                }
                free(claims);

                const char* not_found = "403 - Forbidden";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
                MHD_destroy_response(response);
                return ret;
            }

            cJSON* new_password = cJSON_GetObjectItem(json, "new_password");
            cJSON* old_password = cJSON_GetObjectItem(json, "old_password");
            if (!cJSON_IsString(new_password) || !cJSON_IsString(old_password)) {
                for (size_t i = 0; i < claims_len; ++i) {
                    free(claims[i].key);
                    free(claims[i].value);
                }
                free(claims);

                const char* not_found = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }

            const char *claim_ptr;
            int access = 0;
            for (size_t i = 0; i < claims_len; ++i) {
                if (strcmp(claims[i].key, "sub") == 0) {
                    claim_ptr = claims[i].value;
                }
                if (strcmp(claims[i].key, "aud") == 0) {
                    if (strcmp(claims[i].value, "USER") != 0 && strcmp(claims[i].value, "MANAGER") != 0) {
                        access = 1;
                    }
                }
            }
            if (access == 1) {
                for (size_t i = 0; i < claims_len; ++i) {
                    free(claims[i].key);
                    free(claims[i].value);
                }
                free(claims);

                const char* not_found = "403 - Forbidden";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
                MHD_destroy_response(response);
                return ret;
            }

            int password_change_code = changepassword(claim_ptr, new_password->valuestring, old_password->valuestring);
            if (password_change_code == 0) {
                for (size_t i = 0; i < claims_len; ++i) {
                    free(claims[i].key);
                    free(claims[i].value);
                }
                free(claims);

                const char* not_found = "Password changed";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                return ret;
            }
            else {
                for (size_t i = 0; i < claims_len; ++i) {
                    free(claims[i].key);
                    free(claims[i].value);
                }
                free(claims);

                const char* not_found = "Old password doesn't match";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }

            for (size_t i = 0; i < claims_len; ++i) {
                free(claims[i].key);
                free(claims[i].value);
            }
            free(claims);
        }
    }

    if (strcmp(url, "/magicclaw") == 0 && strcmp(method, "POST") == 0) {
        
        printf("ZLAJAAAAA.\n");

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        printf("KAKIMIIIICS.\n");

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            printf("KRKAAAAAAN.\n");
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                printf("GIMBAAAAAN.\n");
                return MHD_YES;
            }

            cJSON* username_or_email = cJSON_GetObjectItem(json, "username_or_email");
            if (!cJSON_IsString(username_or_email)) {
                const char* not_found = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }

            int find_user_code = find_user_and_send_magic(username_or_email->valuestring);
            fprintf(stderr, "Gonosz Magus\n");
            fprintf(stderr, "Magic exit code %d\n", find_user_code);

            if (find_user_code != 0) {
                const char* not_found = "Account inactive";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }

            const char* not_found = "Magic link sent";
            struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                (void*)not_found, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;

        }
    }

    if (strcmp(url, "/magiclogin") == 0 && strcmp(method, "GET") == 0) {

        printf("Received GET request for URL path: %s\n", url);
        fprintf(stderr, "I love rozen maiden\n");
        char *first_username_char = NULL;

        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_magic_link, &first_username_char);
        fprintf(stderr, "I love rozen maiden1\n");
        printf("Username: %s\n", first_username_char);
        fprintf(stderr, "Username: %s\n", first_username_char);
        fprintf(stderr, "I love rozen maiden77\n");

        if (first_username_char && strcmp(first_username_char, "*") != 0) {

            fprintf(stderr, "I love rozen maiden2\n");
            const char *var_name = "HMAC_KEY";
            char *hmac_key = getenv(var_name);

            char* jwt;
            size_t jwt_length;

            struct l8w8jwt_encoding_params params;
            l8w8jwt_encoding_params_init(&params);

            params.alg = L8W8JWT_ALG_HS512;

            params.sub = first_username_char;
            params.iss = "Trello Clone";
            char *role;
            cheeky(first_username_char, &role);
            params.aud = role;
            fprintf(stderr, "I love rozen maiden88888\n");

            params.iat = l8w8jwt_time(NULL);
            params.exp = l8w8jwt_time(NULL) + 600; /* Set to expire after 10 minutes (600 seconds). */

            params.secret_key = (unsigned char*)hmac_key;
            params.secret_key_length = strlen(params.secret_key);

            params.out = &jwt;
            params.out_length = &jwt_length;

            int r = l8w8jwt_encode(&params);

            printf("\n l8w8jwt example HS512 token: %s \n", r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");
            fprintf(stderr, "I love rozen maiden2232323232\n");

            char response_str[512];
            snprintf(response_str, sizeof(response_str), "{\"token\": \"%s\", \"role\": \"%s\", \"expires\": \"%d\"}", jwt, role, 600);
            struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
                (void*)response_str, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            printf("KKKKKKKKKKKKK.\n");

            /* Always free the output jwt string! */
            l8w8jwt_free(jwt);
            free(first_username_char);

            return ret;
        }
        else if (first_username_char != NULL && !strcmp(first_username_char, "*")) {
            struct MHD_Response* response;
            const char* response_text = "Expired magic link";
            response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            free(first_username_char);

            return ret;
        }
        else {
            struct MHD_Response* response;
            const char* response_text = "Bad request";
            response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            return ret;
        }
    }

    if (strcmp(url, "/recoverpassword") == 0 && strcmp(method, "POST") == 0) {
        
        printf("ZLAJAAAAA.\n");

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        printf("KAKIMIIIICS.\n");

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            printf("KRKAAAAAAN.\n");
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                printf("GIMBAAAAAN.\n");
                return MHD_YES;
            }

            cJSON* email = cJSON_GetObjectItem(json, "email");
            if (!cJSON_IsString(email)) {
                const char* not_found = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }

            int find_recovery_code = find_user_and_send_recovery(email->valuestring);
            if (find_recovery_code == 1) {
                const char* not_found = "Inactive account";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }
            else if (find_recovery_code == 0) {
                const char* not_found = "Email sent";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                return ret;
            }
            else if (find_recovery_code == 3) {
                const char* not_found = "No such user";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }
            else {
                const char* not_found = "Internal server error";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                    (void*)not_found, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                MHD_destroy_response(response);
                return ret;
            }
        }
    }

    if (strcmp(url, "/confirmrecovercode") == 0 && strcmp(method, "GET") == 0) {

        printf("Received GET request for URL path: %s\n", url);
        fprintf(stderr, "I love rozen maiden\n");
        int is_valid = 0;

        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_recovery_link, &is_valid);
        printf("is valid: %d\n", is_valid);
        
        if (is_valid) {
            struct MHD_Response* response;
            const char* response_text = "Valid link";
            response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);

            return ret;
        }
        else {
            struct MHD_Response* response;
            const char* response_text = "Invalid link";
            response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            return ret;
        }
    }

    if (strcmp(url, "/confirmpasswordrecovery") == 0 && strcmp(method, "POST") == 0) {
        
        printf("ZLAJAAAAA.\n");

        if (*con_cls == NULL) {
            struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
            *con_cls = (void*)conn_info;
            return MHD_YES;
        }

        printf("KAKIMIIIICS.\n");

        struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
        // If there's upload data, accumulate it
        if (*upload_data_size > 0) {
            // Reallocate memory to store incoming data
            conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
            memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
            conn_info->json_size += *upload_data_size;
            conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
            *upload_data_size = 0;  // Signal that we've processed this data
            printf("KRKAAAAAAN.\n");
            return MHD_YES;
        }
        else {
            // All data received, process the JSON
            cJSON* json = cJSON_Parse(conn_info->json_data);
            if (json == NULL) {
                const char* error_response = "Invalid JSON format";
                struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                    (void*)error_response, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                cJSON_Delete(json);
                printf("GIMBAAAAAN.\n");
                return MHD_YES;
            }

            printf("Received GET request for URL path: %s\n", url);
            fprintf(stderr, "Received GET request for URL path: %s\n", url);
            int is_valid = 0;

            MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_recovery_link, &is_valid);
            printf("is valid: %d\n", is_valid);
            fprintf(stderr, "is valid: %d\n", is_valid);
            
            if (is_valid) {

                cJSON* new_password = cJSON_GetObjectItem(json, "new_password");
                if (!cJSON_IsString(new_password)) {
                    const char* not_found = "Invalid JSON format";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
                        (void*)not_found, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                    MHD_destroy_response(response);
                    return ret;
                }

                char recovery_link[41];
                MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, return_recovery_link, recovery_link);
                fprintf(stderr, "Recovery link: %s\n", recovery_link);

                if (recovery_link) {

                    int recover_password_code = recoverpassword(recovery_link, new_password->valuestring);
                    fprintf(stderr, "Recovery pls\n");

                    if (!recover_password_code) {
                        struct MHD_Response* response;
                        const char* response_text = "Password changed";
                        response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
                        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                        MHD_destroy_response(response);

                        return ret;
                    }

                    struct MHD_Response* response;
                    const char* response_text = "Internal server error";
                    response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                    MHD_destroy_response(response);

                    return ret;
                }
                else {
                    struct MHD_Response* response;
                    const char* response_text = "Internal server error";
                    response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                    MHD_destroy_response(response);

                    return ret;
                }
            }
            else {
                struct MHD_Response* response;
                const char* response_text = "Invalid link";
                response = MHD_create_response_from_buffer(strlen(response_text), (void*)response_text, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);

                return ret;
            }
        }
    }

    // Handle other routes or return 404
    const char* not_found = "404 - Not Found";
    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
        (void*)not_found, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {

    const char *var_name = "HMAC_KEY";
    char *hmac_key = getenv(var_name);
    if (!hmac_key) {
        fprintf(stderr, "Cannot find HMAC key\n");
        return 1;
    }

    struct MHD_Daemon* daemon;

    // Set the PORT from environment variable, fallback to default PORT 8080
    char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;

    // Start the HTTP server
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
        &answer_to_connection, NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d\n", port);
    repo();

    // Run indefinitely (could also add logic to handle graceful shutdowns)
    pause();

    MHD_stop_daemon(daemon);
    return 0;
}
