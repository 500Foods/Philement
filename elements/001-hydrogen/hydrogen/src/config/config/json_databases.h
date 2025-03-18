#ifndef JSON_DATABASES_H
#define JSON_DATABASES_H

#include <stdbool.h>
#include <jansson.h>
#include "../config.h"

/**
 * Load databases configuration from JSON
 * 
 * @param root The JSON root object containing configuration
 * @param config The application configuration structure to populate
 * @return true on success, false on failure
 */
bool load_json_databases(json_t* root, AppConfig* config);

#endif /* JSON_DATABASES_H */