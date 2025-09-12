/*
 * Authorization Code Flow Example for Hydrogen OIDC
 *
 * This example demonstrates the OAuth 2.0 Authorization Code flow with PKCE
 * to authenticate a user and obtain access tokens from a Hydrogen OIDC provider.
 *
 * Compile with:
 *   gcc -o auth_code_flow auth_code_flow.c -lcurl -lcrypto -lmicrohttpd -ljansson
 *
 * Usage:
 *   ./auth_code_flow
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <microhttpd.h>
#include <jansson.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

/* Configuration - Replace with your actual values */
#define CLIENT_ID "your_client_id"
#define CLIENT_SECRET "your_client_secret"
#define REDIRECT_URI "http://localhost:8000/callback"
#define AUTH_ENDPOINT "https://hydrogen.example.com/oauth/authorize"
#define TOKEN_ENDPOINT "https://hydrogen.example.com/oauth/token"
#define USERINFO_ENDPOINT "https://hydrogen.example.com/oauth/userinfo"
#define JWKS_ENDPOINT "https://hydrogen.example.com/oauth/jwks"
#define ISSUER "https://hydrogen.example.com"
#define PORT 8000
#define TIMEOUT_SECONDS 300

/* Global variables */
static char code_verifier[128];
static char state_value[64];
static char auth_code[256] = {0};
static char access_token[4096] = {0};
static char refresh_token[4096] = {0};
static char id_token[8192] = {0};
static int got_code = 0;
static int shutdown_server = 0;
static char error_message[1024] = {0};

/* Structure for curl HTTP responses */
struct MemoryStruct {
    char *memory;
    size_t size;
};

/* Function prototypes */
static char *generate_code_verifier(void);
static char *generate_code_challenge(const char *verifier);
static char *base64_url_encode(const unsigned char *input, int length);
static char *generate_random_string(size_t length);
static void build_authorization_url(char *url, size_t url_len);
static int token_request(const char *authorization_code);
static int refresh_token_request(void);
static int userinfo_request(void);
static int validate_id_token(void);
static enum MHD_Result callback_handler(void *cls, struct MHD_Connection *connection,
                          const char *url, const char *method,
                          const char *version, const char *upload_data,
                          size_t *upload_data_size, void **con_cls);
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static void start_callback_server(void);
static void log_ssl_errors(void);
static void print_json_value(json_t *value, const char *prefix);
static json_t *parse_jwt_payload(const char *jwt);
static char *find_json_string_value(json_t *obj, const char *key);
static int current_time(void);
static int display_token_info(const char *token, const char *token_type);
static int perform_http_request(const char *url, struct curl_slist *headers, 
                             const char *post_fields, struct MemoryStruct *chunk, 
                             long *http_code);

int main(void) {
    printf("Authorization Code Flow with PKCE Example\n");
    printf("=========================================\n\n");

    /* Initialize OpenSSL */
    OpenSSL_add_all_algorithms();
    
    /* Generate code verifier and state for PKCE */
    char *verifier = generate_code_verifier();
    char *state = generate_random_string(32);
    if (!verifier || !state) {
        fprintf(stderr, "Failed to generate required parameters\n");
        free(verifier);
        free(state);
        return 1;
    }
    strcpy(code_verifier, verifier);
    strcpy(state_value, state);
    free(verifier);
    free(state);

    /* Build and display authorization URL */
    char auth_url[2048];
    build_authorization_url(auth_url, sizeof(auth_url));
    printf("Please open the following URL in your browser:\n\n%s\n\n", auth_url);
    printf("Waiting for authorization callback...\n");

    /* Start callback server to receive authorization code */
    start_callback_server();

    /* Process the authorization code if received */
    if (strlen(auth_code) > 0) {
        printf("\nAuthorization code received: %s\n", auth_code);
        printf("Exchanging code for tokens...\n");
        
        if (token_request(auth_code)) {
            printf("Tokens received successfully!\n");
            
            /* Validate the ID token */
            printf("\nValidating ID token...\n");
            if (validate_id_token()) {
                printf("ID token validation successful!\n");
                
                /* Display token information */
                printf("\nAccess token information:\n");
                display_token_info(access_token, "Access");
                
                printf("\nID token information:\n");
                display_token_info(id_token, "ID");
                
                /* Use access token to get user information */
                printf("\nFetching user information...\n");
                userinfo_request();
                
                /* Demonstrate token refresh */
                printf("\nSimulating token expiry and refresh...\n");
                printf("Refreshing tokens...\n");
                refresh_token_request();
            } else {
                printf("ID token validation failed: %s\n", error_message);
            }
        } else {
            printf("Failed to obtain tokens: %s\n", error_message);
        }
    } else if (strlen(error_message) > 0) {
        printf("Authorization failed: %s\n", error_message);
    } else {
        printf("No authorization code received. Timeout or user aborted.\n");
    }

    /* Clean up OpenSSL */
    EVP_cleanup();
    return 0;
}

/* Generate a random code verifier for PKCE */
static char *generate_code_verifier(void) {
    unsigned char random[64];
    if (RAND_bytes(random, sizeof(random)) != 1) {
        log_ssl_errors();
        return NULL;
    }
    return base64_url_encode(random, sizeof(random));
}

/* Generate a code challenge from the verifier using SHA256 */
static char *generate_code_challenge(const char *verifier) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        log_ssl_errors();
        return NULL;
    }
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(ctx, verifier, strlen(verifier)) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        log_ssl_errors();
        EVP_MD_CTX_free(ctx);
        return NULL;
    }
    
    EVP_MD_CTX_free(ctx);
    return base64_url_encode(hash, (int)hash_len);
}

/* Base64-URL encode data */
static char *base64_url_encode(const unsigned char *input, int length) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new(BIO_s_mem());
    
    if (!b64 || !bmem) {
        if (b64) BIO_free(b64);
        if (bmem) BIO_free(bmem);
        return NULL;
    }
    
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    
    char *base64_data = NULL;
    long base64_length = BIO_get_mem_data(bmem, &base64_data);
    
    char *result = malloc((size_t)base64_length + 1);
    if (!result) {
        BIO_free_all(b64);
        return NULL;
    }
    
    memcpy(result, base64_data, (size_t)base64_length);
    result[base64_length] = 0;
    
    /* Base64-URL encoding: replace '+' with '-', '/' with '_', and remove '=' */
    for (char *p = result; *p; p++) {
        if (*p == '+') *p = '-';
        else if (*p == '/') *p = '_';
        else if (*p == '=') {
            *p = '\0';
            break;
        }
    }
    
    BIO_free_all(b64);
    return result;
}

/* Generate a random string for the state parameter */
static char *generate_random_string(size_t length) {
    static const char charset[] = 
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    unsigned char *random = malloc(length);
    if (!random) return NULL;
    
    if (RAND_bytes(random, (int)length) != 1) {
        log_ssl_errors();
        free(random);
        return NULL;
    }
    
    char *result = malloc(length + 1);
    if (!result) {
        free(random);
        return NULL;
    }
    
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[random[i] % (sizeof(charset) - 1)];
    }
    
    result[length] = 0;
    free(random);
    return result;
}

/* Build the authorization URL with all required parameters */
static void build_authorization_url(char *url, size_t url_len) {
    char *challenge = generate_code_challenge(code_verifier);
    if (!challenge) {
        snprintf(url, url_len, "Error generating code challenge");
        return;
    }
    
    snprintf(url, url_len, 
        "%s"
        "?client_id=%s"
        "&redirect_uri=%s"
        "&response_type=code"
        "&scope=openid profile email"
        "&code_challenge=%s"
        "&code_challenge_method=S256"
        "&state=%s",
        AUTH_ENDPOINT, CLIENT_ID, REDIRECT_URI, challenge, state_value);
    
    free(challenge);
}

/* Helper function to perform HTTP requests */
static int perform_http_request(const char *url, struct curl_slist *headers, 
                             const char *post_fields, struct MemoryStruct *chunk, 
                             long *http_code) {
    CURL *curl;
    CURLcode res;
    int success = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (post_fields) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
        
        if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        #ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        #endif
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
        
        if (res != CURLE_OK) {
            snprintf(error_message, sizeof(error_message), 
                     "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else if (*http_code != 200) {
            snprintf(error_message, sizeof(error_message), 
                     "HTTP error: %ld, Response: %s", *http_code, chunk->memory);
        } else {
            success = 1;
        }
        
        curl_easy_cleanup(curl);
    } else {
        snprintf(error_message, sizeof(error_message), "Failed to initialize curl");
    }
    
    return success;
}

/* Make a token request to exchange the authorization code for tokens */
static int token_request(const char *authorization_code) {
    struct MemoryStruct chunk = {malloc(1), 0};
    long http_code = 0;
    
    if (!chunk.memory) {
        snprintf(error_message, sizeof(error_message), "Memory allocation failed");
        return 0;
    }
    
    /* Prepare headers and post fields */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, "Accept: application/json");
    
    char post_fields[4096];
    snprintf(post_fields, sizeof(post_fields),
        "grant_type=authorization_code"
        "&code=%s"
        "&redirect_uri=%s"
        "&client_id=%s"
        "&client_secret=%s"
        "&code_verifier=%s",
        authorization_code, REDIRECT_URI, CLIENT_ID, CLIENT_SECRET, code_verifier);
    
    /* Perform the request */
    int success = perform_http_request(TOKEN_ENDPOINT, headers, post_fields, &chunk, &http_code);
    
    if (success) {
        printf("Token response received (%ld bytes)\n", (long)chunk.size);
        
        /* Parse JSON response */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            snprintf(error_message, sizeof(error_message), "JSON parsing error: %s", json_error.text);
            success = 0;
        } else {
            /* Extract tokens from JSON */
            json_t *access_token_json = json_object_get(root, "access_token");
            json_t *refresh_token_json = json_object_get(root, "refresh_token");
            json_t *id_token_json = json_object_get(root, "id_token");
            
            if (!access_token_json || !json_is_string(access_token_json)) {
                snprintf(error_message, sizeof(error_message), "No access_token in response");
                success = 0;
            } else {
                strncpy(access_token, json_string_value(access_token_json), sizeof(access_token) - 1);
                
                if (refresh_token_json && json_is_string(refresh_token_json))
                    strncpy(refresh_token, json_string_value(refresh_token_json), sizeof(refresh_token) - 1);
                
                if (id_token_json && json_is_string(id_token_json))
                    strncpy(id_token, json_string_value(id_token_json), sizeof(id_token) - 1);
            }
            json_decref(root);
        }
    }
    
    curl_slist_free_all(headers);
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Make a refresh token request to get new access token */
static int refresh_token_request(void) {
    if (refresh_token[0] == '\0') {
        printf("No refresh token available\n");
        return 0;
    }
    
    struct MemoryStruct chunk = {malloc(1), 0};
    long http_code = 0;
    
    if (!chunk.memory) {
        printf("Memory allocation failed\n");
        return 0;
    }
    
    /* Prepare headers and post fields */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, "Accept: application/json");
    
    char post_fields[4096];
    snprintf(post_fields, sizeof(post_fields),
        "grant_type=refresh_token"
        "&refresh_token=%.1900s"
        "&client_id=%s"
        "&client_secret=%s",
        refresh_token, CLIENT_ID, CLIENT_SECRET);
    
    /* Perform the request */
    int success = perform_http_request(TOKEN_ENDPOINT, headers, post_fields, &chunk, &http_code);
    
    if (success) {
        printf("Refresh token response received\n");
        
        /* Parse JSON response */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            printf("JSON parsing error: %s\n", json_error.text);
            success = 0;
        } else {
            /* Extract tokens from JSON */
            json_t *access_token_json = json_object_get(root, "access_token");
            json_t *refresh_token_json = json_object_get(root, "refresh_token");
            
            if (!access_token_json || !json_is_string(access_token_json)) {
                printf("No access_token in response\n");
                success = 0;
            } else {
                strncpy(access_token, json_string_value(access_token_json), sizeof(access_token) - 1);
                
                if (refresh_token_json && json_is_string(refresh_token_json))
                    strncpy(refresh_token, json_string_value(refresh_token_json), sizeof(refresh_token) - 1);
                
                printf("New access token received!\n");
                printf("\nNew access token information:\n");
                display_token_info(access_token, "Access");
                
                /* Get updated user info with new token */
                printf("\nFetching user information with new access token...\n");
                userinfo_request();
            }
            json_decref(root);
        }
    }
    
    curl_slist_free_all(headers);
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Request user information using the access token */
static int userinfo_request(void) {
    if (access_token[0] == '\0') {
        printf("No access token available\n");
        return 0;
    }
    
    struct MemoryStruct chunk = {malloc(1), 0};
    long http_code = 0;
    
    if (!chunk.memory) {
        printf("Memory allocation failed\n");
        return 0;
    }
    
    /* Prepare headers */
    char auth_header[4096];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %.4000s", access_token);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Accept: application/json");
    
    /* Perform the request */
    int success = perform_http_request(USERINFO_ENDPOINT, headers, NULL, &chunk, &http_code);
    
    if (success) {
        printf("User info response received\n");
        
        /* Parse and display the user info */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            printf("JSON parsing error: %s\n", json_error.text);
            success = 0;
        } else {
            printf("\nUser Profile Information:\n");
            print_json_value(root, "");
            json_decref(root);
        }
    }
    
    curl_slist_free_all(headers);
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Validate the ID token */
static int validate_id_token(void) {
    if (id_token[0] == '\0') {
        snprintf(error_message, sizeof(error_message), "No ID token available");
        return 0;
    }
    
    /* Parse the ID token payload */
    json_t *payload = parse_jwt_payload(id_token);
    if (!payload) {
        snprintf(error_message, sizeof(error_message), "Failed to parse ID token");
        return 0;
    }
    
    /* Validate required claims */
    char *iss = find_json_string_value(payload, "iss");
    char *sub = find_json_string_value(payload, "sub");
    char *aud = find_json_string_value(payload, "aud");
    int exp = 0, iat = 0;
    
    json_t *exp_json = json_object_get(payload, "exp");
    json_t *iat_json = json_object_get(payload, "iat");
    
    if (exp_json && json_is_integer(exp_json)) exp = (int)json_integer_value(exp_json);
    if (iat_json && json_is_integer(iat_json)) iat = (int)json_integer_value(iat_json);
    
    int now = current_time();
    int validation_failed = 0;
    
    /* Check required claims */
    if (!iss) {
        snprintf(error_message, sizeof(error_message), "Missing 'iss' claim in ID token");
        validation_failed = 1;
    } else if (strcmp(iss, ISSUER) != 0) {
        snprintf(error_message, sizeof(error_message), 
                "Invalid issuer: expected '%s', got '%s'", ISSUER, iss);
        validation_failed = 1;
    }
    
    if (!sub && !validation_failed) {
        snprintf(error_message, sizeof(error_message), "Missing 'sub' claim in ID token");
        validation_failed = 1;
    }
    
    if (!aud && !validation_failed) {
        snprintf(error_message, sizeof(error_message), "Missing 'aud' claim in ID token");
        validation_failed = 1;
    } else if (strcmp(aud, CLIENT_ID) != 0 && !validation_failed) {
        snprintf(error_message, sizeof(error_message), 
                "Invalid audience: expected '%s', got '%s'", CLIENT_ID, aud);
        validation_failed = 1;
    }
    
    if (exp == 0 && !validation_failed) {
        snprintf(error_message, sizeof(error_message), "Missing 'exp' claim in ID token");
        validation_failed = 1;
    } else if (exp < now && !validation_failed) {
        snprintf(error_message, sizeof(error_message), 
                "Token expired at %d, current time is %d", exp, now);
        validation_failed = 1;
    }
    
    free(iss); free(sub); free(aud);
    json_decref(payload);
    
    if (validation_failed) return 0;
    
    /* Token signature verification would be done here in a production application */
    printf("ID token validation checks passed:\n");
    printf("- Issuer: %s\n", ISSUER);
    printf("- Subject: Present\n");
    printf("- Audience: %s\n", CLIENT_ID);
    printf("- Expiration: %d (%d seconds from now)\n", exp, exp - now);
    printf("- Issued at: %d\n", iat);
    
    printf("\nWARNING: This example does not verify the token signature.\n");
    printf("In a production environment, you MUST verify the token signature.\n");
    
    return 1;
}

/* Handle HTTP callback from authorization server */
static enum MHD_Result callback_handler(void *cls, struct MHD_Connection *connection,
                          const char *url, const char *method,
                          const char *version, const char *upload_data,
                          size_t *upload_data_size, void **con_cls) {
    static int aptr;
    const char *response_page;
    struct MHD_Response *response;
    int ret;
    
    (void)cls; (void)version; (void)upload_data; (void)upload_data_size;
    
    if (&aptr != *con_cls) {
        *con_cls = &aptr;
        return MHD_YES;
    }
    
    *con_cls = NULL;
    
    if (strcmp(method, "GET") != 0) return MHD_NO;
    
    /* Check if callback received */
    if (strstr(url, "/callback") != NULL) {
        /* Extract authorization code */
        const char *code = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "code");
        const char *state = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "state");
        const char *error = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "error");
        const char *error_description = MHD_lookup_connection_value(connection, 
                                                                 MHD_GET_ARGUMENT_KIND, 
                                                                 "error_description");
        
        /* Handle different callback scenarios */
        if (error) {
            snprintf(error_message, sizeof(error_message), "Authorization error: %s", error);
            if (error_description) {
                size_t len = strlen(error_message);
                snprintf(error_message + len, sizeof(error_message) - len, " - %s", error_description);
            }
            response_page = "<html><body><h1>Authorization Failed</h1><p>The authorization server returned an error.</p><p>You can close this window now.</p></body></html>";
        }
        else if (!state || strcmp(state, state_value) != 0) {
            snprintf(error_message, sizeof(error_message), "Invalid state parameter");
            response_page = "<html><body><h1>Security Error</h1><p>Invalid state parameter. This could be a CSRF attack.</p><p>You can close this window now.</p></body></html>";
        }
        else if (code) {
            strncpy(auth_code, code, sizeof(auth_code) - 1);
            auth_code[sizeof(auth_code) - 1] = '\0';
            got_code = 1;
            response_page = "<html><body><h1>Authorization Successful!</h1><p>You have successfully authorized the application.</p><p>You can close this window now.</p></body></html>";
        }
        else {
            snprintf(error_message, sizeof(error_message), "No authorization code received");
            response_page = "<html><body><h1>Authorization Failed</h1><p>No authorization code was received.</p><p>You can close this window now.</p></body></html>";
        }
        
        /* Send response and signal to stop the server */
        response = MHD_create_response_from_buffer(strlen(response_page), (void*)response_page, MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        shutdown_server = 1;
        return ret;
    }
    
    /* Default response for other URLs */
    response_page = "<html><body><h1>404 Not Found</h1><p>The requested page was not found.</p></body></html>";
    response = MHD_create_response_from_buffer(strlen(response_page), (void*)response_page, MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

/* Callback function for curl to store received data */
// cppcheck-suppress[constParameterCallback] - libcurl callback signature requires void*
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("Not enough memory\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

/* Start the callback server to receive the authorization code */
static void start_callback_server(void) {
    struct MHD_Daemon *daemon;
    
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                           &callback_handler, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start server\n");
        return;
    }
    
    printf("Callback server started on port %d\n", PORT);
    printf("Waiting for authorization response...\n");
    
    /* Wait for authorization code or timeout */
    time_t start_time = time(NULL);
    while (!shutdown_server && time(NULL) - start_time < TIMEOUT_SECONDS) {
        sleep(1);
        if (got_code) break;
    }
    
    MHD_stop_daemon(daemon);
    printf("Callback server stopped\n");
}

/* Log OpenSSL errors */
static void log_ssl_errors(void) {
    unsigned long err;
    char err_buf[256];
    
    while ((err = ERR_get_error()) != 0) {
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        fprintf(stderr, "OpenSSL error: %s\n", err_buf);
    }
}

/* Print JSON values recursively */
static void print_json_value(json_t *value, const char *prefix) {
    if (!value) return;
    
    if (json_is_object(value)) {
        const char *key;
        json_t *val;
        
        json_object_foreach(value, key, val) {
            if (json_is_string(val)) {
                printf("%s%s: %s\n", prefix, key, json_string_value(val));
            } else if (json_is_integer(val)) {
                printf("%s%s: %lld\n", prefix, key, json_integer_value(val));
            } else if (json_is_real(val)) {
                printf("%s%s: %f\n", prefix, key, json_real_value(val));
            } else if (json_is_true(val)) {
                printf("%s%s: true\n", prefix, key);
            } else if (json_is_false(val)) {
                printf("%s%s: false\n", prefix, key);
            } else if (json_is_null(val)) {
                printf("%s%s: null\n", prefix, key);
            } else {
                char new_prefix[128];
                snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
                if (json_is_array(val)) {
                    printf("%s%s: [\n", prefix, key);
                    print_json_value(val, new_prefix);
                    printf("%s]\n", prefix);
                } else if (json_is_object(val)) {
                    printf("%s%s: {\n", prefix, key);
                    print_json_value(val, new_prefix);
                    printf("%s}\n", prefix);
                }
            }
        }
    } else if (json_is_array(value)) {
        size_t index;
        json_t *elem;
        json_array_foreach(value, index, elem) {
            if (json_is_string(elem)) {
                printf("%s[%zu]: %s\n", prefix, index, json_string_value(elem));
            } else if (json_is_integer(elem)) {
                printf("%s[%zu]: %lld\n", prefix, index, json_integer_value(elem));
            } else {
                char new_prefix[128];
                snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
                print_json_value(elem, new_prefix);
            }
        }
    } else if (json_is_string(value)) {
        printf("%s%s\n", prefix, json_string_value(value));
    } else if (json_is_integer(value)) {
        printf("%s%lld\n", prefix, json_integer_value(value));
    } else if (json_is_real(value)) {
        printf("%s%f\n", prefix, json_real_value(value));
    } else if (json_is_true(value)) {
        printf("%strue\n", prefix);
    } else if (json_is_false(value)) {
        printf("%sfalse\n", prefix);
    } else if (json_is_null(value)) {
        printf("%snull\n", prefix);
    }
}

/* Parse JWT payload (simplified without signature verification) */
static json_t *parse_jwt_payload(const char *jwt) {
    char *token_dup = strdup(jwt);
    if (!token_dup) return NULL;
    
    /* Split the token into parts */
    char *header = token_dup;
    char *payload = strchr(header, '.');
    if (!payload) {
        free(token_dup);
        return NULL;
    }
    
    *payload++ = '\0'; /* Terminate header and move to payload */
    
    char *signature = strchr(payload, '.');
    if (!signature) {
        free(token_dup);
        return NULL;
    }
    
    *signature = '\0'; /* Terminate payload */
    
    /* Base64URL-decode the payload */
    size_t payload_len = strlen(payload);
    char *base64_buf = strdup(payload);
    if (!base64_buf) {
        free(token_dup);
        return NULL;
    }
    
    /* Replace '-' with '+' and '_' with '/' */
    for (size_t i = 0; i < payload_len; i++) {
        if (base64_buf[i] == '-') base64_buf[i] = '+';
        else if (base64_buf[i] == '_') base64_buf[i] = '/';
    }
    
    /* Add padding if needed */
    size_t padding = 4 - (payload_len % 4);
    if (padding < 4) {
        char *padded = malloc(payload_len + padding + 1);
        if (!padded) {
            free(base64_buf);
            free(token_dup);
            return NULL;
        }
        
        strcpy(padded, base64_buf);
        for (size_t i = 0; i < padding; i++) {
            padded[payload_len + i] = '=';
        }
        padded[payload_len + padding] = '\0';
        
        free(base64_buf);
        base64_buf = padded;
    }
    
    /* Decode using OpenSSL */
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new_mem_buf(base64_buf, -1);
    bio = BIO_push(b64, bio);
    
    size_t decoded_max_len = (payload_len * 3) / 4 + 1;
    unsigned char *decoded = malloc(decoded_max_len);
    if (!decoded) {
        BIO_free_all(bio);
        free(base64_buf);
        free(token_dup);
        return NULL;
    }
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int decoded_len = BIO_read(bio, decoded, (int)payload_len);
    
    BIO_free_all(bio);
    free(base64_buf);
    
    if (decoded_len <= 0) {
        free(decoded);
        free(token_dup);
        return NULL;
    }
    
    /* Null-terminate the JSON and parse it */
    decoded[decoded_len] = '\0';
    json_error_t error;
    json_t *root = json_loads((const char *)decoded, 0, &error);
    
    free(decoded);
    free(token_dup);
    return root;
}

/* Find string value in JSON object */
static char *find_json_string_value(json_t *obj, const char *key) {
    json_t *val = json_object_get(obj, key);
    if (!val || !json_is_string(val)) return NULL;
    return strdup(json_string_value(val));
}

/* Get current UNIX time */
static int current_time(void) {
    return (int)time(NULL);
}

/* Display token information */
static int display_token_info(const char *token, const char *token_type) {
    if (!token || token[0] == '\0') {
        printf("%s token not available\n", token_type);
        return 0;
    }
    
    json_t *payload = parse_jwt_payload(token);
    if (!payload) {
        printf("Failed to parse %s token\n", token_type);
        return 0;
    }
    
    printf("%s token payload:\n", token_type);
    print_json_value(payload, "  ");
    
    json_t *exp = json_object_get(payload, "exp");
    if (exp && json_is_integer(exp)) {
        int exp_time = (int)json_integer_value(exp);
        int now = current_time();
        printf("\n%s token expires in %d seconds\n", token_type, exp_time - now);
    }
    
    json_decref(payload);
    return 1;
}
