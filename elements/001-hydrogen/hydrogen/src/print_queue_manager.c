#include "print_queue_manager.h"
#include "queue.h"
#include "configuration.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <jansson.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
extern volatile sig_atomic_t print_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

static Queue* print_queue = NULL;

static void cleanup_print_queue_manager(void* arg);
static void process_print_job(const char* job_data);

void init_print_queue() {
    QueueAttributes print_queue_attrs = {0}; // Initialize with default values for now
    print_queue = queue_create("PrintQueue", &print_queue_attrs);
    if (!print_queue) {
        log_this("PrintQueueManager", "Failed to create PrintQueue", 3, true, true, true);
    } else {
        log_this("PrintQueueManager", "PrintQueue created successfully", 0, true, true, true);
    }
}

void shutdown_print_queue() {
    printf("- Initiating Print Queue shutdown\n");
    print_queue = NULL;
    printf("- Print Queue shutdown complete\n");
}

void* print_queue_manager(void* arg) {
    (void)arg; // Unused parameter

    pthread_cleanup_push(cleanup_print_queue_manager, NULL);

    log_this("PrintQueueManager", "Print queue manager started", 0, true, true, true);

    while (!print_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        if (queue_size(print_queue) == 0 && !print_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (print_queue_shutdown) {
	    printf("- Print Queue shutdown signal received, processing remaining jobs\n");
        }

        while (queue_size(print_queue) > 0) {
            size_t job_size;
            int priority;
            char* job_data = queue_dequeue(print_queue, &job_size, &priority);
            if (job_data) {
                process_print_job(job_data);
                free(job_data);
            }
        }
    }

    printf("- Print Queue Manager exiting\n");

    pthread_cleanup_pop(1);
    return NULL;
}

static void cleanup_print_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    printf("- Cleaning up Print Queue Manager\n");
    // Add any necessary cleanup code here
}

static void process_print_job(const char* job_data) {
    if (!job_data) {
        log_this("PrintQueueManager", "Received null job data", 3, true, false, true);
        return;
    }

    json_error_t error;
    json_t* json = json_loads(job_data, 0, &error);
    if (!json) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to parse job JSON: %s", error.text);
        log_this("PrintQueueManager", error_msg, 3, true, false, true);
        return;
    }

    const char* original_filename = json_string_value(json_object_get(json, "original_filename"));
    const char* new_filename = json_string_value(json_object_get(json, "new_filename"));
    json_int_t file_size = json_integer_value(json_object_get(json, "file_size"));

    char log_buffer[256];
    snprintf(log_buffer, sizeof(log_buffer), "Processing print job: %s (original: %s), size: %lld bytes",
             new_filename ? new_filename : "unknown",
             original_filename ? original_filename : "unknown",
             (long long)file_size);
    log_this("PrintQueueManager", log_buffer, 0, true, false, true);

    // TODO: Implement actual print job processing here

    json_decref(json);
}

