/*
 * Firebase / Cloud Firestore engine - shared types.
 */

#ifndef DATABASE_ENGINE_FIREBASE_TYPES_H
#define DATABASE_ENGINE_FIREBASE_TYPES_H

#include <src/database/database.h>
#include <curl/curl.h>

#define FIREBASE_DEFAULT_EMULATOR_HOST "127.0.0.1"
#define FIREBASE_DEFAULT_EMULATOR_PORT 8080
#define FIREBASE_DEFAULT_PRODUCTION_HOST "firestore.googleapis.com"
#define FIREBASE_DEFAULT_PRODUCTION_PORT 443
#define FIREBASE_DEFAULT_DATABASE "(default)"
#define FIREBASE_MAX_FIELD_BYTES (900 * 1024)
#define FIREBASE_ZONEINFO_DIR "/usr/share/zoneinfo"
#define FIREBASE_SCHEMA_COLLECTION "_schema"

typedef struct FirebaseConnection {
    char* project;
    char* database;
    char* host;
    int port;
    bool emulator;
    char* schema;
    char* base_url;
    CURL* inflight;
    volatile bool abort_requested;
    bool in_transaction;
} FirebaseConnection;

#endif /* DATABASE_ENGINE_FIREBASE_TYPES_H */
