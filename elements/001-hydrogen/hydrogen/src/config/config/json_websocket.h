#ifndef JSON_WEBSOCKET_H
#define JSON_WEBSOCKET_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load WebSocketServer configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_websocket(json_t* root, AppConfig* config);

#endif /* JSON_WEBSOCKET_H */