/*
 * Configuration utility functions used across modules
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdbool.h>
#include <jansson.h>
#include "env/config_env_utils.h"

// Helper to check if a value contains sensitive information
bool is_sensitive_value(const char* name);

#endif /* CONFIG_UTILS_H */