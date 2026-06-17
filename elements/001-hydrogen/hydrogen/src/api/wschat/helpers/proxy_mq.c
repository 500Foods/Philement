/*
 * Chat Proxy Multi Queue - Thread-Safe Chunk Queue
 *
 * Thread-safe queue used to transfer JSON chunks from the CURL worker
 * thread (producer) to the LWS service thread (consumer).
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "proxy_multi.h"

// System includes
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Maximum chunks per queue before backpressure
#define MAX_CHUNKS_PER_QUEUE 4096

// Initialize chunk queue
void chunk_queue_init(StreamChunkQueue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
}

// Destroy chunk queue
void chunk_queue_destroy(StreamChunkQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    
    StreamChunkNode* node = queue->head;
    while (node) {
        StreamChunkNode* next = node->next;
        free(node->json_data);
        free(node);
        node = next;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
}

// Enqueue a chunk (producer thread - CURL callback)
bool chunk_queue_enqueue(StreamChunkQueue* queue, const char* json_data, size_t data_len) {
    pthread_mutex_lock(&queue->mutex);
    
    // Check queue size limit (backpressure)
    if (queue->count >= MAX_CHUNKS_PER_QUEUE) {
        pthread_mutex_unlock(&queue->mutex);
        log_this(SR_CHAT, "Chunk queue full (%d), dropping chunk", LOG_LEVEL_ALERT, 1, MAX_CHUNKS_PER_QUEUE);
        return false;
    }
    
    // Warn when queue is getting full (80% capacity)
    if (queue->count >= (MAX_CHUNKS_PER_QUEUE * 80 / 100) && 
        queue->count % 100 == 0) {  // Log every 100th chunk to avoid spam
        log_this(SR_CHAT, "Chunk queue high (%zu/%d)", LOG_LEVEL_STATE, 2, queue->count, MAX_CHUNKS_PER_QUEUE);
    }
    
    // Allocate node
    StreamChunkNode* node = (StreamChunkNode*)malloc(sizeof(StreamChunkNode));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    
    // Allocate and copy data
    node->json_data = (char*)malloc(data_len + 1);
    if (!node->json_data) {
        free(node);
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    
    memcpy(node->json_data, json_data, data_len);
    node->json_data[data_len] = '\0';
    node->data_len = data_len;
    node->next = NULL;
    
    // Add to queue
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->count++;
    
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Dequeue a chunk (consumer thread - LWS callback)
StreamChunkNode* chunk_queue_dequeue(StreamChunkQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    
    StreamChunkNode* node = queue->head;
    if (node) {
        queue->head = node->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
        queue->count--;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return node;
}

// Check if queue has data
bool chunk_queue_has_data(const StreamChunkQueue* queue) {
    // Note: This is a snapshot check, actual data may change immediately after
    return queue->head != NULL;
}

// Get queue count
size_t chunk_queue_get_count(const StreamChunkQueue* queue) {
    return queue->count;
}
