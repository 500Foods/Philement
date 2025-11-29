/*
 * Mock webserver core functions implementation for unit testing
 */

#include "mock_webserver_core.h"
#include <string.h>
#include <stdlib.h>

char* mock_get_payload_subdirectory_path(const PayloadData* payload, const char* subdir, AppConfig* config) {
    (void)config; // Unused in mock

    if (!payload) {
        return NULL;
    }

    if (!subdir || subdir[0] == '\0') {
        return strdup("/mock/payload/");
    }

    // Return mock paths based on subdirectory
    if (strcmp(subdir, "terminal/") == 0) {
        return strdup("/mock/payload/terminal/");
    } else if (strcmp(subdir, "swagger") == 0) {
        return strdup("/mock/payload/swagger");
    } else {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "/mock/payload/%s", subdir);
        return strdup(buffer);
    }
}

char* mock_resolve_filesystem_path(const char* path_spec, AppConfig* config) {
    (void)config; // Unused in mock

    if (!path_spec) {
        return NULL;
    }

    if (path_spec[0] == '/') {
        // Absolute path - return as-is
        return strdup(path_spec);
    }

    if (path_spec[0] == '\0') {
        return strdup("/mock/webroot/");
    }

    // Relative path - prepend mock webroot
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "/mock/webroot/%s", path_spec);
    return strdup(buffer);
}