#ifndef JSON_RESOURCES_H
#define JSON_RESOURCES_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load system resources configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_resources(json_t* root, AppConfig* config);

#endif /* JSON_RESOURCES_H */