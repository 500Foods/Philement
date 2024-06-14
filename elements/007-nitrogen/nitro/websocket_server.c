#define _GNU_SOURCE             
#include <unistd.h>
#include "websocket_server.h"
#include "jwt.h"
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#define MAX_PAYLOAD 1024

struct per_session_data {
    bool authenticated;
};

void *websocket_server_thread(void *arg) {
    websocket_thread_arg_t *ws_arg = (websocket_thread_arg_t *)arg;
    run_websocket_server(ws_arg->port, ws_arg->secret_key, ws_arg->running);
    return NULL;
}

static int callback_nitro(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len) {
    struct per_session_data *pss = (struct per_session_data *)user;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            pss->authenticated = false;
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:
            if (!pss->authenticated) {
                // Verify JWT
                if (verify_jwt((char *)in, server_secret_key)) {
                    pss->authenticated = true;
                    char response[] = "Authentication successful";
                    lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
                } else {
                    char response[] = "Authentication failed";
                    lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
                    return -1;  // Close connection
                }
            } else {
                // Handle authenticated messages here
                printf("Received message: %s\n", (char *)in);
                // Echo the message back to the client
                lws_write(wsi, (unsigned char *)in, len, LWS_WRITE_TEXT);
            }
            break;

        case LWS_CALLBACK_PROTOCOL_DESTROY:
            printf("DEBUG: Protocol destroy callback triggered.\n");
            // Perform any necessary cleanup
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "nitro-protocol",
        callback_nitro,
        sizeof(struct per_session_data),
        MAX_PAYLOAD,
        0, // Add the 'id' field and set it to 0 or any other appropriate value
        NULL, // Add the 'user' field initializer
	0   
    },
    { NULL, NULL, 0, 0, 0, NULL, 0}  // terminator
};

void run_websocket_server(int port, const char *secret_key, volatile sig_atomic_t *running) {
    server_secret_key = secret_key;

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);

    if (!context) {
        fprintf(stderr, "lws init failed\n");
        return;
    }

    printf("WebSocket server started on port %d\n", port);

    while (*running) {
        lws_service(context, 250);
        usleep(10000);  // Sleep for 1ms
    }

    printf("DEBUG: websocket server exiting.\n");
    lws_context_destroy(context);
}

