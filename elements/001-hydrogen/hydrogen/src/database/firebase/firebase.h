/*
 * Firebase engine - version / description helpers.
 */

#ifndef DATABASE_ENGINE_FIREBASE_FIREBASE_H
#define DATABASE_ENGINE_FIREBASE_FIREBASE_H

const char* firebase_engine_get_version(void);
bool firebase_engine_is_available(void);
const char* firebase_engine_get_description(void);
void firebase_engine_test_functions(void);

#endif /* DATABASE_ENGINE_FIREBASE_FIREBASE_H */
