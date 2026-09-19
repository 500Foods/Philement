/*
 * Firebird Database Engine - Interface Header
 *
 * Header file for Firebird engine interface functions.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_INTERFACE_H
#define DATABASE_ENGINE_FIREBIRD_INTERFACE_H

#include <src/database/database.h>

/*
 * Main interface function — returns the Firebird DatabaseEngineInterface
 * vtable. Mirrors db2_get_interface() in db2/interface.h.
 */
DatabaseEngineInterface* firebird_get_interface(void);

/*
 * Engine information functions (for testing and metrics).
 */
const char* firebird_engine_get_version(void);
bool firebird_engine_is_available(void);
const char* firebird_engine_get_description(void);
void firebird_engine_test_functions(void);

#endif // DATABASE_ENGINE_FIREBIRD_INTERFACE_H
