/*
 * Implementation of the print queue manager for the Hydrogen 3D printer.
 * 
 * Processes 3D print jobs from a priority queue, handling job metadata in
 * JSON format including file information and print parameters. Features
 * include job status logging, graceful shutdown with job preservation,
 * and cleanup of incomplete jobs.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include "print_queue_manager.h"
#include "queue.h"
#include "configuration.h"
#include "logging.h"

extern volatile sig_atomic_t print_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

Queue* print_queue = NULL;

static void cleanup_print_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    log_this("PrintQueueManager", "Shutdown: Cleaning up Print Queue Manager", 0, true, true, true);
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

int init_print_queue(void) {
    QueueAttributes print_queue_attrs = {0}; // Initialize with default values for now
    print_queue = queue_create("PrintQueue", &print_queue_attrs);
    if (!print_queue) {
        log_this("PrintQueueManager", "Failed to create PrintQueue", 3, true, true, true);
        return 0;
    }
    log_this("PrintQueueManager", "PrintQueue created successfully", 0, true, true, true);
    return 1;
}

void* print_queue_manager(void* arg) {
    (void)arg; // Unused parameter

    pthread_cleanup_push(cleanup_print_queue_manager, NULL);

    log_this("PrintQueueManager", "Print queue manager started", 0, true, true, true);

    while (!print_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(print_queue) == 0 && !print_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (print_queue_shutdown) {
            log_this("PrintQueueManager", "Shutdown: Print Queue shutdown signal received, processing remaining jobs", 0, true, true, true);
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

    log_this("PrintQueueManager", "Shutdown: Print Queue Manager exiting", 0, true, true, true);

    pthread_cleanup_pop(1);
    return NULL;
}

void shutdown_print_queue() {
    log_this("PrintQueueManager", "Shutdown: Initiating Print Queue shutdown", 0, true, true, true);

    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);

    // Wait for any ongoing job to finish
    usleep(500000); // Wait for 500ms

    // Drain remaining jobs
    size_t remaining = queue_size(print_queue);
    log_this("PrintQueueManager", "Shutdown: Remaining jobs in print queue: %zu", 0, true, true, true, remaining);

    while (queue_size(print_queue) > 0) {
        size_t size;
        int priority;
        char* job = queue_dequeue(print_queue, &size, &priority);
        if (job) {
            log_this("PrintQueueManager", "Shutdown: Drained job: %s", 0, true, true, true, job);
            free(job);
        }
    }

    // Clear the queue contents but don't destroy it
    if (print_queue) {
        queue_clear(print_queue);
    }
    log_this("PrintQueueManager", "Shutdown: Print Queue shutdown complete", 0, true, true, true);
}
