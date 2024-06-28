#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <jansson.h>

#define VERSION "0.1.0"
#define DEFAULT_SERVER_NAME "Philement/hydrogen"

 typedef struct {
    int value;
    const char* label;
} PriorityLevel;
#define NUM_PRIORITY_LEVELS 5
extern const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS];
extern int MAX_PRIORITY_LABEL_WIDTH;

char* get_executable_path();
long get_file_size(const char* filename);
char* get_file_modification_time(const char* filename);
void create_default_config(const char* config_path);
json_t* load_config(const char* config_path);
const char* get_priority_label(int priority);
void calculate_max_priority_label_width();

#endif // CONFIGURATION_H
