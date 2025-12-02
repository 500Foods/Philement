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
static int mock_lws_hdr_copy_should_fail = 0;
static int mock_lws_hdr_total_length_should_fail = 0;
static int mock_lws_get_peer_should_fail = 0;
static char mock_peer_address[256] = "127.0.0.1";

// Mock header data for lws_hdr_copy - support different tokens
static char mock_auth_header_data[256] = "";
static int mock_auth_header_len = 0;
static char mock_uri_data[512] = "";
static int mock_uri_len = 0;

// Mock function implementations - matching real libwebsockets signatures
int mock_lws_hdr_copy(struct lws *wsi, char *dest, int len, enum lws_token_indexes token)
{
    (void)wsi;

    // Check for failure condition
    if (mock_lws_hdr_copy_should_fail) {
        return -1;
    }

    // Return different data based on token type
    if (token == WSI_TOKEN_HTTP_AUTHORIZATION) {
        if (dest && len > 0 && mock_auth_header_len > 0) {
            size_t copy_len = (mock_auth_header_len < len) ? (size_t)mock_auth_header_len : (size_t)len - 1;
            strncpy(dest, mock_auth_header_data, copy_len);
            dest[copy_len] = '\0';
            return (int)copy_len;
        }
        return 0;
    } else if (token == WSI_TOKEN_GET_URI) {
        if (dest && len > 0 && mock_uri_len > 0) {
            size_t copy_len = (mock_uri_len < len) ? (size_t)mock_uri_len : (size_t)len - 1;
            strncpy(dest, mock_uri_data, copy_len);
            dest[copy_len] = '\0';
            return (int)copy_len;
        }
        return 0;
    }

    return 0;
}

int mock_lws_hdr_total_length(struct lws *wsi, enum lws_token_indexes token)
{
    (void)wsi;

    // Check for failure condition
    if (mock_lws_hdr_total_length_should_fail) {
        return -1;
    }

    // Return different lengths based on token type
    if (token == WSI_TOKEN_HTTP_AUTHORIZATION) {
        return mock_auth_header_len;
    } else if (token == WSI_TOKEN_GET_URI) {
        return mock_uri_len;
    }

    return 0;
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

    // Check for failure condition
    if (mock_lws_get_peer_should_fail) {
        return -1;
    }

    if (name && namelen > 0) {
        // Return the configured peer address
        size_t copy_len = strlen(mock_peer_address);
        if ((size_t)namelen > copy_len) {
            strcpy(name, mock_peer_address);
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

int mock_lws_callback_on_writable(struct lws *wsi)
{
    (void)wsi;
    return 0; // Success
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
    // Set Authorization header data
    if (data) {
        size_t data_len = strlen(data);
        if (data_len < sizeof(mock_auth_header_data)) {
            mock_auth_header_len = (int)data_len;
            strcpy(mock_auth_header_data, data);
        } else {
            mock_auth_header_len = (int)sizeof(mock_auth_header_data) - 1;
            strncpy(mock_auth_header_data, data, (size_t)mock_auth_header_len);
            mock_auth_header_data[mock_auth_header_len] = '\0';
        }
    } else {
        mock_auth_header_data[0] = '\0';
        mock_auth_header_len = 0;
    }
}

void mock_lws_set_uri_data(const char* uri)
{
    // Set GET URI data
    if (uri) {
        size_t uri_len = strlen(uri);
        if (uri_len < sizeof(mock_uri_data)) {
            mock_uri_len = (int)uri_len;
            strcpy(mock_uri_data, uri);
        } else {
            mock_uri_len = (int)sizeof(mock_uri_data) - 1;
            strncpy(mock_uri_data, uri, (size_t)mock_uri_len);
            mock_uri_data[mock_uri_len] = '\0';
        }
    } else {
        mock_uri_data[0] = '\0';
        mock_uri_len = 0;
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

void mock_lws_set_peer_address(const char* address)
{
    if (address) {
        size_t addr_len = strlen(address);
        if (addr_len < sizeof(mock_peer_address)) {
            strcpy(mock_peer_address, address);
        }
    }
}

void mock_lws_set_hdr_copy_failure(int should_fail)
{
    mock_lws_hdr_copy_should_fail = should_fail;
}

void mock_lws_set_hdr_total_length_failure(int should_fail)
{
    mock_lws_hdr_total_length_should_fail = should_fail;
}

void mock_lws_set_get_peer_failure(int should_fail)
{
    mock_lws_get_peer_should_fail = should_fail;
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
    mock_lws_hdr_copy_should_fail = 0;
    mock_lws_hdr_total_length_should_fail = 0;
    mock_lws_get_peer_should_fail = 0;
    strcpy(mock_peer_address, "127.0.0.1"); // Reset to default
    mock_auth_header_data[0] = '\0';
    mock_auth_header_len = 0;
    mock_uri_data[0] = '\0';
    mock_uri_len = 0;
}