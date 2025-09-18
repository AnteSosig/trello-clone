#ifndef PASSWORD_VALIDATOR_H
#define PASSWORD_VALIDATOR_H

#ifdef __cplusplus
extern "C" {
#endif

// Password validation result codes
#define PASSWORD_VALID 0
#define PASSWORD_TOO_SHORT -1
#define PASSWORD_TOO_COMMON -2
#define PASSWORD_INVALID_CHARS -3

// Minimum password length
#define MIN_PASSWORD_LENGTH 8

/**
 * Initialize the password validator by loading the blacklist
 * Returns 0 on success, -1 on failure
 */
int init_password_validator(void);

/**
 * Cleanup password validator resources
 */
void cleanup_password_validator(void);

/**
 * Validate a password against security rules
 * Returns:
 * - PASSWORD_VALID (0) if password is valid
 * - PASSWORD_TOO_SHORT if password is too short
 * - PASSWORD_TOO_COMMON if password is in blacklist
 * - PASSWORD_INVALID_CHARS if password contains invalid characters
 */
int validate_password(const char* password);

/**
 * Check if password is in the common passwords blacklist
 * Returns 1 if password is blacklisted, 0 if not
 */
int is_password_blacklisted(const char* password);

/**
 * Get error message for validation result
 */
const char* get_password_error_message(int validation_result);

#ifdef __cplusplus
}
#endif

#endif // PASSWORD_VALIDATOR_H
