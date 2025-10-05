/*
 * Mock libmicrohttpd (MHD) functions implementation for unit testing
 */

#include "mock_libmicrohttpd.h"
#include <string.h>
#include <stdlib.h>
#include <microhttpd.h>

// Mock state variables
static const char* mock_mhd_lookup_result = NULL;
static const union MHD_ConnectionInfo* mock_mhd_connection_info = NULL;
static bool mock_mhd_create_response_should_fail = false;
static bool mock_mhd_add_header_should_fail = false;
static enum MHD_Result mock_mhd_queue_response_result = MHD_YES;

/*
 * Mock implementation of MHD_lookup_connection_value
 * Use weak linkage to avoid conflicts with other test files
 */
__attribute__((weak))
const char* MHD_lookup_connection_value(struct MHD_Connection *connection,
                                         enum MHD_ValueKind kind,
                                         const char *key) {
    (void)connection; // Suppress unused parameter warning
    (void)kind;       // Suppress unused parameter warning
    (void)key;        // Suppress unused parameter warning

    // Return the mock result
    return mock_mhd_lookup_result;
}

/*
 * Mock implementation of MHD_get_connection_info
 * Matches the actual microhttpd.h signature
 */
__attribute__((weak))
const union MHD_ConnectionInfo* MHD_get_connection_info(struct MHD_Connection *connection,
                                                       enum MHD_ConnectionInfoType info_type,
                                                       ...) {
    (void)connection; // Suppress unused parameter warning
    (void)info_type;  // Suppress unused parameter warning

    // Return the mock connection info
    return mock_mhd_connection_info;
}

/*
 * Mock implementation of MHD_create_response_from_buffer
 */
__attribute__((weak))
struct MHD_Response* MHD_create_response_from_buffer(size_t size, void *buffer,
                                                    enum MHD_ResponseMemoryMode mode) {
    (void)size; (void)buffer; (void)mode;

    if (mock_mhd_create_response_should_fail) {
        return NULL;
    }

    // Return a dummy non-NULL pointer to simulate success
    return (struct MHD_Response*)0xDEADBEEF;
}

/*
 * Mock implementation of MHD_create_response_from_fd
 */
__attribute__((weak))
struct MHD_Response* MHD_create_response_from_fd(size_t size, int fd) {
    (void)size; (void)fd;

    if (mock_mhd_create_response_should_fail) {
        return NULL;
    }

    // Return a dummy non-NULL pointer to simulate success
    return (struct MHD_Response*)0xDEADBEEF;
}

/*
 * Mock implementation of MHD_add_response_header
 */
__attribute__((weak))
enum MHD_Result MHD_add_response_header(struct MHD_Response *response,
                                       const char *header, const char *content) {
    (void)response; (void)header; (void)content;

    return mock_mhd_add_header_should_fail ? MHD_NO : MHD_YES;
}

/*
 * Mock implementation of MHD_queue_response
 */
__attribute__((weak))
enum MHD_Result MHD_queue_response(struct MHD_Connection *connection,
                                  unsigned int status_code,
                                  struct MHD_Response *response) {
    (void)connection; (void)status_code; (void)response;

    return mock_mhd_queue_response_result;
}

/*
 * Mock implementation of MHD_destroy_response
 */
__attribute__((weak))
void MHD_destroy_response(struct MHD_Response *response) {
    (void)response;
    // Mock cleanup - do nothing
}

/*
 * Reset all mock state
 */
void mock_mhd_reset_all(void) {
    if (mock_mhd_lookup_result) {
        free((void*)mock_mhd_lookup_result);
        mock_mhd_lookup_result = NULL;
    }
    mock_mhd_connection_info = NULL;
    mock_mhd_create_response_should_fail = false;
    mock_mhd_add_header_should_fail = false;
    mock_mhd_queue_response_result = MHD_YES;
}

/*
 * Set the result that MHD_lookup_connection_value should return
 */
void mock_mhd_set_lookup_result(const char *result) {
    if (mock_mhd_lookup_result) {
        free((void*)mock_mhd_lookup_result);
    }

    if (result) {
        mock_mhd_lookup_result = strdup(result);
    } else {
        mock_mhd_lookup_result = NULL;
    }
}

/*
 * Get the current mock lookup result
 */
const char* mock_mhd_get_lookup_result(void) {
    return mock_mhd_lookup_result;
}

/*
 * Set the mock connection info result
 */
void mock_mhd_set_connection_info(const union MHD_ConnectionInfo *info) {
    mock_mhd_connection_info = info;
}

// Session management mock state variables
static bool mock_session_has_capacity = true;
static TerminalSession *mock_session_create_result = NULL;
static int mock_session_send_result = 0;
static bool mock_session_resize_result = true;
static size_t mock_session_connections = 0;
static size_t mock_session_max_connections = 10;

/*
 * Mock implementation of session_manager_has_capacity
 */
__attribute__((weak))
bool session_manager_has_capacity(void) {
    return mock_session_has_capacity;
}

/*
 * Mock implementation of create_terminal_session
 */
__attribute__((weak))
TerminalSession* create_terminal_session(const char *shell_command, int rows, int cols) {
    (void)shell_command; // Suppress unused parameter warning
    (void)rows;          // Suppress unused parameter warning
    (void)cols;          // Suppress unused parameter warning

    return mock_session_create_result;
}

/*
 * Mock implementation of remove_terminal_session
 */
__attribute__((weak))
bool remove_terminal_session(TerminalSession *session) {
    (void)session; // Suppress unused parameter warning
    // Mock cleanup - return success
    return true;
}

/*
 * Mock implementation of send_data_to_session
 */
__attribute__((weak))
int send_data_to_session(TerminalSession *session, const char *data, size_t data_size) {
    (void)session;  // Suppress unused parameter warning
    (void)data;     // Suppress unused parameter warning
    (void)data_size; // Suppress unused parameter warning

    return mock_session_send_result;
}

/*
 * Mock implementation of update_session_activity
 */
__attribute__((weak))
void update_session_activity(TerminalSession *session) {
    (void)session; // Suppress unused parameter warning
    // Mock activity update - do nothing
}

/*
 * Mock implementation of resize_terminal_session
 */
__attribute__((weak))
bool resize_terminal_session(TerminalSession *session, int rows, int cols) {
    (void)session; // Suppress unused parameter warning
    (void)rows;    // Suppress unused parameter warning
    (void)cols;    // Suppress unused parameter warning

    return mock_session_resize_result;
}

/*
 * Mock implementation of get_session_manager_stats
 */
#ifdef USE_MOCK_LIBMICROHTTPD
bool get_session_manager_stats(size_t *connections, size_t *max_connections) {
    if (connections) {
        *connections = mock_session_connections;
    }
    if (max_connections) {
        *max_connections = mock_session_max_connections;
    }
    return true;
}
#endif

/*
 * Reset all session mock state
 */
void mock_session_reset_all(void) {
    mock_session_has_capacity = true;
    mock_session_create_result = NULL;
    mock_session_send_result = 0;
    mock_session_resize_result = true;
    mock_session_connections = 0;
    mock_session_max_connections = 10;
}

/*
 * Set the mock result for session_manager_has_capacity
 */
void mock_session_set_has_capacity(bool capacity) {
    mock_session_has_capacity = capacity;
}

/*
 * Set the mock result for create_terminal_session
 */
void mock_session_set_create_result(TerminalSession *session) {
    mock_session_create_result = session;
}

/*
 * Set the mock result for send_data_to_session
 */
void mock_session_set_send_result(int result) {
    mock_session_send_result = result;
}

/*
 * Set the mock result for resize_terminal_session
 */
void mock_session_set_resize_result(bool result) {
    mock_session_resize_result = result;
}

/*
 * Set the mock stats for get_session_manager_stats
 */
void mock_session_set_stats(size_t connections, size_t max_connections) {
    mock_session_connections = connections;
    mock_session_max_connections = max_connections;
}

/*
 * Set whether MHD_create_response_from_buffer should fail
 */
void mock_mhd_set_create_response_should_fail(bool should_fail) {
    mock_mhd_create_response_should_fail = should_fail;
}

/*
 * Set whether MHD_add_response_header should fail
 */
void mock_mhd_set_add_header_should_fail(bool should_fail) {
    mock_mhd_add_header_should_fail = should_fail;
}

/*
 * Set the result that MHD_queue_response should return
 */
void mock_mhd_set_queue_response_result(enum MHD_Result result) {
    mock_mhd_queue_response_result = result;
}
