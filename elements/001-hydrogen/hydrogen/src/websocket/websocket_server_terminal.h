#ifndef WEBSOCKET_SERVER_TERMINAL_H
#define WEBSOCKET_SERVER_TERMINAL_H

// Terminal WebSocket includes
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal.h>

// Libwebsockets header for struct lws
#include <libwebsockets.h>

// Forward declarations for terminal functions
int validate_terminal_protocol(struct lws *wsi);
json_t* parse_terminal_json_message(void);
int validate_terminal_message_type(json_t *json_msg);
TerminalWSConnection* create_terminal_adapter(struct lws *wsi, TerminalSession *session);
int process_terminal_message(TerminalWSConnection *ws_conn_adapter);
int handle_terminal_message(struct lws *wsi);
TerminalSession* find_or_create_terminal_session(struct lws *wsi);
void stop_pty_bridge_thread(TerminalSession *session);

#endif // WEBSOCKET_SERVER_TERMINAL_H