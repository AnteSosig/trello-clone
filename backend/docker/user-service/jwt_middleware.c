#include "jwt_middleware.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Same secret key as used in main.c for encoding
static const char* JWT_SECRET = "YoUR sUpEr S3krEt 1337 HMAC kEy HeRE";

/**
 * Extract JWT token from Authorization header
 */
int extract_jwt_from_headers(struct MHD_Connection* connection, char* token_out, size_t token_size) {
    const char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    
    if (!auth_header) {
        return -1; // No Authorization header
    }
    
    // Check if it starts with "Bearer "
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        return -2; // Invalid format
    }
    
    const char* token = auth_header + 7; // Skip "Bearer "
    
    if (strlen(token) >= token_size) {
        return -3; // Token too long
    }
    
    strcpy(token_out, token);
    return 0; // Success
}

/**
 * Validate JWT token and extract user information
 */
int validate_jwt_token(const char* token, AuthContext* auth) {
    struct l8w8jwt_decoding_params params;
    enum l8w8jwt_validation_result validation_result;
    
    // Initialize auth context
    memset(auth, 0, sizeof(AuthContext));
    auth->is_valid = 0;
    
    // Initialize decoding parameters
    l8w8jwt_decoding_params_init(&params);
    
    params.alg = L8W8JWT_ALG_HS512;
    params.jwt = (char*)token;
    params.jwt_length = strlen(token);
    params.verification_key = (unsigned char*)JWT_SECRET;
    params.verification_key_length = strlen(JWT_SECRET);
    
    // Validate current time
    params.validate_iat = 1;
    params.validate_exp = 1;
    params.iat_tolerance_seconds = 10; // 10 seconds tolerance
    
    // Decode and validate the token
    struct l8w8jwt_claim* claims = NULL;
    size_t claims_length = 0;
    
    int result = l8w8jwt_decode(&params, &validation_result, &claims, &claims_length);
    
    if (result != L8W8JWT_SUCCESS) {
        snprintf(auth->error_message, sizeof(auth->error_message), "Token decoding failed");
        goto cleanup;
    }
    
    if (validation_result != L8W8JWT_VALID) {
        snprintf(auth->error_message, sizeof(auth->error_message), "Token validation failed");
        goto cleanup;
    }
    
    // Extract claims
    for (size_t i = 0; i < claims_length; i++) {
        if (strcmp(claims[i].key, "sub") == 0) {
            strncpy(auth->user_id, claims[i].value, sizeof(auth->user_id) - 1);
            auth->user_id[sizeof(auth->user_id) - 1] = '\0';
        } else if (strcmp(claims[i].key, "aud") == 0) {
            strncpy(auth->role, claims[i].value, sizeof(auth->role) - 1);
            auth->role[sizeof(auth->role) - 1] = '\0';
        }
    }
    
    // Validate that we got required claims
    if (strlen(auth->user_id) == 0 || strlen(auth->role) == 0) {
        snprintf(auth->error_message, sizeof(auth->error_message), "Missing required claims");
        goto cleanup;
    }
    
    auth->is_valid = 1;
    
cleanup:
    if (claims) {
        l8w8jwt_free_claims(claims, claims_length);
    }
    
    return auth->is_valid ? 0 : -1;
}

/**
 * Check if user has required permission based on role
 */
int check_permission(const AuthContext* auth, Permission required_permission) {
    if (!auth->is_valid) {
        return -1;
    }
    
    // Define role-based permissions
    if (strcmp(auth->role, "MANAGER") == 0) {
        // Managers have all permissions
        return 0;
    } else if (strcmp(auth->role, "USER") == 0) {
        // Users have limited permissions
        switch (required_permission) {
            case PERM_READ_USERS:
                return 0; // Users can read user data
            case PERM_CREATE_USERS:
            case PERM_UPDATE_USERS:
            case PERM_DELETE_USERS:
            case PERM_ADMIN_ONLY:
                return -1; // Users cannot perform these actions
            default:
                return -1;
        }
    }
    
    return -1; // Unknown role or permission denied
}

/**
 * Send 401 Unauthorized response
 */
int send_unauthorized_response(struct MHD_Connection* connection, const char* message) {
    char* error_json = create_error_json("Unauthorized", message ? message : "Authentication required");
    
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(error_json), 
        error_json, 
        MHD_RESPMEM_MUST_COPY
    );
    
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Content-Type", "application/json");
    
    int ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
    MHD_destroy_response(response);
    free(error_json);
    
    return ret;
}

/**
 * Send 403 Forbidden response
 */
int send_forbidden_response(struct MHD_Connection* connection, const char* message) {
    char* error_json = create_error_json("Forbidden", message ? message : "Insufficient permissions");
    
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(error_json), 
        error_json, 
        MHD_RESPMEM_MUST_COPY
    );
    
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Content-Type", "application/json");
    
    int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
    MHD_destroy_response(response);
    free(error_json);
    
    return ret;
}

/**
 * Main authentication function that combines token extraction and validation
 */
int authenticate_request(struct MHD_Connection* connection, AuthContext* auth) {
    char token[1024];
    
    // Extract token from header
    int extract_result = extract_jwt_from_headers(connection, token, sizeof(token));
    if (extract_result != 0) {
        strcpy(auth->error_message, "Missing or invalid Authorization header");
        auth->is_valid = 0;
        return -1;
    }
    
    // Validate token
    return validate_jwt_token(token, auth);
}

/**
 * Helper function to create JSON error responses
 */
char* create_error_json(const char* error, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON* error_obj = cJSON_CreateString(error);
    cJSON* message_obj = cJSON_CreateString(message);
    
    cJSON_AddItemToObject(json, "error", error_obj);
    cJSON_AddItemToObject(json, "message", message_obj);
    
    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);
    
    return json_string;
}

