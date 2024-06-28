#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define QUEUE_HASH_SIZE 256

typedef struct QueueElement {
    char* data;
    size_t size;
    int priority;
    struct timespec timestamp;
    struct QueueElement* next;
} QueueElement;

typedef struct Queue {
    char* name;
    QueueElement* head;
    QueueElement* tail;
    size_t size;
    size_t memory_used;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    struct Queue* hash_next;
    int highest_priority;
    struct timespec oldest_element_timestamp;
    struct timespec youngest_element_timestamp;
} Queue;

typedef struct QueueAttributes {
    size_t initial_memory;
    size_t chunk_size;
    size_t warning_limit;
    // Add more attributes as needed
} QueueAttributes;

typedef struct QueueSystem {
    Queue* queues[QUEUE_HASH_SIZE];
    pthread_mutex_t mutex;
} QueueSystem;

extern QueueSystem queue_system;

void queue_system_init();
void queue_system_destroy();
Queue* queue_find(const char* name);
Queue* queue_create(const char* name, QueueAttributes* attrs);
void queue_destroy(Queue* queue);
bool queue_enqueue(Queue* queue, const char* data, size_t size, int priority);
char* queue_dequeue(Queue* queue, size_t* size, int* priority);
size_t queue_size(Queue* queue);
size_t queue_memory_usage(Queue* queue);
double queue_oldest_element_age(Queue* queue);
double queue_youngest_element_age(Queue* queue);

#endif // QUEUE_H
