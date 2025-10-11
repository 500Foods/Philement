#ifndef WEBSOCKET_SERVER_MESSAGE_H
#define WEBSOCKET_SERVER_MESSAGE_H

// Libwebsockets header for struct lws
#include <libwebsockets.h>

// Jansson JSON library
#include <jansson.h>

// Forward declarations for WebSocket message functions
int validate_session_and_context(const WebSocketSessionData *session);
int buffer_message_data(struct lws *wsi, const void *in, size_t len);
int parse_and_handle_message(struct lws *wsi);
int ws_handle_receive(struct lws *wsi, const WebSocketSessionData *session, const void *in, size_t len);
int handle_message_type(struct lws *wsi, const char *type);
int ws_write_json_response(struct lws *wsi, json_t *json);
int ws_write_raw_data(struct lws *wsi, const char *data, size_t len);
json_t* create_json_response(const char *type, const char *data);
json_t* create_pty_output_json(const char *buffer, size_t data_size);

#endif // WEBSOCKET_SERVER_MESSAGE_H