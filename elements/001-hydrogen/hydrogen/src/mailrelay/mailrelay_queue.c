/*
 * Mail Relay bounded in-memory queue implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_queue.h>

#include <stdlib.h>
#include <string.h>

static int timespec_compare(const struct timespec* a, const struct timespec* b) {
    if (a->tv_sec < b->tv_sec) return -1;
    if (a->tv_sec > b->tv_sec) return 1;
    if (a->tv_nsec < b->tv_nsec) return -1;
    if (a->tv_nsec > b->tv_nsec) return 1;
    return 0;
}

static bool item_is_due(const MailRelayQueueItem* item, const struct timespec* now) {
    return timespec_compare(&item->next_attempt_at, now) <= 0;
}

/* List is sorted by priority descending, so the first due item is the
 * highest-priority item that is ready to send. */
static MailRelayQueueItem* find_first_due(MailRelayQueue* queue, const struct timespec* now) {
    MailRelayQueueItem* current = queue->head;
    while (current) {
        if (item_is_due(current, now)) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static MailRelayQueueItem* find_earliest(MailRelayQueue* queue) {
    MailRelayQueueItem* earliest = NULL;
    MailRelayQueueItem* current = queue->head;
    while (current) {
        if (!earliest || timespec_compare(&current->next_attempt_at, &earliest->next_attempt_at) < 0) {
            earliest = current;
        }
        current = current->next;
    }
    return earliest;
}

static void remove_item(MailRelayQueue* queue, MailRelayQueueItem* target) {
    if (!queue || !target) {
        return;
    }
    if (queue->head == target) {
        queue->head = target->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
        return;
    }
    MailRelayQueueItem* prev = queue->head;
    while (prev && prev->next != target) {
        prev = prev->next;
    }
    if (prev) {
        prev->next = target->next;
        if (!target->next) {
            queue->tail = prev;
        }
    }
}

MailRelayQueue* mailrelay_queue_create(int capacity) {
    if (capacity <= 0) {
        capacity = MAILRELAY_QUEUE_DEFAULT_CAPACITY;
    }

    MailRelayQueue* queue = calloc(1, sizeof(MailRelayQueue));
    if (!queue) {
        return NULL;
    }

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }

    queue->capacity = capacity;
    queue->size = 0;
    queue->shutdown = false;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void mailrelay_queue_destroy(MailRelayQueue* queue) {
    if (!queue) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = true;

    MailRelayQueueItem* current = queue->head;
    while (current) {
        MailRelayQueueItem* next = current->next;
        mailrelay_message_free(&current->message);
        free(current);
        current = next;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    pthread_mutex_unlock(&queue->mutex);

    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);
    free(queue);
}

MailRelayStatus mailrelay_queue_enqueue_scheduled(MailRelayQueue* queue,
                                                  const MailRelayMessage* message,
                                                  int priority,
                                                  int attempts,
                                                  const struct timespec* next_attempt_at) {
    if (!queue || !message || !next_attempt_at) {
        return MAILRELAY_INVALID_ARGS;
    }

    pthread_mutex_lock(&queue->mutex);
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return MAILRELAY_SHUTDOWN;
    }
    if (queue->size >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return MAILRELAY_QUEUE_FULL;
    }

    MailRelayQueueItem* item = calloc(1, sizeof(MailRelayQueueItem));
    if (!item) {
        pthread_mutex_unlock(&queue->mutex);
        return MAILRELAY_INVALID_ARGS;
    }

    if (!mailrelay_message_copy(&item->message, message)) {
        free(item);
        pthread_mutex_unlock(&queue->mutex);
        return MAILRELAY_INVALID_ARGS;
    }

    item->priority = priority;
    item->attempts = attempts;
    clock_gettime(CLOCK_REALTIME, &item->enqueued_at);
    item->next_attempt_at = *next_attempt_at;
    item->next = NULL;

    // Insert in priority order: higher priority before lower priority,
    // FIFO within the same priority.
    if (!queue->head || priority > queue->head->priority) {
        item->next = queue->head;
        queue->head = item;
        if (!queue->tail) {
            queue->tail = item;
        }
    } else {
        MailRelayQueueItem* prev = queue->head;
        while (prev->next && prev->next->priority >= priority) {
            prev = prev->next;
        }
        item->next = prev->next;
        prev->next = item;
        if (!item->next) {
            queue->tail = item;
        }
    }

    queue->size++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return MAILRELAY_OK;
}

MailRelayStatus mailrelay_queue_enqueue(MailRelayQueue* queue,
                                        const MailRelayMessage* message,
                                        int priority) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return mailrelay_queue_enqueue_scheduled(queue, message, priority, 0, &now);
}

MailRelayStatus mailrelay_queue_dequeue(MailRelayQueue* queue,
                                        int timeout_ms,
                                        MailRelayQueueItem* item) {
    if (!queue || !item) {
        return MAILRELAY_INVALID_ARGS;
    }

    pthread_mutex_lock(&queue->mutex);

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += timeout_ms / 1000;
    deadline.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    while (true) {
        if (queue->shutdown && queue->size == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return MAILRELAY_SHUTDOWN;
        }

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        MailRelayQueueItem* due = find_first_due(queue, &now);
        if (due) {
            remove_item(queue, due);
            *item = *due;
            mailrelay_message_init(&due->message);
            free(due);
            queue->size--;
            pthread_mutex_unlock(&queue->mutex);
            return MAILRELAY_OK;
        }

        if (queue->shutdown) {
            // Queue has only future-due items and shutdown was requested.
            pthread_mutex_unlock(&queue->mutex);
            return MAILRELAY_SHUTDOWN;
        }

        struct timespec wait_deadline = deadline;
        MailRelayQueueItem* earliest = find_earliest(queue);
        if (earliest && timespec_compare(&earliest->next_attempt_at, &deadline) < 0) {
            wait_deadline = earliest->next_attempt_at;
        }

        int rc = pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &wait_deadline);
        if (rc == ETIMEDOUT) {
            clock_gettime(CLOCK_REALTIME, &now);
            due = find_first_due(queue, &now);
            if (due) {
                continue;
            }
            pthread_mutex_unlock(&queue->mutex);
            return MAILRELAY_TIMEOUT;
        }
    }
}

void mailrelay_queue_shutdown(MailRelayQueue* queue) {
    if (!queue) {
        return;
    }
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

// cppcheck-suppress constParameterPointer
int mailrelay_queue_size(MailRelayQueue* queue) {
    if (!queue) {
        return 0;
    }
    pthread_mutex_lock(&queue->mutex);
    int size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}

// cppcheck-suppress constParameterPointer
int mailrelay_queue_capacity(MailRelayQueue* queue) {
    if (!queue) {
        return 0;
    }
    return queue->capacity;
}
