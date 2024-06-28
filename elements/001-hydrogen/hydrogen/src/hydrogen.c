#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "configuration.h"
#include "logging.h"
#include "queue.h"
#include "log_queue_manager.h"

volatile sig_atomic_t keep_running = 1;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

void inthandler(int signum) {
    (void)signum;  // Silence unused parameter warning
    pthread_mutex_lock(&terminate_mutex);
    keep_running = 0;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
}

int main(int argc, char *argv[]) {
    // Initialize the queue system
    queue_system_init();

    // Create the SystemLog queue
    QueueAttributes system_log_attrs = {0}; // Initialize with default values for now
    Queue* system_log_queue = queue_create("SystemLog", &system_log_attrs);
    if (!system_log_queue) {
        fprintf(stderr, "Failed to create SystemLog queue\n");
        queue_system_destroy();
        return 1;
    }

    // Launch log queue manager
    pthread_t log_thread;
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        fprintf(stderr, "Failed to start log queue manager thread\n");
        queue_destroy(system_log_queue);
        queue_system_destroy();
        return 1;
    }


    signal(SIGINT, inthandler);

    log_this("Initialization", "Starting Hydrogen server", 0, true, false, true);

    char* executable_path = get_executable_path();
    if (!executable_path) {
        log_this("Initialization", "Error: Unable to get executable path", 0, true, false, true);
        queue_system_destroy();
        return 1;
    }

    char buffer[256];

    snprintf(buffer, sizeof(buffer), "Executable: %s", executable_path);
    log_this("Initialization", buffer, 0, true, false, true);

    snprintf(buffer, sizeof(buffer), "Version: %s", VERSION);
    log_this("Initialization", buffer, 0, true, false, true);

    long file_size = get_file_size(executable_path);
    if (file_size >= 0) {
        snprintf(buffer, sizeof(buffer), "Size: %ld bytes", file_size);
        log_this("Initialization", buffer, 0, true, false, true);
    } else {
        log_this("Initialization", "Error: Unable to get file size", 0, true, false, true);
    }

    char* mod_time = get_file_modification_time(executable_path);
    if (mod_time) {
        snprintf(buffer, sizeof(buffer), "Last modified: %s", mod_time);
        log_this("Initialization", buffer, 0, true, false, true);
        free(mod_time);
    } else {
        log_this("Initialization", "Error: Unable to get modification time", 0, true, false, true);
    }

    char* config_path = (argc > 1) ? argv[1] : "hydrogen.json";
    json_t* root = load_config(config_path);

    const char* server_name = DEFAULT_SERVER_NAME;

    if (root) {
        json_t* name = json_object_get(root, "ServerName");
        if (json_is_string(name)) {
            server_name = json_string_value(name);
        }
    }

    snprintf(buffer, sizeof(buffer), "Server Name: %s", server_name);
    log_this("Initialization", buffer, 0, true, false, true);
    log_this("Initialization", "Configuration loaded", 0, true, false, true);
    log_this("Initialization", "Press Ctrl+C to exit", 0, true, false, true);

    while (keep_running) {
        pthread_mutex_lock(&terminate_mutex);
        pthread_cond_wait(&terminate_cond, &terminate_mutex);
        pthread_mutex_unlock(&terminate_mutex);
    }

    log_this("Shutdown", "Cleaning up and exiting", 0, true, true, true);

if (root) {
    json_decref(root);
}
free(executable_path);

// Cancel the log queue manager thread
pthread_cancel(log_thread);
pthread_join(log_thread, NULL);

// Clean up the queue system
queue_system_destroy();

// Clean up the condition variable and mutex
pthread_cond_destroy(&terminate_cond);
pthread_mutex_destroy(&terminate_mutex);

		return 0 ;
		}
