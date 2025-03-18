#ifndef JSON_LOGGING_H
#define JSON_LOGGING_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load logging configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_logging(json_t* root, AppConfig* config);

#endif /* JSON_LOGGING_H */