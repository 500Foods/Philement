#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

typedef struct queue_node {
    json_t* data;
    struct queue_node* next;
} queue_node;

struct queue_t {
    queue_node* head;
    queue_node* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

queue_t* queue_init(void) {
    queue_t* queue = malloc(sizeof(queue_t));
    queue->head = queue->tail = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}

void queue_push(queue_t* queue, json_t* item) {
    queue_node* node = malloc(sizeof(queue_node));
    node->data = item;
    node->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if (queue->tail == NULL) {
        queue->head = queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

json_t* queue_pop(queue_t* queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    queue_node* node = queue->head;
    json_t* data = node->data;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&queue->mutex);
    free(node);
    return data;
}

void queue_free(queue_t* queue) {
    while (queue->head != NULL) {
        queue_node* node = queue->head;
        queue->head = node->next;
        json_decref(node->data);
        free(node);
    }
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}
