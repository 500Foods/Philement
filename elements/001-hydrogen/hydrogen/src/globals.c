// Should be a short list

#include <src/hydrogen.h>

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

// Global startup log level for filtering during initialization
int startup_log_level = LOG_LEVEL_TRACE;  // Default to TRACE

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

// Initialize startup log level from environment variable
void init_startup_log_level(void) {
    const char* env_level = getenv("HYDROGEN_LOG_LEVEL");
    if (!env_level) {
        // Default to TRACE if not set
        startup_log_level = LOG_LEVEL_TRACE;
        return;
    }

    // Convert to uppercase for case-insensitive comparison
    char upper_level[16];
    size_t len = strlen(env_level);
    if (len >= sizeof(upper_level)) {
        // Too long, default to TRACE
        startup_log_level = LOG_LEVEL_TRACE;
        return;
    }

    for (size_t i = 0; i < len; i++) {
        upper_level[i] = (char)toupper((unsigned char)env_level[i]);
    }
    upper_level[len] = '\0';

    // Convert string to log level
    if (strcmp(upper_level, "TRACE") == 0) {
        startup_log_level = LOG_LEVEL_TRACE;
    } else if (strcmp(upper_level, "DEBUG") == 0) {
        startup_log_level = LOG_LEVEL_DEBUG;
    } else if (strcmp(upper_level, "STATE") == 0) {
        startup_log_level = LOG_LEVEL_STATE;
    } else if (strcmp(upper_level, "ALERT") == 0) {
        startup_log_level = LOG_LEVEL_ALERT;
    } else if (strcmp(upper_level, "ERROR") == 0) {
        startup_log_level = LOG_LEVEL_ERROR;
    } else if (strcmp(upper_level, "FATAL") == 0) {
        startup_log_level = LOG_LEVEL_FATAL;
    } else if (strcmp(upper_level, "QUIET") == 0) {
        startup_log_level = LOG_LEVEL_QUIET;
    } else {
        // Invalid value, default to TRACE
        startup_log_level = LOG_LEVEL_TRACE;
    }
}
