/*
 * Unity Test File: Swagger Error Paths Tests
 * This file contains unit tests for error handling paths in swagger.c
 * that are not covered by other tests, including:
 * - Memory allocation failures
 * - Brotli decompression failures
 * - get_server_url proxy scenarios
 * - create_dynamic_initializer error paths
 */

// Enable mocks BEFORE any other includes to override system functions
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/swagger/swagger.h>

// Include Brotli for decompression testing
#include <brotli/decode.h>

// Forward declarations for functions being tested
bool init_swagger_support_from_payload(SwaggerConfig *config, PayloadFile *payload_files, size_t num_payload_files);
bool decompress_brotli_data(const uint8_t *compressed_data, size_t compressed_size, uint8_t **decompressed_data, size_t *decompressed_size);
char* get_server_url(struct MHD_Connection *connection, const SwaggerConfig *config);
char* create_dynamic_initializer(const char *base_content, const char *server_url, const SwaggerConfig *config);

// Structure to hold in-memory Swagger files (copied from swagger.c)
typedef struct {
    char *name;         // File name (e.g., "index.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} SwaggerFile;

// Mock global state (from swagger.c)
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t server_starting;

// Global swagger state (for resetting between tests)
extern SwaggerFile *swagger_files;
extern size_t num_swagger_files;
extern bool swagger_initialized;
extern const SwaggerConfig *global_swagger_config;

// Function prototypes for test functions
void test_decompress_brotli_null_inputs(void);
void test_get_server_url_no_app_config(void);
void test_get_server_url_x_forwarded_https(void);
void test_get_server_url_x_forwarded_port_non_default(void);
void test_get_server_url_x_forwarded_port_default_http(void);
void test_get_server_url_x_forwarded_port_default_https(void);
void test_get_server_url_host_with_port(void);
void test_get_server_url_no_host_direct_access(void);
void test_create_dynamic_initializer_null_app_config(void);
void test_create_dynamic_initializer_null_api_prefix(void);

// Mock structures for testing
struct MockMHDConnection {
    char *host_header;
    char *x_forwarded_proto;
    char *x_forwarded_port;
};

// Global state for tests
static struct MockMHDConnection mock_connection = {0};

// Mock app_config
static AppConfig mock_app_config = {0};

// Extern declaration for global app_config
extern AppConfig *app_config;

// Mock function prototypes
const char *MHD_lookup_connection_value(struct MHD_Connection *connection,
                                       enum MHD_ValueKind kind, const char *key);

// Mock implementations for testing
const char *MHD_lookup_connection_value(struct MHD_Connection *connection,
                                       enum MHD_ValueKind kind, const char *key) {
    (void)kind;
    struct MockMHDConnection *conn = (struct MockMHDConnection *)connection;
    if (!conn) return NULL;
    
    if (strcmp(key, "Host") == 0) {
        return conn->host_header;
    }
    if (strcmp(key, "X-Forwarded-Proto") == 0) {
        return conn->x_forwarded_proto;
    }
    if (strcmp(key, "X-Forwarded-Port") == 0) {
        return conn->x_forwarded_port;
    }
    return NULL;
}

// Test fixtures
static SwaggerConfig test_config;
static PayloadFile test_payload_files[2];

void setUp(void) {
    // Reset system mocks
    mock_system_reset_all();
    
    // Reset global server state
    server_stopping = false;
    web_server_shutdown = false;
    server_starting = true;
    
    // Clean up any existing swagger files from previous tests
    if (swagger_files) {
        for (size_t i = 0; i < num_swagger_files; i++) {
            free(swagger_files[i].name);
            free(swagger_files[i].data);
        }
        free(swagger_files);
        swagger_files = NULL;
        num_swagger_files = 0;
    }
    swagger_initialized = false;
    global_swagger_config = NULL;
    
    // Initialize mock connection
    memset(&mock_connection, 0, sizeof(mock_connection));
    mock_connection.host_header = (char*)"localhost:8080";
    mock_connection.x_forwarded_proto = NULL;
    mock_connection.x_forwarded_port = NULL;

    // Initialize mock app_config
    memset(&mock_app_config, 0, sizeof(AppConfig));
    mock_app_config.api.prefix = strdup("/api/v1");
    mock_app_config.webserver.port = 8080;
    app_config = &mock_app_config;

    // Initialize test config
    memset(&test_config, 0, sizeof(SwaggerConfig));
    test_config.enabled = true;
    test_config.payload_available = true;
    test_config.prefix = strdup("/swagger");

    // Set up minimal payload files for testing
    memset(test_payload_files, 0, sizeof(test_payload_files));
    test_payload_files[0].name = (char*)"swagger/test.html";
    test_payload_files[0].data = (uint8_t*)"test data";
    test_payload_files[0].size = 9;
    test_payload_files[0].is_compressed = false;
    
    test_payload_files[1].name = (char*)"swagger/test.css";
    test_payload_files[1].data = (uint8_t*)"test css";
    test_payload_files[1].size = 8;
    test_payload_files[1].is_compressed = false;
}

void tearDown(void) {
    // Clean up test config
    free(test_config.prefix);
    memset(&test_config, 0, sizeof(SwaggerConfig));

    // Clean up mock app_config
    free(mock_app_config.api.prefix);
    memset(&mock_app_config, 0, sizeof(AppConfig));
    
    // Reset system mocks
    mock_system_reset_all();
    
    // Reset global swagger state
    swagger_initialized = false;
    global_swagger_config = NULL;
}

//=============================================================================
// decompress_brotli_data Error Path Tests
//=============================================================================

void test_decompress_brotli_null_inputs(void) {
    uint8_t *output = NULL;
    size_t output_size = 0;
    const uint8_t test_data[] = "test";
    
    // Test with NULL compressed_data
    bool result1 = decompress_brotli_data(NULL, 4, &output, &output_size);
    TEST_ASSERT_FALSE(result1);
    
    // Test with NULL decompressed_data pointer
    bool result2 = decompress_brotli_data(test_data, 4, NULL, &output_size);
    TEST_ASSERT_FALSE(result2);
    
    // Test with NULL decompressed_size pointer
    bool result3 = decompress_brotli_data(test_data, 4, &output, NULL);
    TEST_ASSERT_FALSE(result3);
}

//=============================================================================
// get_server_url Error Path and Proxy Tests
//=============================================================================

void test_get_server_url_no_app_config(void) {
    // Set app_config to NULL
    app_config = NULL;
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    // Should return NULL when app_config is NULL
    TEST_ASSERT_NULL(result);
    
    // Restore app_config
    app_config = &mock_app_config;
}

void test_get_server_url_x_forwarded_https(void) {
    // Set up X-Forwarded-Proto header
    mock_connection.x_forwarded_proto = (char*)"https";
    mock_connection.host_header = (char*)"api.example.com";
    mock_connection.x_forwarded_port = NULL;
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "https://") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "api.example.com") != NULL);
    
    free(result);
}

void test_get_server_url_x_forwarded_port_non_default(void) {
    // Set up X-Forwarded-Proto and X-Forwarded-Port headers with non-default port
    mock_connection.x_forwarded_proto = (char*)"https";
    mock_connection.host_header = (char*)"api.example.com";
    mock_connection.x_forwarded_port = (char*)"8443";
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "https://") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "api.example.com:8443") != NULL);
    
    free(result);
}

void test_get_server_url_x_forwarded_port_default_http(void) {
    // Set up X-Forwarded-Proto as http with default port 80
    mock_connection.x_forwarded_proto = (char*)"http";
    mock_connection.host_header = (char*)"api.example.com";
    mock_connection.x_forwarded_port = (char*)"80";
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "http://") != NULL);
    // Port 80 should be omitted for http
    TEST_ASSERT_TRUE(strstr(result, ":80") == NULL);
    
    free(result);
}

void test_get_server_url_x_forwarded_port_default_https(void) {
    // Set up X-Forwarded-Proto as https with default port 443
    mock_connection.x_forwarded_proto = (char*)"https";
    mock_connection.host_header = (char*)"api.example.com";
    mock_connection.x_forwarded_port = (char*)"443";
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "https://") != NULL);
    // Port 443 should be omitted for https
    TEST_ASSERT_TRUE(strstr(result, ":443") == NULL);
    
    free(result);
}

void test_get_server_url_host_with_port(void) {
    // Host header already includes port
    mock_connection.host_header = (char*)"localhost:9000";
    mock_connection.x_forwarded_proto = NULL;
    mock_connection.x_forwarded_port = NULL;
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "localhost:9000") != NULL);
    
    free(result);
}

void test_get_server_url_no_host_direct_access(void) {
    // No Host header - should use localhost with port from config
    mock_connection.host_header = NULL;
    mock_connection.x_forwarded_proto = NULL;
    mock_connection.x_forwarded_port = NULL;
    
    char *result = get_server_url((struct MHD_Connection*)&mock_connection, &test_config);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "localhost:8080") != NULL);
    
    free(result);
}

//=============================================================================
// create_dynamic_initializer Error Path Tests
//=============================================================================

void test_create_dynamic_initializer_null_app_config(void) {
    // Set app_config to NULL
    app_config = NULL;
    
    char *result = create_dynamic_initializer("{}", "http://localhost:8080", &test_config);
    
    // Should return NULL when app_config is NULL
    TEST_ASSERT_NULL(result);
    
    // Restore app_config
    app_config = &mock_app_config;
}

void test_create_dynamic_initializer_null_api_prefix(void) {
    // Set api.prefix to NULL
    free(mock_app_config.api.prefix);
    mock_app_config.api.prefix = NULL;
    
    char *result = create_dynamic_initializer("{}", "http://localhost:8080", &test_config);
    
    // Should return NULL when api.prefix is NULL
    TEST_ASSERT_NULL(result);
    
    // Restore api.prefix
    mock_app_config.api.prefix = strdup("/api/v1");
}

//=============================================================================
// Main
//=============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // decompress_brotli_data error tests
    RUN_TEST(test_decompress_brotli_null_inputs);
    
    // get_server_url error and proxy tests
    RUN_TEST(test_get_server_url_no_app_config);
    RUN_TEST(test_get_server_url_x_forwarded_https);
    RUN_TEST(test_get_server_url_x_forwarded_port_non_default);
    RUN_TEST(test_get_server_url_x_forwarded_port_default_http);
    RUN_TEST(test_get_server_url_x_forwarded_port_default_https);
    RUN_TEST(test_get_server_url_host_with_port);
    RUN_TEST(test_get_server_url_no_host_direct_access);
    
    // create_dynamic_initializer error tests
    RUN_TEST(test_create_dynamic_initializer_null_app_config);
    RUN_TEST(test_create_dynamic_initializer_null_api_prefix);

    return UNITY_END();
}
