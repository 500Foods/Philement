/*
 * Mock libwebsockets functions for unit testing
 *
 * This file provides mock implementations of specific libwebsockets functions
 * used in websocket_server.c to enable unit testing without external dependencies.
 */

#ifndef MOCK_LIBWEBSOCKETS_H
#define MOCK_LIBWEBSOCKETS_H

#include <stddef.h>

// Forward declarations - these will be resolved by the real libwebsockets.h
struct lws_context;
struct lws;

// Mock function declarations - these will override the real ones when USE_MOCK_LIBWEBSOCKETS is defined
#ifdef USE_MOCK_LIBWEBSOCKETS

// Override specific functions with our mocks
#define lws_hdr_copy mock_lws_hdr_copy
#define lws_hdr_total_length mock_lws_hdr_total_length
#define lws_wsi_user mock_lws_wsi_user
#define lws_get_context mock_lws_get_context
#define lws_context_user mock_lws_context_user
#define lws_service mock_lws_service
#define lws_cancel_service mock_lws_cancel_service
#define lws_set_log_level mock_lws_set_log_level
#define lws_create_context mock_lws_create_context
#define lws_context_destroy mock_lws_context_destroy
#define lws_get_peer_simple mock_lws_get_peer_simple
#define lws_is_final_fragment mock_lws_is_final_fragment
#define lws_write mock_lws_write
#define lws_get_protocol mock_lws_get_protocol
#define lws_callback_on_writable mock_lws_callback_on_writable

// Always declare mock function prototypes for the .c file
int mock_lws_hdr_copy(struct lws *wsi, char *dest, int len, enum lws_token_indexes token);
int mock_lws_hdr_total_length(struct lws *wsi, enum lws_token_indexes token);
void *mock_lws_wsi_user(struct lws *wsi);
struct lws_context *mock_lws_get_context(const struct lws *wsi);
void *mock_lws_context_user(struct lws_context *context);
int mock_lws_service(struct lws_context *context, int timeout_ms);
void mock_lws_cancel_service(struct lws_context *context);
void mock_lws_set_log_level(int level, void (*log_emit_function)(int level, const char *line));
struct lws_context *mock_lws_create_context(const struct lws_context_creation_info *info);
void mock_lws_context_destroy(struct lws_context *context);
int mock_lws_get_peer_simple(struct lws *wsi, char *name, int namelen);
int mock_lws_is_final_fragment(struct lws *wsi);
int mock_lws_write(struct lws *wsi, unsigned char *buf, size_t len, enum lws_write_protocol protocol);
const struct lws_protocols *mock_lws_get_protocol(struct lws *wsi);
int mock_lws_callback_on_writable(struct lws *wsi);

// Mock control functions for tests
void mock_lws_set_hdr_copy_result(int result);
void mock_lws_set_hdr_total_length_result(int result);
void mock_lws_set_hdr_data(const char* data);
void mock_lws_set_uri_data(const char* uri);
void mock_lws_set_wsi_user_result(void* result);
void mock_lws_set_get_context_result(struct lws_context* result);
void mock_lws_set_context_user_result(void* result);
void mock_lws_set_service_result(int result);
void mock_lws_set_create_context_result(struct lws_context* result);
void mock_lws_set_is_final_fragment_result(int result);
void mock_lws_set_write_result(int result);
void mock_lws_set_protocol_name(const char* name);
void mock_lws_set_peer_address(const char* address);
void mock_lws_set_hdr_copy_failure(int should_fail);
void mock_lws_set_hdr_total_length_failure(int should_fail);
void mock_lws_set_get_peer_failure(int should_fail);
int mock_lws_get_is_final_fragment_result(void);
void mock_lws_reset_all(void);

#endif // USE_MOCK_LIBWEBSOCKETS

#endif // MOCK_LIBWEBSOCKETS_H