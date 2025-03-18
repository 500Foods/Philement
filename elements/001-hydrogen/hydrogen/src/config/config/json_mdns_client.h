#ifndef JSON_MDNS_CLIENT_H
#define JSON_MDNS_CLIENT_H

#include <stdbool.h>
#include <jansson.h>
#include "../config.h"

/**
 * Load mDNS client configuration from JSON
 * 
 * @param root The JSON root object
 * @param config The configuration structure to populate
 * @return true on success, false on failure
 */
bool load_json_mdns_client(json_t* root, AppConfig* config);

#endif /* JSON_MDNS_CLIENT_H */