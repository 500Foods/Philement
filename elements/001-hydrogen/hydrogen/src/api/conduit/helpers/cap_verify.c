/*
 * ChaCha (Cap) token verification helper.
 *
 * Performs a synchronous POST to the siteverify endpoint and parses
 * the JSON result. Secrets are kept out of logs and error strings.
 */

#include <src/hydrogen.h>
#include <src/api/conduit/helpers/cap_verify.h>

#include <curl/curl.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAP_VERIFY_MAX_BODY (8 * 1024)
#define CAP_VERIFY_INITIAL_BUFFER 1024
#define CAP_VERIFY_CONNECT_TIMEOUT_SECONDS 5L
#define CAP_VERIFY_REQUEST_TIMEOUT_SECONDS 10L

typedef struct CapResponseBuffer {
    char* data;
    size_t size;
    size_t capacity;
} CapResponseBuffer;

static char* cap_verify_post_impl(const char* url,
                                  const char* body,
                                  long* out_status,
                                  char* error_out,
                                  size_t error_sz);

static char* (*cap_verify_post_fn)(const char*,
                                   const char*,
                                   long*,
                                   char*,
                                   size_t) = cap_verify_post_impl;

static size_t cap_verify_write_callback(const void* contents,
                                        size_t size,
                                        size_t nmemb,
                                        void* userp) {
    size_t realsize = size * nmemb;
    CapResponseBuffer* buf = (CapResponseBuffer*)userp;

    if (buf->size + realsize >= CAP_VERIFY_MAX_BODY) {
        return 0;
    }

    if (buf->size + realsize + 1 > buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2 : CAP_VERIFY_INITIAL_BUFFER;
        while (new_cap < buf->size + realsize + 1) {
            new_cap *= 2;
        }
        char* new_data = realloc(buf->data, new_cap);
        if (!new_data) {
            return 0;
        }
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';
    return realsize;
}

static char* cap_verify_post_impl(const char* url,
                                  const char* body,
                                  long* out_status,
                                  char* error_out,
                                  size_t error_sz) {
    *out_status = 0;

    CURL* curl = curl_easy_init();
    if (!curl) {
        snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: curl init failed");
        return NULL;
    }

    CapResponseBuffer response = {0};
    response.data = malloc(CAP_VERIFY_INITIAL_BUFFER);
    if (!response.data) {
        curl_easy_cleanup(curl);
        snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: out of memory");
        return NULL;
    }
    response.data[0] = '\0';
    response.capacity = CAP_VERIFY_INITIAL_BUFFER;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        free(response.data);
        curl_easy_cleanup(curl);
        snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: header alloc failed");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body ? body : "");
    if (body) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cap_verify_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CAP_VERIFY_CONNECT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, CAP_VERIFY_REQUEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hydrogen-Cap-Verify/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, out_status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        free(response.data);
        snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: %s", curl_easy_strerror(result));
        return NULL;
    }

    return response.data;
}

static bool is_valid_http_url(const char* url) {
    if (!url || !*url) {
        return false;
    }
    return (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0);
}

static cap_verify_result_t hard_fail(const char* msg, char* error_out, size_t error_sz) {
    if (error_out && error_sz > 0) {
        snprintf(error_out, error_sz, "%s", msg ? msg : "CAP_VERIFY_FAILED");
    }
    return CAP_VERIFY_HARD_FAIL;
}

static cap_verify_result_t fallback(const char* msg, char* error_out, size_t error_sz) {
    if (error_out && error_sz > 0) {
        snprintf(error_out, error_sz, "%s", msg ? msg : "CAP_VERIFY_FAILED");
    }
    return CAP_VERIFY_FALLBACK;
}

// Classify an HTTP status code from the siteverify endpoint.
// 2xx (specifically 200) is processed further by the caller.
// 5xx is a transient server-side failure -> fallback.
// 4xx and other status codes are treated as hard failures (bad token / client error).
static cap_verify_result_t classify_http_failure(long http_status,
                                                 char* error_out,
                                                 size_t error_sz) {
    char msg[128];
    snprintf(msg, sizeof(msg), "CAP_VERIFY_FAILED: HTTP %ld", http_status);
    if (http_status >= 500 && http_status < 600) {
        return fallback(msg, error_out, error_sz);
    }
    return hard_fail(msg, error_out, error_sz);
}

cap_verify_result_t cap_verify_token(const char* token,
                                     const char* site_id,
                                     char* error_out,
                                     size_t error_sz) {
    if (!error_out || error_sz == 0) {
        return CAP_VERIFY_HARD_FAIL;
    }
    error_out[0] = '\0';

    if (!token || !*token) {
        return hard_fail("CAP_TOKEN_MISSING", error_out, error_sz);
    }

    if (!app_config) {
        return hard_fail("CAP_VERIFY_FAILED: configuration unavailable", error_out, error_sz);
    }

    const char* server = app_config->webserver.chacha_server;
    const char* secret = app_config->webserver.chacha_secret;

    if (!server || !*server) {
        return hard_fail("CAP_VERIFY_FAILED: ChaCha server not configured", error_out, error_sz);
    }
    if (!secret || !*secret) {
        return hard_fail("CAP_VERIFY_FAILED: ChaCha secret not configured", error_out, error_sz);
    }
    if (!is_valid_http_url(server)) {
        return hard_fail("CAP_VERIFY_FAILED: invalid ChaCha server URL", error_out, error_sz);
    }

    char url[1024];
    if (site_id && *site_id) {
        snprintf(url, sizeof(url), "%s/%s/siteverify", server, site_id);
    } else {
        snprintf(url, sizeof(url), "%s/siteverify", server);
    }

    json_t* request_obj = json_object();
    if (!request_obj) {
        return hard_fail("CAP_VERIFY_FAILED: out of memory", error_out, error_sz);
    }
    json_object_set_new(request_obj, "secret", json_string(secret));
    json_object_set_new(request_obj, "response", json_string(token));
    char* request_body = json_dumps(request_obj, JSON_COMPACT);
    json_decref(request_obj);
    if (!request_body) {
        return hard_fail("CAP_VERIFY_FAILED: out of memory", error_out, error_sz);
    }

    long http_status = 0;
    char post_error[256] = {0};
    char* response_body = cap_verify_post_fn(url,
                                             request_body,
                                             &http_status,
                                             post_error,
                                             sizeof(post_error));
    free(request_body);

    if (!response_body) {
        // Transport-level failure (timeout, connection refused, etc.) -> fallback.
        if (post_error[0] != '\0') {
            snprintf(error_out, error_sz, "%s", post_error);
        } else {
            snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: transport error");
        }
        return CAP_VERIFY_FALLBACK;
    }

    if (http_status != 200) {
        free(response_body);
        return classify_http_failure(http_status, error_out, error_sz);
    }

    json_error_t json_err;
    json_t* root = json_loads(response_body, 0, &json_err);
    free(response_body);
    if (!root) {
        return hard_fail("CAP_VERIFY_FAILED: invalid siteverify response", error_out, error_sz);
    }

    json_t* success_field = json_object_get(root, "success");
    bool success = json_is_true(success_field);
    json_decref(root);

    if (!success) {
        return hard_fail("CAP_VERIFY_FAILED: token rejected", error_out, error_sz);
    }

    return CAP_VERIFY_OK;
}

void cap_verify_test_set_post_fn(
    char* (*fn)(const char*, const char*, long*, char*, size_t)) {
    cap_verify_post_fn = fn ? fn : cap_verify_post_impl;
}

void cap_verify_test_clear_post_fn(void) {
    cap_verify_post_fn = cap_verify_post_impl;
}
