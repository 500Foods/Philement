#ifndef JSON_NOTIFY_H
#define JSON_NOTIFY_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load notification configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_notify(json_t* root, AppConfig* config);

#endif /* JSON_NOTIFY_H */