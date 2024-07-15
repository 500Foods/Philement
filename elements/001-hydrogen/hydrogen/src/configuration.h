#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// Third-Party Libraries
#include <jansson.h>

// Project Libraries
#include "mdns.h"

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
    char *device_id;
    char *friendly_name;
    char *model;
    char *manufacturer;
    char *version;
    mdns_service_t *services;
    int num_services;
} mDNSConfig;

typedef struct {
    char *server_name;
    char *executable_path;
    char *log_file_path;
    int web_port;
    int websocket_port;
    char *upload_path;
    char *upload_dir;
    size_t max_upload_size;
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
