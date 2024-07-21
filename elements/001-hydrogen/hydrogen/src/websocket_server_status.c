// System Libraries
#include <sys/sysinfo.h>
#include <time.h>

// Third-Party Libraries
#include <jansson.h>
#include <libwebsockets.h>

// Project Libraries
#include "websocket_server.h"
#include "logging.h"
#include "configuration.h"

extern AppConfig *app_config;
extern time_t server_start_time;
extern int ws_connections;
extern int ws_connections_total;
extern int ws_requests;

void handle_status_request(struct lws *wsi)
{
    log_this("WebSocket", "Preparing status response", 0, true, true, true);

    json_t *response = json_object();

    // Add basic system information
    json_object_set_new(response, "status", json_string("success"));
    json_object_set_new(response, "serverName", json_string(app_config->server_name));
    json_object_set_new(response, "version", json_string(VERSION));
    json_object_set_new(response, "uptime", json_integer(time(NULL) - server_start_time));
    json_object_set_new(response, "activeConnections", json_integer(ws_connections));
    json_object_set_new(response, "totalConnections", json_integer(ws_connections_total));
    json_object_set_new(response, "totalRequests", json_integer(ws_requests));

    char *response_str = json_dumps(response, JSON_COMPACT);
    size_t len = strlen(response_str);

    log_this("WebSocket", "Status response: %s", 0, true, true, true, response_str);

    unsigned char *buf = malloc(LWS_PRE + len);
    memcpy(buf + LWS_PRE, response_str, len);

    int written = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
    log_this("WebSocket", "Wrote %d bytes to WebSocket", 0, true, true, true, written);

    free(buf);
    free(response_str);
    json_decref(response);
}
