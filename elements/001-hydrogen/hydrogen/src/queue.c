#include "queue.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

QueueSystem queue_system;

static unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % QUEUE_HASH_SIZE;
}

void queue_system_init() {
    memset(&queue_system, 0, sizeof(QueueSystem));
    pthread_mutex_init(&queue_system.mutex, NULL);
}

void queue_system_destroy() {
    pthread_mutex_lock(&queue_system.mutex);
    for (int i = 0; i < QUEUE_HASH_SIZE; i++) {
        Queue* queue = queue_system.queues[i];
        while (queue) {
            Queue* next = queue->hash_next;
            queue_system.queues[i] = next;  // Remove from hash table
            if (queue->name) {  // Check if the queue is still valid
                queue_destroy(queue);
            }
            queue = next;
        }
    }
    pthread_mutex_unlock(&queue_system.mutex);
    pthread_mutex_destroy(&queue_system.mutex);
}

Queue* queue_find(const char* name) {
    unsigned int index = hash(name);
    pthread_mutex_lock(&queue_system.mutex);
    Queue* queue = queue_system.queues[index];
    while (queue) {
        if (strcmp(queue->name, name) == 0) {
            pthread_mutex_unlock(&queue_system.mutex);
            return queue;
        }
        queue = queue->hash_next;
    }
    pthread_mutex_unlock(&queue_system.mutex);
    return NULL;
}

Queue* queue_create(const char* name, QueueAttributes* attrs) {
    if (name == NULL || attrs == NULL) {
        return NULL;
    }

    Queue* existing_queue = queue_find(name);
    if (existing_queue) {
        return existing_queue;
    }

    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) {
        return NULL;
    }

    queue->name = strdup(name);
    if (queue->name == NULL) {
        free(queue);
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->memory_used = 0;
    queue->hash_next = NULL;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->name);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->name);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->name);
        free(queue);
        return NULL;
    }

    // TODO: Initialize queue with attributes

    // Add the queue to the hash table
    unsigned int index = hash(name);
    pthread_mutex_lock(&queue_system.mutex);
    queue->hash_next = queue_system.queues[index];
    queue_system.queues[index] = queue;
    pthread_mutex_unlock(&queue_system.mutex);

    // Log the queue creation
    if (strcmp(name, "SystemLog") == 0) {
        log_this("QueueSystem", "SystemLog queue created", 0, true, false, true);
    } else {
        log_this("QueueSystem", "New queue created", 0, true, true, true);
    }

    return queue;
}

void queue_destroy(Queue* queue) {
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->head != NULL) {
        QueueElement* temp = queue->head;
        queue->head = queue->head->next;
        free(temp->data);
        free(temp);
    }

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    free(queue->name);
    free(queue);
}

bool queue_enqueue(Queue* queue, const char* data, size_t size, int priority) {
    if (!queue || !data || size == 0) {
        return false;
    }

    QueueElement* new_element = (QueueElement*)malloc(sizeof(QueueElement));
    if (!new_element) {
        return false;
    }

    new_element->data = malloc(size + 1);  // +1 for null terminator
    if (!new_element->data) {
        free(new_element);
        return false;
    }

    memcpy(new_element->data, data, size);
    new_element->data[size] = '\0';  // Ensure null termination
    new_element->size = size;
    new_element->priority = priority;
    clock_gettime(CLOCK_REALTIME, &new_element->timestamp);
    new_element->next = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->tail) {
        queue->tail->next = new_element;
    } else {
        queue->head = new_element;
    }
    queue->tail = new_element;
    queue->size++;
    queue->memory_used += size;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);

    return true;
} 

char* queue_dequeue(Queue* queue, size_t* size, int* priority) {
    if (!queue || !size || !priority) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->size == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    QueueElement* element = queue->head;
    queue->head = element->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;
    queue->memory_used -= element->size;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    char* data = element->data;
    *size = element->size;
    *priority = element->priority;

    free(element); // Free the QueueElement, but not the data

    return data;
}

size_t queue_size(Queue* queue) {
    if (!queue) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    size_t size = queue->size;
    pthread_mutex_unlock(&queue->mutex);

    return size;
}


size_t queue_memory_usage(Queue* queue) {
    if (!queue) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    size_t memory_used = queue->memory_used;
    pthread_mutex_unlock(&queue->mutex);

    return memory_used;
}

long queue_oldest_element_age(Queue* queue) {
    if (!queue) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    long age_ms = (now.tv_sec - queue->head->timestamp.tv_sec) * 1000 +
                  (now.tv_nsec - queue->head->timestamp.tv_nsec) / 1000000;

    pthread_mutex_unlock(&queue->mutex);

    return age_ms;
}

long queue_youngest_element_age(Queue* queue) {
    if (!queue) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    long age_ms = (now.tv_sec - queue->tail->timestamp.tv_sec) * 1000 +
                  (now.tv_nsec - queue->tail->timestamp.tv_nsec) / 1000000;

    pthread_mutex_unlock(&queue->mutex);

    return age_ms;
}

void queue_clear(Queue* queue) {
    if (!queue) return;

    pthread_mutex_lock(&queue->mutex);
    while (queue->head) {
        QueueElement* temp = queue->head;
        queue->head = queue->head->next;
        free(temp->data);
        free(temp);
    }
    queue->tail = NULL;
    queue->size = 0;
    queue->memory_used = 0;
    pthread_mutex_unlock(&queue->mutex);
}
