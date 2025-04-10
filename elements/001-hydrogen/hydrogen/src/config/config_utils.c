/*
 * Configuration utility functions implementation
 * 
 * Provides common utilities used across configuration modules:
 * - Sensitive value detection
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// Project headers
#include <jansson.h>
#include "config_utils.h"
#include "env/config_env_utils.h"
#include "logging/config_logging_utils.h"
#include "../logging/logging.h"

/*
 * Helper function to detect sensitive configuration values
 * 
 * This function checks if a configuration key contains sensitive terms
 * like "key", "token", "pass", etc. that might contain secrets.
 * 
 * @param name The configuration key name
 * @return true if the name contains a sensitive term, false otherwise
 */
bool is_sensitive_value(const char* name) {
    if (!name) return false;
    
    // Comprehensive list of sensitive terms (merged from multiple implementations)
    const char* sensitive_terms[] = {
        "key", "token", "pass", "secret", "auth", "cred", 
        "cert", "jwt", "seed", "private", "hash", "salt",
        "cipher", "encrypt", "signature", "access"
    };
    const int num_terms = sizeof(sensitive_terms) / sizeof(sensitive_terms[0]);
    
    for (int i = 0; i < num_terms; i++) {
        if (strcasestr(name, sensitive_terms[i])) {
            return true;
        }
    }
    
    return false;
}