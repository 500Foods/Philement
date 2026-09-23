/*
 * Firebird Database Engine - Connection Management Header
 *
 * Header file for Firebird connection management functions.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_CONNECTION_H
#define DATABASE_ENGINE_FIREBIRD_CONNECTION_H

#include <src/database/database.h>
#include "types.h"

// Connection lifecycle
bool firebird_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool firebird_disconnect(DatabaseHandle* connection);
bool firebird_health_check(DatabaseHandle* connection);
bool firebird_reset_connection(DatabaseHandle* connection);

// Watchdog cancel hook — implements engine cancel_inflight
void firebird_cancel_inflight(DatabaseHandle* connection);

// Active-statement tracking for watchdog cancel support
void firebird_active_stmt_set(DatabaseHandle* connection, void* stmt_handle);
void firebird_active_stmt_clear(DatabaseHandle* connection, const void* stmt_handle);

// Library loading
bool load_libfbclient_functions(const char* designator);

/*
 * Load libChaCha.so with RTLD_NOW before any other subsystem publishes
 * sha256_init into the global namespace. SQLite's sqlite3_load_extension
 * opens /usr/local/lib/crypto.so with RTLD_GLOBAL, and that library also
 * defines sha256_init. If ChaCha is loaded after that, its relocation
 * binds the wrong function and every Firebird attach fails with
 * "TomCrypt library error initializing sha256: Invalid error code."
 * RTLD_DEEPBIND is not used: AddressSanitizer rejects it.
 */
void firebird_preload_wire_crypt(void);

// Prepared-statement cache helpers
FirebirdConnection* firebird_create_connection_wrapper(void);
void firebird_destroy_connection_wrapper(FirebirdConnection* fb_conn);

// Firebird-specific utility for DPB building
// (shared between utils.c and connection.c)
char* firebird_build_attach_string(const ConnectionConfig* config, size_t* out_dpb_len);

// Firebird-specific utility for connection-string building
// (shared between utils.c and connection.c)
FirebirdConnection* firebird_get_connection_wrapper(DatabaseHandle* connection);

// Designator helper (shared across firebird connection.c functions)
const char* firebird_designator_safe(const DatabaseHandle* connection);

#endif // DATABASE_ENGINE_FIREBIRD_CONNECTION_H
