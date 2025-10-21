/*
 * System Test API Endpoint Implementation
 * 
 * Implements the /api/system/test endpoint that provides diagnostic information
 * for development and debugging, including caller information, headers, and parameters.
 */

// Standard includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Project includes 
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/webserver/web_server_core.h>

// Local includes
#include "test.h"

/*
 * Enhanced test endpoint with complete diagnostics
 * 
 * This test endpoint is designed to help diagnose API issues by returning:
 * - Client information (IP, headers)
 * - JWT authentication details
 * - Query and POST parameters
 * - Server information and environment
 * - Request timing and performance metrics 
 * - All request headers
 */
enum MHD_Result handle_system_test_request(struct MHD_Connection *connection,
                                         const char *method,
                                         const char *upload_data,
                                         size_t *upload_data_size,
                                         void **con_cls) {
    (void)upload_data; // Mark parameter as intentionally unused
    log_this(SR_API, "Handling test endpoint request", LOG_LEVEL_STATE, 0);
    
    // Start timing for performance metrics
#ifdef UNITY_TEST_MODE
    struct timespec start_time = {1000000, 500000000}; // Mock start time
#else
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
    
    // Initialize connection context if needed
    if (*con_cls == NULL) {
        static int connection_initialized = 1;
        *con_cls = &connection_initialized;  // Mark as initialized
        return MHD_YES;  // Continue processing
    }
    
    // Process POST data if present
    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            // More data coming, continue processing
            *upload_data_size = 0;
            return MHD_YES;
        }
    }
    
    // Use default JWT secret since configuration access is not available
    const char* jwt_secret = "hydrogen_api_secret_change_me";
    
    // Create the response JSON object
    json_t *response_obj = json_object();
    
    // 1. Add client IP address
    char *client_ip = api_get_client_ip(connection);
    if (client_ip) {
        json_object_set_new(response_obj, "client_ip", json_string(client_ip));
        free(client_ip);
    } else {
        json_object_set_new(response_obj, "client_ip", json_string("unknown"));
    }
    
    // 2. Add X-Forwarded-For header if present
    const char *forwarded_for = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Forwarded-For");
    if (forwarded_for) {
        json_object_set_new(response_obj, "x_forwarded_for", json_string(forwarded_for));
    } else {
        json_object_set_new(response_obj, "x_forwarded_for", json_null());
    }
    
    // 3. Add JWT claims if present
    json_t *jwt_claims = api_extract_jwt_claims(connection, jwt_secret);
    if (jwt_claims) {
        json_object_set_new(response_obj, "jwt_claims", jwt_claims);
    } else {
        json_object_set_new(response_obj, "jwt_claims", json_null());
    }

    // 4. Add query parameters
    json_t *query_params = api_extract_query_params(connection);
    json_object_set_new(response_obj, "query_parameters", query_params);
    
    // 5. Add POST data if applicable
    if (strcmp(method, "POST") == 0) {
        const char *content_type = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Type");
        
        if (content_type && strstr(content_type, "application/x-www-form-urlencoded")) {
            // Process form data
            json_t *post_data = json_object();
            
            // Manually extract form data fields from the connection
            const char *field1_value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "field1");
            const char *field2_value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "field2");
            
            if (field1_value) {
                json_object_set_new(post_data, "field1", json_string(field1_value));
            }
            
            if (field2_value) {
                json_object_set_new(post_data, "field2", json_string(field2_value));
            }
            
            // Use the post data we've extracted directly
            json_object_set_new(response_obj, "post_data", post_data);
        } else if (content_type && strstr(content_type, "application/json")) {
            // For JSON data, we would need to have captured the raw POST data
            // which requires more complex processing using a post processor
            json_object_set_new(response_obj, "post_data", json_string("JSON data not captured in this example"));
        } else {
            json_object_set_new(response_obj, "post_data", json_string("Unsupported content type"));
        }
    } else {
        json_object_set_new(response_obj, "post_data", json_null());
    }
    
    // 6. Add server information
    json_t *server_info = json_object();
    
    // System information
    struct utsname system_info;
    if (uname(&system_info) == 0) {
        json_object_set_new(server_info, "system", json_string(system_info.sysname));
        json_object_set_new(server_info, "hostname", json_string(system_info.nodename));
        json_object_set_new(server_info, "release", json_string(system_info.release));
        json_object_set_new(server_info, "version", json_string(system_info.version));
        json_object_set_new(server_info, "machine", json_string(system_info.machine));
    }
    
    // Process information
    char pid_str[16];
    snprintf(pid_str, sizeof(pid_str), "%d", getpid());
    json_object_set_new(server_info, "pid", json_string(pid_str));

    // Add some environment variables that might be useful for debugging
    const char *env_vars[] = {"PATH", "LD_LIBRARY_PATH", "HOME", "USER", "LANG", "TZ"};
    json_t *env_obj = json_object();
    for (size_t i = 0; i < sizeof(env_vars) / sizeof(env_vars[0]); i++) {
        const char *env_value = getenv(env_vars[i]);
        if (env_value) {
            json_object_set_new(env_obj, env_vars[i], json_string(env_value));
        }
    }
    json_object_set_new(server_info, "environment", env_obj);
    
    json_object_set_new(response_obj, "server_info", server_info);
    
    // 7. Add request headers selectively (safer than trying to get all)
    json_t *headers = json_object();
    const char *important_headers[] = {
        "User-Agent", "Accept", "Content-Type", "Host",
        "Connection", "Cache-Control", "Referer",
        "Authorization", "X-Requested-With"
    };
    
    for (size_t i = 0; i < sizeof(important_headers) / sizeof(important_headers[0]); i++) {
        const char *value = MHD_lookup_connection_value(
            connection, MHD_HEADER_KIND, important_headers[i]);
        if (value) {
            json_object_set_new(headers, important_headers[i], json_string(value));
        }
    }
    json_object_set_new(response_obj, "request_headers", headers);
    
    // 8. Add request information
    json_t *request_info = json_object();
    json_object_set_new(request_info, "method", json_string(method));
    
    // Get URL directly from host header and request path
    const char *host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
    // Use known endpoint path since we can't get it directly from MHD in this context
    const char *endpoint_path = "/api/system/test";
    
    // Reconstruct the URL with any query parameters
    if (host) {
        char uri_buffer[256] = {0};
        const char *has_params = json_object_size(query_params) > 0 ? "?" : "";
        snprintf(uri_buffer, sizeof(uri_buffer), "http://%s%s%s", 
                host, endpoint_path, has_params);
        json_object_set_new(request_info, "uri", json_string(uri_buffer));
    } else {
        json_object_set_new(request_info, "uri", json_string(endpoint_path));
    }
    
    json_object_set_new(request_info, "http_version", json_string(
        strcmp(method, "HTTP/1.0") == 0 ? "HTTP/1.0" : "HTTP/1.1"));
    json_object_set_new(response_obj, "request_info", request_info);
    
    // 9. Add timing/performance information
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double processing_time = ((double)(end_time.tv_sec - start_time.tv_sec)) * 1000.0 +
                            ((double)(end_time.tv_nsec - start_time.tv_nsec)) / 1000000.0;

    json_t *timing = json_object();
    json_object_set_new(timing, "processing_time_ms", json_real(processing_time));
    json_object_set_new(timing, "timestamp", json_integer(time(NULL)));
    json_object_set_new(response_obj, "timing", timing);

    // Reset connection context before returning to prevent cleanup issues
    *con_cls = NULL;
    
    // Send the JSON response using the common utility
    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}
