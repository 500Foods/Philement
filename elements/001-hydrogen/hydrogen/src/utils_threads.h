/*
 * Thread management and metrics tracking utilities.
 * 
 * Provides thread management functionality including:
 * - Thread registration and tracking
 * - Memory metrics collection
 * - Thread synchronization
 * - Stack size monitoring
 */

#ifndef UTILS_THREADS_H
#define UTILS_THREADS_H

// System includes
#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>

// Constants
#define MAX_SERVICE_THREADS 32

// Memory metrics structure
typedef struct {
    size_t virtual_bytes;     // Virtual memory usage in bytes
    size_t resident_bytes;    // Resident memory usage in bytes
} ThreadMemoryMetrics;

// Service thread and memory information
typedef struct {
    pthread_t thread_ids[MAX_SERVICE_THREADS];  // Array of pthread IDs
    pid_t thread_tids[MAX_SERVICE_THREADS];     // Array of Linux thread IDs
    int thread_count;                           // Number of active threads
    size_t virtual_memory;                      // Total virtual memory for service
    size_t resident_memory;                     // Total resident memory for service
    ThreadMemoryMetrics thread_metrics[MAX_SERVICE_THREADS];  // Memory metrics per thread
    double memory_percent;                      // Percentage of total process memory
} ServiceThreads;

// Global tracking structures
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_threads;
extern ServiceThreads print_threads;

// Thread management functions
void init_service_threads(ServiceThreads *threads);
void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id);

// Memory tracking functions
void update_service_thread_metrics(ServiceThreads *threads);
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id);

#endif // UTILS_THREADS_H