/*
 * Firebase engine - REST HTTP seam (libcurl, injectable for Unity).
 */

#ifndef DATABASE_ENGINE_FIREBASE_HTTP_H
#define DATABASE_ENGINE_FIREBASE_HTTP_H

#include <src/database/database.h>
#include <curl/curl.h>

#define FIREBASE_HTTP_MAX_BODY (1024 * 1024)
#define FIREBASE_HTTP_INITIAL_BUFFER 4096
#define FIREBASE_HTTP_CONNECT_TIMEOUT_SECONDS 10L
#define FIREBASE_HTTP_REQUEST_TIMEOUT_SECONDS 5L

typedef struct FirebaseHttpResponse {
    long http_status;
    char* body;
    size_t body_size;
    char* error_message;
} FirebaseHttpResponse;

typedef struct FirebaseHttpBuffer {
    char* data;
    size_t size;
    size_t capacity;
} FirebaseHttpBuffer;

FirebaseHttpResponse* firebase_http_response_alloc(void);
void firebase_http_response_free(FirebaseHttpResponse* response);
void firebase_http_response_set_error(FirebaseHttpResponse* response, const char* msg);

size_t firebase_http_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
int firebase_http_xferinfo_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                    curl_off_t ultotal, curl_off_t ulnow);

bool firebase_http_url_matches(const char* url, const char* substring);
bool firebase_http_take_fixture(const char* url, long* out_status, char** out_body);

void firebase_http_test_set_response(const char* url_substring, long http_status, const char* body);
void firebase_http_test_clear_responses(void);

void firebase_http_apply_curl_opts(CURL* curl, bool verify_ssl, volatile bool* abort_flag);

char* firebase_http_build_documents_url(const char* host, int port, const char* project,
                                        const char* database, bool emulator);

bool firebase_http_status_is_healthy(long http_status);

FirebaseHttpResponse* firebase_http_get(const char* url, const char* auth_header, bool verify_ssl,
                                        CURL** inflight_slot, volatile bool* abort_flag);

#endif /* DATABASE_ENGINE_FIREBASE_HTTP_H */
