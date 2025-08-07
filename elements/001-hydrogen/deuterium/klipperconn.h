#ifndef KLIPPERCONN_H
#define KLIPPERCONN_H

#include <pthread.h>
#include <jansson.h>
#include "queue.h"

typedef struct {
    int socket_fd;
    queue_t* send_queue;
    queue_t* receive_queue;
    pthread_t sender_thread;
    pthread_t receiver_thread;
    int should_stop;
    char* socket_path;
} klipper_connection;

klipper_connection* klipper_init(void);
int klipper_start_threads(klipper_connection* conn);
void klipper_send_command(klipper_connection* conn, json_t* command);
json_t* klipper_get_message(klipper_connection* conn);
void klipper_cleanup(klipper_connection* conn);

#endif
