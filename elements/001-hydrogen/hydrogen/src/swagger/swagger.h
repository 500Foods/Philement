/*
 * Swagger Support
 * 
 * Provides functionality for serving Swagger UI documentation:
 * - Payload detection and extraction from executable
 * - In-memory file serving with Brotli support
 * - Request routing and handling
 */

#ifndef SWAGGER_H
#define SWAGGER_H

// System headers
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project headers
#include <src/config/config.h>
#include <src/config/config_webserver.h>  // For WebServerConfig
#include <src/config/config_swagger.h>    // For SwaggerConfig

/**
 * Initialize Swagger support
 *
 * Checks for Swagger payload in executable and extracts it if found.
 * Must be called during server initialization.
 *
 * @param config The Swagger configuration
 * @return true if initialization successful, false otherwise
 */
bool init_swagger_support(SwaggerConfig *config);

/**
 * Initialize Swagger support from payload cache files
 *
 * Takes pre-loaded files from payload cache and sets up memory structures.
 * This is used when files have already been loaded by the payload system.
 *
 * @param config The Swagger configuration
 * @param payload_files Array of payload files
 * @param num_payload_files Number of files in the array
 * @return true if initialization successful, false otherwise
 */
bool init_swagger_support_from_payload(SwaggerConfig *config, PayloadFile *payload_files, size_t num_payload_files);

/**
 * Validate if a URL is a Swagger request (wrapper for webserver integration)
 *
 * @param url The request URL to validate
 * @return true if URL is valid for Swagger, false otherwise
 */
bool swagger_url_validator(const char *url);

/**
 * Handle a Swagger request (wrapper for webserver integration)
 *
 * @param cls The SwaggerConfig pointer
 * @param connection The MHD connection
 * @param url The request URL
 * @param method The HTTP method
 * @param version The HTTP version
 * @param upload_data Upload data buffer
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_YES if handled successfully, MHD_NO otherwise
 */
enum MHD_Result swagger_request_handler(void *cls,
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method,
                                       const char *version,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls);


/**
 * Check if a request is for Swagger UI content
 * 
 * @param url The request URL
 * @param config The Swagger configuration
 * @return true if URL matches Swagger prefix, false otherwise
 */
bool is_swagger_request(const char *url, const SwaggerConfig *config);

/**
 * Handle a Swagger UI request
 * 
 * Serves files from the in-memory Swagger UI payload.
 * Supports Brotli compression if available.
 * 
 * @param connection The MHD connection
 * @param url The request URL
 * @param config The Swagger configuration
 * @return MHD_YES if handled successfully, MHD_NO otherwise
 */
enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url,
                                     const SwaggerConfig *config);

/**
 * Free Swagger files memory
 *
 * Frees the in-memory Swagger file structures.
 * Called internally by cleanup_swagger_support.
 */
void free_swagger_files(void);

/**
 * Clean up Swagger support
 *
 * Frees any resources allocated for Swagger support.
 * Must be called during server shutdown.
 */
void cleanup_swagger_support(void);


#endif // SWAGGER_H
