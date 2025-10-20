/*
 * Conduit Query Endpoint Header
 *
 * REST API endpoint for executing database queries by reference.
 */

//@ swagger:path /api/conduit/query
//@ swagger:method POST
//@ swagger:summary Execute database query by reference
//@ swagger:description Executes a pre-defined query from the Query Table Cache
//@ swagger:operationId executeQuery
//@ swagger:request body application/json QueryRequest
//@ swagger:response 200 application/json QueryResponse
//@ swagger:response 400 application/json ErrorResponse
//@ swagger:response 404 application/json ErrorResponse
//@ swagger:response 408 application/json ErrorResponse
//@ swagger:response 500 application/json ErrorResponse

#ifndef CONDUIT_QUERY_H
#define CONDUIT_QUERY_H

// Project includes
#include <src/hydrogen.h>

// MHD includes for HTTP handling
#include <microhttpd.h>

// Query request structure
typedef struct {
    int query_ref;
    char* database;
    json_t* params;  // JSON object with INTEGER, STRING, BOOLEAN, FLOAT sections
} ConduitQueryRequest;

// Query response structure
typedef struct {
    bool success;
    int query_ref;
    char* description;
    json_t* rows;
    size_t row_count;
    size_t column_count;
    long long execution_time_ms;
    char* queue_used;
    char* error_message;
} ConduitQueryResponse;

// Function prototypes

// Main query handler
int conduit_query_handler(
    struct MHD_Connection* connection,
    const char* url,
    const char* method,
    const char* upload_data,
    size_t* upload_data_size,
    void** con_cls
);

// Request parsing
ConduitQueryRequest* conduit_query_parse_request(const char* json_str);
void conduit_query_free_request(ConduitQueryRequest* request);

// Response handling
ConduitQueryResponse* conduit_query_execute(const ConduitQueryRequest* request);
void conduit_query_free_response(ConduitQueryResponse* response);

// JSON response sending
int conduit_query_send_response(struct MHD_Connection* connection, ConduitQueryResponse* response);

#endif // CONDUIT_QUERY_H