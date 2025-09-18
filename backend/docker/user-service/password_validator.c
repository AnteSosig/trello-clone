#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "password_validator.h"

// Maximum number of blacklisted passwords
#define MAX_BLACKLISTED_PASSWORDS 200
#define MAX_PASSWORD_LENGTH 256

// Global array to store blacklisted passwords
static char blacklisted_passwords[MAX_BLACKLISTED_PASSWORDS][MAX_PASSWORD_LENGTH];
static int blacklist_count = 0;
static int validator_initialized = 0;

/**
 * Convert string to lowercase for case-insensitive comparison
 */
static void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

/**
 * Load blacklisted passwords from file
 */
int init_password_validator(void) {
    if (validator_initialized) {
        return 0; // Already initialized
    }

    FILE* file = fopen("common_passwords.txt", "r");
    if (file == NULL) {
    	fprintf(stderr, "Warning: Could not open common_passwords.txt. Password blacklist disabled.\n");
    	return -1;
    }

    char line[MAX_PASSWORD_LENGTH];
    blacklist_count = 0;

    while (fgets(line, sizeof(line), file) && blacklist_count < MAX_BLACKLISTED_PASSWORDS) {
        // Remove newline character
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }

        // Copy password to blacklist (convert to lowercase for comparison)
        strcpy(blacklisted_passwords[blacklist_count], line);
        to_lowercase(blacklisted_passwords[blacklist_count]);
        blacklist_count++;
    }

    fclose(file);
    validator_initialized = 1;
    
    printf("Password validator initialized with %d blacklisted passwords.\n", blacklist_count);
    return 0;
}

/**
 * Cleanup password validator resources
 */
void cleanup_password_validator(void) {
    blacklist_count = 0;
    validator_initialized = 0;
}

/**
 * Check if password is in the blacklist
 */
int is_password_blacklisted(const char* password) {
    if (!validator_initialized || password == NULL) {
        return 0;
    }

    // Create lowercase copy for comparison
    char lowercase_password[MAX_PASSWORD_LENGTH];
    strncpy(lowercase_password, password, sizeof(lowercase_password) - 1);
    lowercase_password[sizeof(lowercase_password) - 1] = '\0';
    to_lowercase(lowercase_password);

    // Check against blacklist
    for (int i = 0; i < blacklist_count; i++) {
        if (strcmp(lowercase_password, blacklisted_passwords[i]) == 0) {
            return 1; // Password is blacklisted
        }
    }

    return 0; // Password not in blacklist
}

/**
 * Validate password against all security rules
 */
int validate_password(const char* password) {
    if (password == NULL) {
        return PASSWORD_INVALID_CHARS;
    }

    // Check minimum length
    size_t len = strlen(password);
    if (len < MIN_PASSWORD_LENGTH) {
        return PASSWORD_TOO_SHORT;
    }

    // Check for invalid characters (basic validation)
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)password[i];
        // Allow printable ASCII characters except space (optional: you can allow spaces)
        if (c < 33 || c > 126) {
            return PASSWORD_INVALID_CHARS;
        }
    }

    // Check blacklist
    if (is_password_blacklisted(password)) {
        return PASSWORD_TOO_COMMON;
    }

    return PASSWORD_VALID;
}

/**
 * Get human-readable error message
 */
const char* get_password_error_message(int validation_result) {
    switch (validation_result) {
        case PASSWORD_VALID:
            return "Password is valid";
        case PASSWORD_TOO_SHORT:
            return "Password must be at least 8 characters long";
        case PASSWORD_TOO_COMMON:
            return "Password is too common. Please choose a stronger password";
        case PASSWORD_INVALID_CHARS:
            return "Password contains invalid characters";
        default:
            return "Unknown password validation error";
    }
}
