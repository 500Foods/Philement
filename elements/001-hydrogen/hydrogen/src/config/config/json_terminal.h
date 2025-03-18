#ifndef JSON_TERMINAL_H
#define JSON_TERMINAL_H

#include <stdbool.h>
#include <jansson.h>
#include "../config.h"

/**
 * Load terminal configuration from JSON
 * 
 * @param root The JSON root object containing configuration
 * @param config The application configuration structure to populate
 * @return true on success, false on failure
 */
bool load_json_terminal(json_t* root, AppConfig* config);

#endif /* JSON_TERMINAL_H */