#ifndef JSON_PRINT_QUEUE_H
#define JSON_PRINT_QUEUE_H

#include <stdbool.h>
#include <jansson.h>
#include "../config.h"

/**
 * Load print queue configuration from JSON
 * 
 * @param root The JSON root object containing configuration
 * @param config The application configuration structure to populate
 * @return true on success, false on failure
 */
bool load_json_print_queue(json_t* root, AppConfig* config);

#endif /* JSON_PRINT_QUEUE_H */