#ifndef JSON_OIDC_H
#define JSON_OIDC_H

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"

/*
 * Load OIDC configuration from JSON
 * 
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_json_oidc(json_t* root, AppConfig* config);

#endif /* JSON_OIDC_H */