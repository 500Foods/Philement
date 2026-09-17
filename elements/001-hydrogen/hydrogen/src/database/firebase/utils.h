/*
 * Firebase engine - connection-string and string helpers.
 */

#ifndef DATABASE_ENGINE_FIREBASE_UTILS_H
#define DATABASE_ENGINE_FIREBASE_UTILS_H

#include <src/database/database.h>

char* firebase_get_connection_string(const ConnectionConfig* config);
bool firebase_validate_connection_string(const char* connection_string);
char* firebase_escape_string(const DatabaseHandle* connection, const char* input);

bool firebase_host_is_production(const char* host);
bool firebase_config_is_emulator(const ConnectionConfig* config);
const char* firebase_resolved_project(const ConnectionConfig* config);
const char* firebase_resolved_database(const ConnectionConfig* config);
const char* firebase_resolved_host(const ConnectionConfig* config);
int firebase_resolved_port(const ConnectionConfig* config);

#endif /* DATABASE_ENGINE_FIREBASE_UTILS_H */
