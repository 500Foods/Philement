#ifndef JSON_WEBSERVER_H
#define JSON_WEBSERVER_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load webserver configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_webserver(json_t* root, AppConfig* config);

#endif /* JSON_WEBSERVER_H */