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
            queue_destroy(queue);
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

    // Store the queue name for logging
    char* queue_name = strdup(queue->name);
    if (queue_name == NULL) {
        // If we can't allocate memory for the name, just use a generic message
        queue_name = "unknown";
    }

    // Acquire the queue mutex to ensure thread-safety
    pthread_mutex_lock(&queue->mutex);

    // Free all remaining elements in the queue
    QueueElement* current = queue->head;
    while (current != NULL) {
        QueueElement* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }

    // Release the mutex before destroying it
    pthread_mutex_unlock(&queue->mutex);

    // Destroy the mutex and condition variables
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    // Log the queue destruction, but not for SystemLog queue
    if (strcmp(queue_name, "SystemLog") != 0) {
        log_this("QueueSystem", "Queue destroyed", 0, true, true, true);
    }

    // Free the queue name
    free(queue->name);

    // Free the temporary queue name if we allocated it
    if (strcmp(queue_name, "unknown") != 0) {
        free(queue_name);
    }

    // Finally, free the queue structure itself
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
    // TODO: Implement
    return 0;
}

double queue_oldest_element_age(Queue* queue) {
    // TODO: Implement
    return 0.0;
}

double queue_youngest_element_age(Queue* queue) {
    // TODO: Implement
    return 0.0;
}
