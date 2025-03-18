#ifndef JSON_MAIL_RELAY_H
#define JSON_MAIL_RELAY_H

#include <stdbool.h>
#include <jansson.h>
#include "../config.h"

/**
 * Load mail relay configuration from JSON
 * 
 * @param root The JSON root object containing configuration
 * @param config The application configuration structure to populate
 * @return true on success, false on failure
 */
bool load_json_mail_relay(json_t* root, AppConfig* config);

#endif /* JSON_MAIL_RELAY_H */