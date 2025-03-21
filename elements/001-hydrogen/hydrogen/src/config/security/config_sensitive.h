/*
 * Configuration security utilities for sensitive value handling
 * 
 * This module provides:
 * - Detection of sensitive configuration parameters
 * - Standardized approach to masking sensitive values in logs
 * - Common security patterns across the configuration system
 */

#ifndef CONFIG_SENSITIVE_H
#define CONFIG_SENSITIVE_H

#include <stdbool.h>

/*
 * Helper function to detect sensitive configuration values
 * 
 * This function checks if a configuration key contains sensitive terms
 * like "key", "token", "pass", etc. that might contain secrets.
 * 
 * @param name The configuration key name
 * @return true if the name contains a sensitive term, false otherwise
 */
bool is_sensitive_value(const char* name);

#endif /* CONFIG_SENSITIVE_H */