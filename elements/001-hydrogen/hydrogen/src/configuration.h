/*
 * Configuration system for the Hydrogen 3D printer server.
 * 
 * Defines structures and functions for managing server configuration including
 * web server settings, WebSocket parameters, and mDNS service discovery options.
 * Configuration is loaded from a JSON file and provides defaults when settings
 * are not specified.
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// System Libraries
#include <stddef.h>

// Third-Party Libraries
#include <jansson.h>

// Project Libraries
#include "mdns_server.h"

#define VERSION "0.1.0"
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_LOG_FILE "/var/log/hydrogen.log"
#define DEFAULT_WEB_PORT 5000
#define DEFAULT_WEBSOCKET_PORT 5001
#define DEFAULT_UPLOAD_PATH "/api/upload"
#define DEFAULT_UPLOAD_DIR "/tmp/hydrogen_uploads"
#define DEFAULT_MAX_UPLOAD_SIZE (2ULL * 1024 * 1024 * 1024)  // 2GB
#define NUM_PRIORITY_LEVELS 5

typedef struct {
    int value;
    const char* label;
} PriorityLevel;

typedef struct {
    int port;
    char *web_root;
    char *upload_path;
    char *upload_dir;
    size_t max_upload_size;
    char *log_level;         // Added LogLevel field
} WebConfig;

typedef struct {
    int port;
    char *key;
    char *protocol;
    size_t max_message_size; // bytes
    char *log_level;         // Added LogLevel field
} WebSocketConfig;

typedef struct {
    char *device_id;
    char *friendly_name;
    char *model;
    char *manufacturer;
    char *version;
    mdns_service_t *services;
    size_t num_services;
    char *log_level;         // Added LogLevel field
} mDNSConfig;

typedef struct {
    char *server_name;
    char *executable_path;
    char *log_file_path;
    WebConfig web;
    WebSocketConfig websocket;
    mDNSConfig mdns;
} AppConfig;

extern const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS];
extern int MAX_PRIORITY_LABEL_WIDTH;
extern int MAX_SUBSYSTEM_LABEL_WIDTH;

// Function Prototypes
AppConfig* load_config(const char* config_path);
char* get_executable_path();
long get_file_size(const char* filename);
char* get_file_modification_time(const char* filename);
void create_default_config(const char* config_path);
const char* get_priority_label(int priority);
void calculate_max_priority_label_width();

#endif // CONFIGURATION_H
