// Should be a short list

#include "hydrogen.h"

// Global configuration instance
AppConfig *app_config = NULL;

//Global subsystem registry state
size_t registry_registered = 0;
size_t registry_running = 0;
size_t registry_attempted = 0;
size_t registry_failed = 0;

// Tracks current executable size
long long server_executable_size = 0;

// Global logging variables
int MAX_PRIORITY_LABEL_WIDTH = 5;    // All log level names are 5 characters
int MAX_SUBSYSTEM_LABEL_WIDTH = 18;  // Default minimum width

PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS] = {
    {0, "TRACE"},
    {1, "DEBUG"},
    {2, "STATE"},
    {3, "ALERT"},
    {4, "ERROR"},
    {5, "FATAL"},
    {6, "QUIET"}
};

void get_executable_size(char *argv[]) {
    struct stat server_executable;
    if (argv && argv[0] && stat(argv[0], &server_executable) == 0) {
        server_executable_size = (long long)server_executable.st_size;
    } else {
        server_executable_size = 0;
    }
}
