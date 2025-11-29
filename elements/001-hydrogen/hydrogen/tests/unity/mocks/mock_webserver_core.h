/*
 * Mock webserver core functions for unit testing
 *
 * This file provides mock implementations for webserver core functions
 * that are difficult to test with real implementations.
 */

#ifndef MOCK_WEBSERVER_CORE_H
#define MOCK_WEBSERVER_CORE_H

#include <src/payload/payload.h>
#include <src/config/config.h>

// Mock function declarations
#ifdef USE_MOCK_WEBSERVER_CORE
#define get_payload_subdirectory_path mock_get_payload_subdirectory_path
#define resolve_filesystem_path mock_resolve_filesystem_path
#endif

char* mock_get_payload_subdirectory_path(const PayloadData* payload, const char* subdir, AppConfig* config);
char* mock_resolve_filesystem_path(const char* path_spec, AppConfig* config);

#endif /* MOCK_WEBSERVER_CORE_H */