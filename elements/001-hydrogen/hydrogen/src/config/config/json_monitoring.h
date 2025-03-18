#ifndef JSON_MONITORING_H
#define JSON_MONITORING_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load system monitoring configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_monitoring(json_t* root, AppConfig* config);

#endif /* JSON_MONITORING_H */