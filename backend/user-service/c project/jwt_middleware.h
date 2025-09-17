#ifndef JWT_MIDDLEWARE_H
#define JWT_MIDDLEWARE_H

#include <microhttpd.h>
#include <cjson/cJSON.h>
#include "decode.h"

// Authentication context structure
typedef struct {
    char user_id[50];
    char role[20];
    int is_valid;
    char error_message[256];
} AuthContext;

// RBAC permissions enum
typedef enum {
    PERM_READ_USERS,
    PERM_CREATE_USERS,
    PERM_UPDATE_USERS,
    PERM_DELETE_USERS,
    PERM_ADMIN_ONLY
} Permission;

// Function declarations
int extract_jwt_from_headers(struct MHD_Connection* connection, char* token_out, size_t token_size);
int validate_jwt_token(const char* token, AuthContext* auth);
int check_permission(const AuthContext* auth, Permission required_permission);
int send_unauthorized_response(struct MHD_Connection* connection, const char* message);
int send_forbidden_response(struct MHD_Connection* connection, const char* message);
int authenticate_request(struct MHD_Connection* connection, AuthContext* auth);

// Helper function to create JSON error responses
char* create_error_json(const char* error, const char* message);

#endif

