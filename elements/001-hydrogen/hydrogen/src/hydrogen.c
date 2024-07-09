#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "configuration.h"
#include "logging.h"
#include "queue.h"
#include "log_queue_manager.h"
#include "print_queue_manager.h"
#include "web_interface.h"

// Handle application state in threadsafe manner
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t shutting_down = 0;

volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;
volatile sig_atomic_t log_queue_shutdown = 0;

pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;
    
// Queue Threads
pthread_t log_thread;
pthread_t print_queue_thread;

// Local data
char* server_name = NULL;
char* executable_path = NULL;
char* upload_path = NULL;
char* upload_dir = NULL;
char* log_file_path = NULL;

// Ctrl+C handler
void inthandler(int signum) {
    (void)signum; 

    printf("\n");
    log_this("Shutdown", "Cleaning up and shutting down", 0, true, true, true);
    usleep(10000);

    pthread_mutex_lock(&terminate_mutex);
    keep_running = 0;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
}

// When it is time to go
void graceful_shutdown() {

    // Signal web server to shut down
    log_this("Shutdown", "Web server shutdown initiated", 0, true, true, true);
    usleep(10000);
    web_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    shutdown_web_server();
   
    // Wait for that and then continue without logging to database
    usleep(10000);
    printf("%s\n", LOG_LINE_BREAK);
    
    // Signal print queue to shut down
    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    printf("- Waiting for Print Queue Manager thread to finish\n");
    pthread_join(print_queue_thread, NULL);
    printf("- Print Queue Manager thread finished\n");

    // Drain any remaining jobs in the print queue
    Queue* print_queue = queue_find("PrintQueue");
    if (print_queue) {
        size_t remaining = queue_size(print_queue);
        printf("- Remaining jobs in print queue: %zu\n", remaining);
        while (queue_size(print_queue) > 0) {
            size_t size;
            int priority;
            char* job = queue_dequeue(print_queue, &size, &priority);
            if (job) {
                printf("- Drained job: %s\n", job);
                free(job);
            }
        }
    }
    shutdown_print_queue();

    // Signal log queue to shut down
    log_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    log_this("Shutdown", "Log queue shutdown initiated", 0, true, true, true);

    printf("- Waiting for Log Queue Manager thread to finish\n");
    pthread_join(log_thread, NULL);
    printf("- Log Queue Manager thread finished\n");

    printf("- Closing file logging\n");
    close_file_logging();

    printf("- Destroying queue system\n");
    queue_system_destroy();

    printf("- Destroying condition variable and mutex\n");
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_destroy(&terminate_mutex);

    // Cleanup these
    free(server_name);
    free(executable_path);
    free(upload_path);
    free(upload_dir);
    free(log_file_path);

    printf("- Cleanup and shutdown complete\n");
    printf("%s\n", LOG_LINE_BREAK);

    shutting_down = 1;
}

int main(int argc, char *argv[]) {
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Set up interrupt handler
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = inthandler;
    sigaction(SIGINT, &act, NULL);

    // Temp storage for output messages
    char buffer[256];
   
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

    // Load configuration
    char* config_path = (argc > 1) ? argv[1] : "hydrogen.json";
    json_t* root = load_config(config_path);

    server_name = strdup(DEFAULT_SERVER_NAME);
    unsigned int web_port = DEFAULT_WEB_PORT;
    upload_path = strdup(DEFAULT_UPLOAD_PATH);
    upload_dir = strdup(DEFAULT_UPLOAD_DIR);
    size_t config_max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;

    if (root) {
        json_t* name = json_object_get(root, "ServerName");
        if (json_is_string(name)) {
	    free(server_name);
            server_name = strdup(json_string_value(name));
        }

        json_t* log_file = json_object_get(root, "LogFile");
        if (json_is_string(log_file)) {
            free(log_file_path);
            log_file_path = strdup(json_string_value(log_file));
        }

        json_t* port = json_object_get(root, "WebPort");
        if (json_is_integer(port)) {
            web_port = json_integer_value(port);
        }

        json_t* path = json_object_get(root, "UploadPath");
        if (json_is_string(path)) {
            free(upload_path);
            upload_path = strdup(json_string_value(path));
        }

        json_t* dir = json_object_get(root, "UploadDir");
        if (json_is_string(dir)) {
            free(upload_dir);
            upload_dir = strdup(json_string_value(dir));
        }

	json_t* max_size = json_object_get(root, "MaxUploadSize");
        if (json_is_integer(max_size)) {
            config_max_upload_size = (size_t)json_integer_value(max_size);
        }

        json_decref(root);
    }

    // LogFile not supplied, use /var/log/hydrogen.log by default
    if (log_file_path == NULL) {
        log_file_path = strdup(DEFAULT_LOG_FILE);
    }

    // Initialize file logging
    FILE* test_file = fopen(log_file_path, "a");
    if (test_file == NULL) {
        log_this("Initialization", "Unable to write to supplied log file:", 1, true, false, false);
        snprintf(buffer, sizeof(buffer), " => %s", log_file_path);
        log_this("Initialization", buffer, 1, true, false, false);
        free(log_file_path);
        log_file_path = strdup("hydrogen.log");
        test_file = fopen(log_file_path, "a");
	if (test_file == NULL) {
            log_this("Initialization", "Unable to write to fallback log file", 1, true, false, false);
            free(log_file_path);
            log_file_path = NULL;
        }
    }
    if (test_file != NULL) {
        fclose(test_file);
    }
    init_file_logging(log_file_path);


    log_this("Initialization", "Configuration loaded", 0, true, true, true);

    // Launch log queue manager
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        fprintf(stderr, "Failed to start log queue manager thread\n");
        queue_destroy(system_log_queue);
        queue_system_destroy();
        return 1;
    }

    
    // Output app information
    snprintf(buffer, sizeof(buffer), LOG_LINE_BREAK);
    log_this("Initialization", buffer, 0, true, true, true);

    snprintf(buffer, sizeof(buffer), "Server Name: %s", server_name);
    log_this("Initialization", buffer, 0, true, true, true);

    executable_path = get_executable_path();
    if (!executable_path) {
        log_this("Initialization", "Error: Unable to get executable path", 0, true, false, true);
        queue_system_destroy();
        return 1;
    }

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

    snprintf(buffer, sizeof(buffer), "Log File: %s", log_file_path ? log_file_path : "None");
    log_this("Initialization", buffer, 0, true, true, true);

    snprintf(buffer, sizeof(buffer), LOG_LINE_BREAK);
    log_this("Initialization", buffer, 0, true, true, true);

    // Initialize print queue
    init_print_queue();

    // Launch print queue manager
    if (pthread_create(&print_queue_thread, NULL, print_queue_manager, NULL) != 0) {
        log_this("Initialization", "Failed to start print queue manager thread", 4, true, true, true);
        // Cleanup code
        queue_destroy(system_log_queue);
        queue_system_destroy();
        return 1;
    }

    // Initialize web server
    if (!init_web_server(web_port, upload_path, upload_dir, config_max_upload_size)) {
        log_this("Initialization", "Failed to initialize web server", 4, true, true, true);
        // Cleanup code
        free(upload_path);
        free(upload_dir);
        queue_system_destroy();
        close_file_logging();
        free(log_file_path);
        free(server_name);
        free(executable_path);
        return 1;
    }

    // Initialize web server thread
    pthread_t web_thread;
    if (pthread_create(&web_thread, NULL, run_web_server, NULL) != 0) {
        log_this("Initialization", "Failed to start web server thread", 4, true, true, true);
        // Cleanup code
        shutdown_web_server(); // This should clean up resources allocated in init_web_server
        free(upload_path);
        free(upload_dir);
        queue_system_destroy();
        close_file_logging();
        free(log_file_path);
        free(server_name);
        free(executable_path);
        return 1;
    }

    // Ready to go
    // Give threads a moment to launch first
    usleep(10000);
    snprintf(buffer, sizeof(buffer), LOG_LINE_BREAK);
    log_this("Initialization", buffer, 0, true, true, true);
    log_this("Initialization", "Application started", 0, true, true, true);
    log_this("Initialization", "Press Ctrl+C to exit", 0, true, false, true);

    ////////////////////////////////////////////////////////////////////////////////

    while (keep_running) {
        pthread_mutex_lock(&terminate_mutex);
        pthread_cond_wait(&terminate_cond, &terminate_mutex);
        pthread_mutex_unlock(&terminate_mutex);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    
    graceful_shutdown();

    return 0;
}
