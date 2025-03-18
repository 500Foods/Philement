#ifndef JSON_API_H
#define JSON_API_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load API configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_api(json_t* root, AppConfig* config);

#endif /* JSON_API_H */