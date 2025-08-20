// Should be a short list

#include "hydrogen.h"

// Global configuration instance
AppConfig *app_config = NULL;

// Tracks current executable size
long long server_executable_size = 0;

void get_executable_size(char *argv[]) {
    struct stat server_executable;
    if (stat(argv[0], &server_executable) == 0) {
        server_executable_size = (long long)server_executable.st_size;
    } else {    
            server_executable_size = 0;
    }   
}