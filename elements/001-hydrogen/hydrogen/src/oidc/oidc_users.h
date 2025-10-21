/*
 * OIDC User Management
 * 
 * Manages user identities and authentication:
 * - User creation and management
 * - User authentication
 * - Profile information handling
 * - Role and permission management
 */

#ifndef OIDC_USERS_H
#define OIDC_USERS_H

#include <src/globals.h>

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/*
 * User Authentication Level
 * Defines the level of authentication the user has achieved
 */
typedef enum {
    AUTH_LEVEL_NONE,             // Not authenticated
    AUTH_LEVEL_SINGLE_FACTOR,    // Password authentication
    AUTH_LEVEL_TWO_FACTOR,       // Two-factor authentication
    AUTH_LEVEL_MULTI_FACTOR      // Multiple factor authentication
} OIDCAuthLevel;

/*
 * User Account Status
 * Defines the current status of a user account
 */
typedef enum {
    USER_STATUS_ACTIVE,          // Account is active
    USER_STATUS_INACTIVE,        // Account is inactive
    USER_STATUS_LOCKED,          // Account is locked (security measure)
    USER_STATUS_PENDING          // Account pending activation
} OIDCUserStatus;

/*
 * OIDC User
 * Represents a user in the OIDC system
 */
typedef struct {
    char *user_id;               // Unique user identifier
    char *username;              // Username for login
    char *email;                 // User email address
    char *phone_number;          // User phone number (optional)
    char *password_hash;         // Hashed password
    char *salt;                  // Salt used for password hashing
    char *given_name;            // First name
    char *family_name;           // Last name
    char *middle_name;           // Middle name (optional)
    char *nickname;              // Nickname (optional)
    char *preferred_username;    // Preferred username (optional)
    char *profile;               // Profile URL (optional)
    char *picture;               // Picture URL (optional)
    char *website;               // Website URL (optional)
    char *gender;                // Gender (optional)
    char *birthdate;             // Birthdate (optional)
    char *zoneinfo;              // Time zone (optional)
    char *locale;                // Locale (optional)
    char *address;               // Address as JSON (optional)
    char **roles;                // Array of user roles
    size_t role_count;           // Number of roles
    OIDCUserStatus status;       // Account status
    time_t created_at;           // Account creation time
    time_t updated_at;           // Account last update time
    time_t last_login;           // Last login time
    int failed_login_attempts;   // Number of failed login attempts
    OIDCAuthLevel auth_level;    // Current authentication level
} OIDCUser;

/*
 * User Context
 * Main structure for user management operations
 */
typedef struct {
    void *user_storage;          // User storage implementation (opaque)
    size_t user_count;           // Number of registered users
    int max_failed_attempts;     // Max failed login attempts before locking
    bool require_email_verification; // Whether email verification is required
    int password_min_length;     // Minimum password length
} OIDCUserContext;

/*
 * Authentication Result
 * Result of an authentication attempt
 */
typedef struct {
    bool success;                // Whether authentication was successful
    OIDCAuthLevel level;         // Authentication level achieved
    char *user_id;               // User ID (if successful)
    char *error;                 // Error message (if unsuccessful)
} OIDCAuthResult;

/*
 * User Management Functions
 */

/*
 * Initialize user management
 * 
 * @param max_failed_attempts Maximum failed login attempts before locking
 * @param require_email_verification Whether email verification is required
 * @param password_min_length Minimum password length
 * @return Initialized user context or NULL on failure
 */
OIDCUserContext* init_oidc_user_management(int max_failed_attempts,
                                         bool require_email_verification,
                                         int password_min_length);

/*
 * Cleanup user management resources
 * 
 * @param context User context to clean up
 */
void cleanup_oidc_user_management(OIDCUserContext *context);

/*
 * Create a new user
 *
 * @param context User context
 * @param username Username
 * @param email Email address
 * @param password Password (plain text, will be hashed)
 * @param given_name First name
 * @param family_name Last name
 * @return User ID or NULL on failure
 */
char* oidc_create_user(const OIDCUserContext *context, const char *username,
                      const char *email, const char *password,
                      const char *given_name, const char *family_name);

/*
 * Get user by ID
 * 
 * @param context User context
 * @param user_id User ID to find
 * @return User structure or NULL if not found
 */
OIDCUser* oidc_get_user_by_id(OIDCUserContext *context, const char *user_id);

/*
 * Get user by username
 * 
 * @param context User context
 * @param username Username to find
 * @return User structure or NULL if not found
 */
OIDCUser* oidc_get_user_by_username(OIDCUserContext *context, const char *username);

/*
 * Get user by email
 * 
 * @param context User context
 * @param email Email to find
 * @return User structure or NULL if not found
 */
OIDCUser* oidc_get_user_by_email(OIDCUserContext *context, const char *email);

/*
 * Get user information by ID
 *
 * @param context User context
 * @param user_id User ID to look up
 * @return JSON string with user info (caller must free) or NULL on error
 */
char* oidc_get_user_info(const OIDCUserContext *context, const char *user_id);

/*
 * Update user information
 *
 * @param context User context
 * @param user_id User ID to update
 * @param field Field to update
 * @param value New value for field
 * @return Update result (0 for success, error code otherwise)
 */
int oidc_update_user(const OIDCUserContext *context, const char *user_id, const char *field, const char *value);

/*
 * Update user profile
 * 
 * @param context User context
 * @param user_id User ID to update
 * @param field Field name to update
 * @param value New value
 * @return true if update successful, false otherwise
 */
bool oidc_update_user_profile(OIDCUserContext *context, const char *user_id,
                             const char *field, const char *value);

/*
 * Delete a user
 * 
 * @param context User context
 * @param user_id User ID to delete
 * @return true if deletion successful, false otherwise
 */
bool oidc_delete_user(OIDCUserContext *context, const char *user_id);

/*
 * Authenticate a user
 *
 * @param context User context
 * @param username Username or email
 * @param password Password
 * @return Authentication result
 */
OIDCAuthResult oidc_authenticate_user(const OIDCUserContext *context, const char *username,
                                    const char *password);

/*
 * Change user password
 * 
 * @param context User context
 * @param user_id User ID
 * @param current_password Current password
 * @param new_password New password
 * @return true if change successful, false otherwise
 */
bool oidc_change_password(OIDCUserContext *context, const char *user_id,
                         const char *current_password, const char *new_password);

/*
 * Reset user password
 * 
 * @param context User context
 * @param user_id User ID
 * @param reset_token Reset token (from email)
 * @param new_password New password
 * @return true if reset successful, false otherwise
 */
bool oidc_reset_password(OIDCUserContext *context, const char *user_id,
                        const char *reset_token, const char *new_password);

/*
 * Generate password reset token
 * 
 * @param context User context
 * @param email User email
 * @return Reset token or NULL on failure
 */
char* oidc_generate_password_reset_token(OIDCUserContext *context, const char *email);

/*
 * Add role to user
 * 
 * @param context User context
 * @param user_id User ID
 * @param role Role to add
 * @return true if role added successfully, false otherwise
 */
bool oidc_add_user_role(OIDCUserContext *context, const char *user_id, const char *role);

/*
 * Remove role from user
 * 
 * @param context User context
 * @param user_id User ID
 * @param role Role to remove
 * @return true if role removed successfully, false otherwise
 */
bool oidc_remove_user_role(OIDCUserContext *context, const char *user_id, const char *role);

/*
 * Check if user has role
 * 
 * @param context User context
 * @param user_id User ID
 * @param role Role to check
 * @return true if user has role, false otherwise
 */
bool oidc_user_has_role(OIDCUserContext *context, const char *user_id, const char *role);

/*
 * Lock user account
 * 
 * @param context User context
 * @param user_id User ID
 * @return true if account locked successfully, false otherwise
 */
bool oidc_lock_user_account(OIDCUserContext *context, const char *user_id);

/*
 * Unlock user account
 * 
 * @param context User context
 * @param user_id User ID
 * @return true if account unlocked successfully, false otherwise
 */
bool oidc_unlock_user_account(OIDCUserContext *context, const char *user_id);

/*
 * Verify user email
 * 
 * @param context User context
 * @param user_id User ID
 * @param verification_token Verification token (from email)
 * @return true if email verified successfully, false otherwise
 */
bool oidc_verify_user_email(OIDCUserContext *context, const char *user_id,
                           const char *verification_token);

/*
 * Generate email verification token
 * 
 * @param context User context
 * @param user_id User ID
 * @return Verification token or NULL on failure
 */
char* oidc_generate_email_verification_token(OIDCUserContext *context, const char *user_id);

/*
 * Export user profile as claims JSON
 * 
 * @param user User to export
 * @param scope Requested scope (space-separated list)
 * @return JSON string with user claims (caller must free)
 */
char* oidc_export_user_claims(OIDCUser *user, const char *scope);

/*
 * List all users
 * 
 * @param context User context
 * @return JSON array of users (caller must free)
 */
char* oidc_list_users(OIDCUserContext *context);

/*
 * Hash a password
 * 
 * @param password Password to hash
 * @param salt Salt to use (if NULL, a new salt will be generated)
 * @param out_salt Output parameter for salt (if salt was NULL)
 * @return Password hash (caller must free)
 */
char* oidc_hash_password(const char *password, const char *salt, char **out_salt);

/*
 * Validate password complexity
 * 
 * @param context User context
 * @param password Password to validate
 * @return true if password meets complexity requirements, false otherwise
 */
bool oidc_validate_password(OIDCUserContext *context, const char *password);

#endif // OIDC_USERS_H
