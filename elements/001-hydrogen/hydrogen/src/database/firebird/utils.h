/*
 * Firebird Database Engine - Utility Functions Header
 *
 * Header file for Firebird utility functions.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_UTILS_H
#define DATABASE_ENGINE_FIREBIRD_UTILS_H

#include <src/database/database.h>

// Utility functions
char* firebird_get_connection_string(const ConnectionConfig* config);
bool  firebird_validate_connection_string(const char* connection_string);
char* firebird_escape_string(const DatabaseHandle* connection, const char* input);

// Connstring parse helper (shared with database_connstring.c)
bool firebird_parse_connstring_url(const char* conn_str, char* host_out, int host_out_sz,
                                    char* port_out, int port_out_sz,
                                    char* path_out, int path_out_sz,
                                    char* user_out, int user_out_sz,
                                    char* pass_out, int pass_out_sz,
                                    char* schema_out, int schema_out_sz);

// isc_status extraction (shared with connection.c)
void firebird_status_to_error(const fb_status_t* status, const char* designator);

#endif // DATABASE_ENGINE_FIREBIRD_UTILS_H
