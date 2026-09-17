/*
 * Firebase engine - REST GET via libcurl, with a Unity injection seam.
 *
 * Production never enqueues fixtures. CURLOPT_NOSIGNAL and a per-request
 * easy handle match the OIDC RP HTTP pattern.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "http.h"

#include <curl/curl.h>
#include <pthread.h>

#define FIREBASE_HTTP_TEST_QUEUE_CAP 8

typedef struct FirebaseHttpTestFixture {
    char* url_substring;
    long http_status;
    char* body;
} FirebaseHttpTestFixture;

static FirebaseHttpTestFixture g_test_queue[FIREBASE_HTTP_TEST_QUEUE_CAP];
static size_t g_test_queue_count = 0;
static pthread_mutex_t g_test_fixture_lock = PTHREAD_MUTEX_INITIALIZER;

FirebaseHttpResponse* firebase_http_response_alloc(void) {
    return calloc(1, sizeof(FirebaseHttpResponse));
}

void firebase_http_response_set_error(FirebaseHttpResponse* response, const char* msg) {
    if (!response) {
        return;
    }
    free(response->error_message);
    response->error_message = msg ? strdup(msg) : NULL;
}

void firebase_http_response_free(FirebaseHttpResponse* response) {
    if (!response) {
        return;
    }
    free(response->body);
    free(response->error_message);
    free(response);
}

// cppcheck-suppress constParameterCallback
// Justification: libcurl CURLOPT_WRITEFUNCTION signature uses char*, not const char*
size_t firebase_http_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    FirebaseHttpBuffer* buf = (FirebaseHttpBuffer*)userdata;
    size_t incoming = size * nmemb;
    if (!buf || !ptr) {
        return 0;
    }
    if (buf->size + incoming > FIREBASE_HTTP_MAX_BODY) {
        return 0;
    }
    if (buf->size + incoming + 1 > buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2 : FIREBASE_HTTP_INITIAL_BUFFER;
        while (new_cap < buf->size + incoming + 1) {
            new_cap *= 2;
        }
        if (new_cap > FIREBASE_HTTP_MAX_BODY + 1) {
            new_cap = FIREBASE_HTTP_MAX_BODY + 1;
        }
        char* grown = realloc(buf->data, new_cap);
        if (!grown) {
            return 0;
        }
        buf->data = grown;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->size, ptr, incoming);
    buf->size += incoming;
    buf->data[buf->size] = '\0';
    return incoming;
}

int firebase_http_xferinfo_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                    curl_off_t ultotal, curl_off_t ulnow) {
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;
    volatile bool* abort_flag = (volatile bool*)clientp;
    if (abort_flag && *abort_flag) {
        return 1;
    }
    return 0;
}

bool firebase_http_url_matches(const char* url, const char* substring) {
    if (!substring || !*substring) {
        return true;
    }
    if (!url) {
        return false;
    }
    return strstr(url, substring) != NULL;
}

bool firebase_http_take_fixture(const char* url, long* out_status, char** out_body) {
    bool taken = false;
    if (!out_status || !out_body) {
        return false;
    }
    pthread_mutex_lock(&g_test_fixture_lock);
    for (size_t i = 0; i < g_test_queue_count; ++i) {
        if (firebase_http_url_matches(url, g_test_queue[i].url_substring)) {
            *out_status = g_test_queue[i].http_status;
            *out_body = g_test_queue[i].body;
            free(g_test_queue[i].url_substring);
            for (size_t j = i + 1; j < g_test_queue_count; ++j) {
                g_test_queue[j - 1] = g_test_queue[j];
            }
            g_test_queue_count--;
            memset(&g_test_queue[g_test_queue_count], 0, sizeof(FirebaseHttpTestFixture));
            taken = true;
            break;
        }
    }
    pthread_mutex_unlock(&g_test_fixture_lock);
    return taken;
}

void firebase_http_test_set_response(const char* url_substring, long http_status, const char* body) {
    pthread_mutex_lock(&g_test_fixture_lock);
    if (g_test_queue_count >= FIREBASE_HTTP_TEST_QUEUE_CAP) {
        free(g_test_queue[0].url_substring);
        free(g_test_queue[0].body);
        for (size_t j = 1; j < g_test_queue_count; ++j) {
            g_test_queue[j - 1] = g_test_queue[j];
        }
        g_test_queue_count--;
    }
    g_test_queue[g_test_queue_count].url_substring =
        (url_substring && *url_substring) ? strdup(url_substring) : NULL;
    g_test_queue[g_test_queue_count].http_status = http_status;
    g_test_queue[g_test_queue_count].body = body ? strdup(body) : NULL;
    g_test_queue_count++;
    pthread_mutex_unlock(&g_test_fixture_lock);
}

void firebase_http_test_clear_responses(void) {
    pthread_mutex_lock(&g_test_fixture_lock);
    for (size_t i = 0; i < g_test_queue_count; ++i) {
        free(g_test_queue[i].url_substring);
        free(g_test_queue[i].body);
    }
    memset(g_test_queue, 0, sizeof(g_test_queue));
    g_test_queue_count = 0;
    pthread_mutex_unlock(&g_test_fixture_lock);
}

void firebase_http_apply_curl_opts(CURL* curl, bool verify_ssl, volatile bool* abort_flag) {
    if (!curl) {
        return;
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, FIREBASE_HTTP_CONNECT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, FIREBASE_HTTP_REQUEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hydrogen-Firebase/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify_ssl ? 2L : 0L);
    if (abort_flag) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, firebase_http_xferinfo_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void*)abort_flag);
    }
}

char* firebase_http_build_documents_url(const char* host, int port, const char* project,
                                        const char* database, bool emulator) {
    if (!host || !*host || !project || !*project) {
        return NULL;
    }
    const char* db = (database && *database) ? database : FIREBASE_DEFAULT_DATABASE;
    const char* scheme = emulator ? "http" : "https";
    int resolved_port = port;
    if (resolved_port <= 0) {
        resolved_port = emulator ? FIREBASE_DEFAULT_EMULATOR_PORT : FIREBASE_DEFAULT_PRODUCTION_PORT;
    }
    char* url = calloc(1, 2048);
    if (!url) {
        return NULL;
    }
    int written = snprintf(url, 2048,
                           "%s://%s:%d/v1/projects/%s/databases/%s/documents/",
                           scheme, host, resolved_port, project, db);
    if (written < 0 || written >= 2048) {
        free(url);
        return NULL;
    }
    return url;
}

bool firebase_http_status_is_healthy(long http_status) {
    return http_status >= 200 && http_status < 300;
}

FirebaseHttpResponse* firebase_http_get(const char* url, const char* auth_header, bool verify_ssl,
                                        CURL** inflight_slot, volatile bool* abort_flag) {
    FirebaseHttpResponse* resp = firebase_http_response_alloc();
    if (!resp) {
        return NULL;
    }

    if (!url || !*url) {
        firebase_http_response_set_error(resp, "URL is NULL or empty");
        return resp;
    }

    long fx_status = 0;
    char* fx_body = NULL;
    if (firebase_http_take_fixture(url, &fx_status, &fx_body)) {
        resp->http_status = fx_status;
        resp->body = fx_body;
        resp->body_size = fx_body ? strlen(fx_body) : 0;
        return resp;
    }

    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        firebase_http_response_set_error(resp, "URL scheme must be http or https");
        return resp;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        firebase_http_response_set_error(resp, "curl_easy_init failed");
        return resp;
    }

    FirebaseHttpBuffer body = {0};
    body.data = calloc(1, FIREBASE_HTTP_INITIAL_BUFFER);
    if (!body.data) {
        curl_easy_cleanup(curl);
        firebase_http_response_set_error(resp, "Failed to allocate response buffer");
        return resp;
    }
    body.capacity = FIREBASE_HTTP_INITIAL_BUFFER;

    struct curl_slist* headers = NULL;
    if (auth_header && *auth_header) {
        headers = curl_slist_append(headers, auth_header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, firebase_http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&body);
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    firebase_http_apply_curl_opts(curl, verify_ssl, abort_flag);

    if (inflight_slot) {
        *inflight_slot = curl;
    }

    CURLcode result = curl_easy_perform(curl);

    if (inflight_slot) {
        *inflight_slot = NULL;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    resp->http_status = http_code;

    if (result == CURLE_OK) {
        resp->body = body.data;
        resp->body_size = body.size;
        body.data = NULL;
    } else {
        free(body.data);
        resp->http_status = 0;
        firebase_http_response_set_error(resp, curl_easy_strerror(result));
        log_this(SR_DATABASE, "Firebase HTTP GET transport error", LOG_LEVEL_ERROR, 0);
    }

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return resp;
}
