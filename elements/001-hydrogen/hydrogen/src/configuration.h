#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <jansson.h>

#define VERSION "0.1.0"
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_LOG_FILE "/var/log/hydrogen.log"
#define DEFAULT_WEB_PORT 5000
#define DEFAULT_UPLOAD_PATH "/api/upload"
#define DEFAULT_UPLOAD_DIR "/tmp/hydrogen_uploads"
#define DEFAULT_MAX_UPLOAD_SIZE (2ULL * 1024 * 1024 * 1024)  // 2GB
#define NUM_PRIORITY_LEVELS 5

typedef struct {
    int value;
    const char* label;
} PriorityLevel;

extern const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS];
extern int MAX_PRIORITY_LABEL_WIDTH;
extern int MAX_SUBSYSTEM_LABEL_WIDTH;

// Function Prototypes
char* get_executable_path();
long get_file_size(const char* filename);
char* get_file_modification_time(const char* filename);
void create_default_config(const char* config_path);
json_t* load_config(const char* config_path);
const char* get_priority_label(int priority);
void calculate_max_priority_label_width();

#endif // CONFIGURATION_H
