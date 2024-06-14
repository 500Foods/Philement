#ifndef NITRO_WEBSOCKET_SERVER_H
#define NITRO_WEBSOCKET_SERVER_H

#include <stddef.h>
#include <signal.h>

typedef struct {
    int port;
    const char *secret_key;
    volatile sig_atomic_t *running;
} websocket_thread_arg_t;

static const char *server_secret_key __attribute__((unused));

void run_websocket_server(int port, const char *secret_key, volatile sig_atomic_t *running);
void *websocket_server_thread(void *arg); 

#endif // NITRO_WEBSOCKET_SERVER_H
