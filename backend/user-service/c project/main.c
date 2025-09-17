#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include "model.h"
#include "repo.h"
#include "algs.h"
#include "encode.h"
#include "jwt_middleware.h"

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

int parse_search_parameters(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    struct Usersearch* userstruct = (struct Usersearch*)cls;
    printf("number of prefilled nigger results: %d\n", *userstruct->number_of_results);

    if (key && value) {
        if (strcmp(key, "name") == 0) {
            printf(value);
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
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
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
            if (parse_user_from_json(json, &user) == 0) {
                if (adduser(&user) == 0) {
                    const char* response_str = "User data received";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
                        (void*)response_str, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                    MHD_destroy_response(response);
                    printf("KKKKKKKKKKKKK.\n");
                }
                else {
                    const char* error_response = "User not added";
                    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(error_response),
                        (void*)error_response, MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
                    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                    MHD_destroy_response(response);
                }
            }
            else {
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
                printf(role);
                char* jwt;
                size_t jwt_length;

                struct l8w8jwt_encoding_params params;
                l8w8jwt_encoding_params_init(&params);

                params.alg = L8W8JWT_ALG_HS512;

                params.sub = username_or_email->valuestring;
                params.iss = "Trello clone";
                params.aud = role;

                params.iat = l8w8jwt_time(NULL);
                params.exp = l8w8jwt_time(NULL) + 600; /* Set to expire after 10 minutes (600 seconds). */

                //hardcoded for now
                params.secret_key = (unsigned char*)"YoUR sUpEr S3krEt 1337 HMAC kEy HeRE";
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
        printf("number of prefilled nigger results: %d\n", *foundusers.number_of_results);

        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_search_parameters, &foundusers);

        if (!foundusers.number_of_results) {
            printf("nigga\n");
        }
        else {
            printf("faggot\n");
            printf("number of nigger results: %d\n", *foundusers.number_of_results);
        }

        char* response_str = (char*)malloc(1024);
        memset(response_str, 0, 1024);
        response_str[0] = '[';
        int len = 1;
        for (int i = 0; i < *foundusers.number_of_results; ++i) {
            const char* username = foundusers.users[i].username;
            const char* first_name = foundusers.users[i].first_name;
            const char* last_name = foundusers.users[i].last_name;
            char user[255];
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

    // Protected endpoint: Get current user profile (requires authentication)
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

    // Protected endpoint: Admin-only user management (requires MANAGER role)
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

    // Test endpoint: Check token validity (any authenticated user)
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
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
