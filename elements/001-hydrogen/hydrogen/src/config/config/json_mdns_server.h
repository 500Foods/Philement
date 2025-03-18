#ifndef JSON_MDNS_SERVER_H
#define JSON_MDNS_SERVER_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load mDNS server configuration from JSON
 * 
 * This function parses the mDNS server configuration from the JSON root.
 * It populates the MDNSServerConfig structure in the AppConfig.
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_mdns_server(json_t* root, AppConfig* config);

#endif /* JSON_MDNS_SERVER_H */