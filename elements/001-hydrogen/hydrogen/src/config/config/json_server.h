#ifndef JSON_SERVER_H
#define JSON_SERVER_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load server configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @param config_path The path to the config file
 * @return true on success, false on error
 */
bool load_json_server(json_t* root, AppConfig* config, const char* config_path);

#endif /* JSON_SERVER_H */