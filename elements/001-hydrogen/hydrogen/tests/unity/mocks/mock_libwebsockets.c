/*
 * Mock libwebsockets implementation for unit testing
 *
 * This file provides simple stub implementations of libwebsockets functions
 * for unit testing websocket_server.c
 */

#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>

// Include our mock header - this must come after libwebsockets.h for types
#include "mock_libwebsockets.h"

// Mock state variables
static int mock_lws_hdr_copy_result = 0;
static int mock_lws_hdr_total_length_result = 0;
static void* mock_lws_wsi_user_result = NULL;
static struct lws_context* mock_lws_get_context_result = NULL;
static void* mock_lws_context_user_result = NULL;
static int mock_lws_service_result = 0;
static struct lws_context* mock_lws_create_context_result = NULL;
static int mock_lws_is_final_fragment_result = 1; // Default to final fragment
static int mock_lws_write_result = 0;
static const char* mock_lws_protocol_name = "hydrogen"; // Default protocol name

// Mock header data for lws_hdr_copy
static char mock_header_data[256] = "";
static int mock_header_data_len = 0;

// Mock function implementations - matching real libwebsockets signatures
int mock_lws_hdr_copy(struct lws *wsi, char *dest, int len, enum lws_token_indexes token)
{
    (void)wsi; (void)token;

    if (mock_lws_hdr_copy_result > 0 && dest && len > 0 && mock_header_data_len > 0) {
        // Copy the mock header data
        size_t copy_len = (mock_header_data_len < len) ? (size_t)mock_header_data_len : (size_t)len - 1;
        strncpy(dest, mock_header_data, copy_len);
        dest[copy_len] = '\0';
        return (int)copy_len;
    }

    return mock_lws_hdr_copy_result;
}

int mock_lws_hdr_total_length(struct lws *wsi, enum lws_token_indexes token)
{
    (void)wsi; (void)token;
    return mock_lws_hdr_total_length_result;
}

void *mock_lws_wsi_user(struct lws *wsi)
{
    (void)wsi;
    return mock_lws_wsi_user_result;
}

struct lws_context *mock_lws_get_context(const struct lws *wsi)
{
    (void)wsi;
    return mock_lws_get_context_result;
}

void *mock_lws_context_user(struct lws_context *context)
{
    (void)context;
    return mock_lws_context_user_result;
}

int mock_lws_service(struct lws_context *context, int timeout_ms)
{
    (void)context; (void)timeout_ms;
    return mock_lws_service_result;
}

void mock_lws_cancel_service(struct lws_context *context)
{
    (void)context;
    // No-op for mock
}

void mock_lws_set_log_level(int level, void (*log_emit_function)(int level, const char *line))
{
    (void)level; (void)log_emit_function;
    // No-op for mock
}

struct lws_context *mock_lws_create_context(const struct lws_context_creation_info *info)
{
    (void)info;
    return mock_lws_create_context_result;
}

void mock_lws_context_destroy(struct lws_context *context)
{
    (void)context;
    // No-op for mock
}

int mock_lws_get_peer_simple(struct lws *wsi, char *name, int namelen)
{
    (void)wsi;

    if (name && namelen > 0) {
        // Return a mock IP address
        const char *mock_ip = "127.0.0.1";
        size_t copy_len = strlen(mock_ip);
        if ((size_t)namelen > copy_len) {
            strcpy(name, mock_ip);
            return (int)copy_len;
        } else {
            // Buffer too small
            return -1;
        }
    }

    return -1;
}

int mock_lws_is_final_fragment(struct lws *wsi)
{
    (void)wsi;
    return mock_lws_is_final_fragment_result;
}

int mock_lws_write(struct lws *wsi, unsigned char *buf, size_t len, enum lws_write_protocol protocol)
{
    (void)wsi; (void)buf; (void)len; (void)protocol;
    return mock_lws_write_result;
}

const struct lws_protocols *mock_lws_get_protocol(struct lws *wsi)
{
    (void)wsi;

    // Return a mock protocol structure with the configured name
    static struct lws_protocols mock_protocol;
    mock_protocol.name = mock_lws_protocol_name;
    return &mock_protocol;
}

// Mock control functions for tests
void mock_lws_set_hdr_copy_result(int result)
{
    mock_lws_hdr_copy_result = result;
}

void mock_lws_set_hdr_data(const char* data)
{
    if (data) {
        size_t data_len = strlen(data);
        if (data_len < sizeof(mock_header_data)) {
            mock_header_data_len = (int)data_len;
            strcpy(mock_header_data, data);
        } else {
            mock_header_data_len = (int)sizeof(mock_header_data) - 1;
            strncpy(mock_header_data, data, (size_t)mock_header_data_len);
            mock_header_data[mock_header_data_len] = '\0';
        }
    } else {
        mock_header_data[0] = '\0';
        mock_header_data_len = 0;
    }
}

void mock_lws_set_hdr_total_length_result(int result)
{
    mock_lws_hdr_total_length_result = result;
}

void mock_lws_set_wsi_user_result(void* result)
{
    mock_lws_wsi_user_result = result;
}

void mock_lws_set_get_context_result(struct lws_context* result)
{
    mock_lws_get_context_result = result;
}

void mock_lws_set_context_user_result(void* result)
{
    mock_lws_context_user_result = result;
}

void mock_lws_set_service_result(int result)
{
    mock_lws_service_result = result;
}

void mock_lws_set_create_context_result(struct lws_context* result)
{
    mock_lws_create_context_result = result;
}

void mock_lws_set_is_final_fragment_result(int result)
{
    mock_lws_is_final_fragment_result = result;
}

void mock_lws_set_write_result(int result)
{
    mock_lws_write_result = result;
}

void mock_lws_set_protocol_name(const char* name)
{
    mock_lws_protocol_name = name ? name : "hydrogen";
}

int mock_lws_get_is_final_fragment_result(void)
{
    return mock_lws_is_final_fragment_result;
}

void mock_lws_reset_all(void)
{
    mock_lws_hdr_copy_result = 0;
    mock_lws_hdr_total_length_result = 0;
    mock_lws_wsi_user_result = NULL;
    mock_lws_get_context_result = NULL;
    mock_lws_context_user_result = NULL;
    mock_lws_service_result = 0;
    mock_lws_create_context_result = NULL;
    mock_lws_is_final_fragment_result = 1; // Default to final fragment
    mock_lws_write_result = 0;
    mock_lws_protocol_name = "hydrogen"; // Reset to default
    mock_header_data[0] = '\0';
    mock_header_data_len = 0;
}