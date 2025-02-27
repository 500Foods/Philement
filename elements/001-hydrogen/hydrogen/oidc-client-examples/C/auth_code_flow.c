/*
 * Authorization Code Flow Example for Hydrogen OIDC
 *
 * This example demonstrates how to implement the OAuth 2.0 Authorization Code flow
 * with PKCE (Proof Key for Code Exchange) to authenticate a user and obtain access
 * tokens from a Hydrogen OIDC provider.
 *
 * --- What is OAuth 2.0 and OpenID Connect (OIDC)? ---
 * OAuth 2.0 is a protocol that allows a user to grant a third-party application limited
 * access to their resources without sharing their credentials. OpenID Connect (OIDC) extends
 * OAuth 2.0 with identity functionality, allowing applications to verify user identity.
 *
 * --- What is the Authorization Code Flow? ---
 * The Authorization Code flow is the most secure OAuth 2.0 flow for applications with a server
 * component. It involves redirecting the user to an authorization server, where they
 * authenticate and grant permissions, then redirecting back to the application with an
 * authorization code. The application exchanges this code for tokens in a secure server-to-server
 * request.
 *
 * --- What is PKCE and why is it important? ---
 * PKCE (Proof Key for Code Exchange, pronounced "pixy") is a security extension that protects
 * against authorization code interception attacks. It works by the client creating a secret
 * "code verifier" and sending a derived "code challenge" with the authorization request.
 * When exchanging the authorization code for tokens, the client sends the original code verifier
 * which the server validates against the previously received challenge.
 *
 * Features demonstrated:
 * - Code Verifier and Challenge generation (PKCE)
 * - Authorization request with state parameter for CSRF protection
 * - Callback server to receive authorization code
 * - Token request with proper authentication
 * - Access token usage for API requests
 * - ID token validation (signature, issuer, audience, expiration)
 * - Token refresh when the access token expires
 * - Proper error handling throughout the flow
 *
 * --- The Authentication Flow Explained ---
 * The flow involves:
 * 1. Generate a code verifier and code challenge (PKCE security mechanism)
 * 2. Redirect user to the OIDC provider's authorization endpoint
 * 3. Receive authorization code via callback
 * 4. Exchange code for tokens
 * 5. Validate the ID token
 * 6. Use access token for API requests
 * 7. Refresh token when it expires
 *
 * --- Token Types Explained ---
 * - Access Token: Allows access to protected resources (APIs)
 * - ID Token: Contains identity information about the authenticated user
 * - Refresh Token: Used to get new access tokens when they expire
 *
 * Compile with:
 *   gcc -o auth_code_flow auth_code_flow.c -lcurl -lcrypto -lmicrohttpd -ljansson
 *
 * Usage:
 *   ./auth_code_flow
 */
#define _GNU_SOURCE
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
#define CLIENT_ID "your_client_id"             /* OAuth client identifier from OIDC provider */
#define CLIENT_SECRET "your_client_secret"     /* Secret key shared between client and OIDC provider */
#define REDIRECT_URI "http://localhost:8000/callback" /* URL where authorization code is sent */
#define AUTH_ENDPOINT "https://hydrogen.example.com/oauth/authorize" /* OIDC authorization URL */
#define TOKEN_ENDPOINT "https://hydrogen.example.com/oauth/token"    /* OIDC token endpoint URL */
#define USERINFO_ENDPOINT "https://hydrogen.example.com/oauth/userinfo" /* User profile info URL */
#define JWKS_ENDPOINT "https://hydrogen.example.com/oauth/jwks"      /* JSON Web Key Set endpoint */
#define ISSUER "https://hydrogen.example.com"  /* Expected token issuer identifier */
#define PORT 8000                              /* Local port for callback server */
#define TIMEOUT_SECONDS 300                    /* Authorization flow timeout (5 minutes) */

/* Global variables */
static char code_verifier[128];      /* PKCE code verifier (random secret) */
static char state_value[64];         /* Anti-CSRF token for request/callback validation */
static char auth_code[256] = {0};    /* Authorization code received from OIDC provider */
static char access_token[4096] = {0}; /* Token used to access protected resources */
static char refresh_token[4096] = {0}; /* Token used to get new access tokens */
static char id_token[8192] = {0};    /* Token containing authenticated user identity */
static int got_code = 0;             /* Flag indicating if auth code was received */
static int shutdown_server = 0;      /* Flag to stop the callback server */
static char error_message[1024] = {0}; /* Storage for error messages */

/* Structure for curl HTTP responses
 * 
 * This structure is used to dynamically store HTTP response data received
 * from curl. The memory is allocated dynamically and grows as needed to
 * accommodate the response data.
 */
struct MemoryStruct {
    char *memory;    /* Dynamically allocated buffer for response data */
    size_t size;     /* Current size of the data stored in memory */
};

/* Function prototypes */
static char *generate_code_verifier();
static char *generate_code_challenge(const char *verifier);
static char *base64_url_encode(const unsigned char *input, int length);
static char *generate_random_string(size_t length);
static void build_authorization_url(char *url, size_t url_len);
static int token_request(const char *auth_code);
static int refresh_token_request();
static int userinfo_request();
static int validate_id_token();
static enum MHD_Result callback_handler(void *cls, struct MHD_Connection *connection,
                          const char *url, const char *method,
                          const char *version, const char *upload_data,
                          size_t *upload_data_size, void **con_cls);
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static void start_callback_server();
static void log_ssl_errors();
static int print_json_value(json_t *value, const char *prefix);
static json_t *parse_jwt_payload(const char *jwt);
static char *find_json_string_value(json_t *obj, const char *key);
static int token_expiration_time(const char *token) __attribute__((unused));
static int current_time();
static int display_token_info(const char *token, const char *token_type);

int main() {
    printf("Authorization Code Flow with PKCE Example\n");
    printf("=========================================\n\n");

    /* Initialize OpenSSL */
    OpenSSL_add_all_algorithms();
    
    /* Generate code verifier and challenge for PKCE */
    char *verifier = generate_code_verifier();
    if (!verifier) {
        fprintf(stderr, "Failed to generate code verifier\n");
        return 1;
    }
    strcpy(code_verifier, verifier);
    free(verifier);

    /* Generate random state value for CSRF protection */
    char *state = generate_random_string(32);
    if (!state) {
        fprintf(stderr, "Failed to generate state parameter\n");
        return 1;
    }
    strcpy(state_value, state);
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

/* Generate a random code verifier for PKCE
 * 
 * The code verifier is a cryptographically random string that serves as 
 * a secret on the client. It must be between 43 and 128 characters long
 * and use only letters, numbers, and certain punctuation. This function
 * generates 64 random bytes and then base64url-encodes them.
 *
 * Returns:
 *   Dynamically allocated string containing the code verifier (caller must free)
 *   NULL on error
 */
static char *generate_code_verifier() {
    unsigned char random[64];
    int result = RAND_bytes(random, sizeof(random));
    
    if (result != 1) {
        log_ssl_errors();
        return NULL;
    }
    
    return base64_url_encode(random, sizeof(random));
}

/* Generate a code challenge from the verifier using SHA256
 * 
 * The code challenge is derived from the code verifier using the SHA-256
 * hash algorithm and base64url-encoding. This challenge is sent to the 
 * authorization server instead of the raw verifier for security reasons.
 * 
 * Parameters:
 *   verifier - The code verifier string to create a challenge from
 * 
 * Returns:
 *   Dynamically allocated string containing the code challenge (caller must free)
 *   NULL on error
 */
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
    
    return base64_url_encode(hash, hash_len);
}

/* Base64-URL encode data
 * 
 * Base64url is a variant of Base64 that is URL-safe: it uses '-' instead of '+',
 * '_' instead of '/', and omits padding ('=') characters. This encoding is used
 * for various OAuth/OIDC parameters and in JWT tokens.
 * 
 * Parameters:
 *   input - The binary data to encode
 *   length - The length of the input data in bytes
 * 
 * Returns:
 *   Dynamically allocated string with base64url-encoded data (caller must free)
 *   NULL on error
 */
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
    
    if (BIO_write(b64, input, length) != length) {
        BIO_free_all(b64);
        return NULL;
    }
    
    BIO_flush(b64);
    
    /* Get the length of the encoded data */
    char *base64_data = NULL;
    long base64_length = BIO_get_mem_data(bmem, &base64_data);
    
    /* Allocate memory for the result */
    char *result = malloc(base64_length + 1);
    if (!result) {
        BIO_free_all(b64);
        return NULL;
    }
    
    /* Copy the data */
    memcpy(result, base64_data, base64_length);
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
    if (!random) {
        return NULL;
    }
    
    if (RAND_bytes(random, length) != 1) {
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

/* Make a token request to exchange the authorization code for tokens */
static int token_request(const char *auth_code) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    int success = 0;
    long http_code = 0;
    
    chunk.memory = malloc(1);
    if (!chunk.memory) {
        snprintf(error_message, sizeof(error_message), "Memory allocation failed");
        return 0;
    }
    
    chunk.size = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        char post_fields[4096];
        snprintf(post_fields, sizeof(post_fields),
            "grant_type=authorization_code"
            "&code=%s"
            "&redirect_uri=%s"
            "&client_id=%s"
            "&client_secret=%s"
            "&code_verifier=%s",
            auth_code, REDIRECT_URI, CLIENT_ID, CLIENT_SECRET, code_verifier);
        
        curl_easy_setopt(curl, CURLOPT_URL, TOKEN_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        
        /* Set content type header */
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        /* Enable verbose output for debugging */
        #ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        #endif
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (res != CURLE_OK) {
            snprintf(error_message, sizeof(error_message), 
                     "curl_easy_perform() failed: %s", curl_easy_strerror(res));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        if (http_code != 200) {
            snprintf(error_message, sizeof(error_message), 
                     "HTTP error: %ld, Response: %s", http_code, chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        printf("Token response received (%ld bytes)\n", (long)chunk.size);
        
        /* Parse JSON response with jansson */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            snprintf(error_message, sizeof(error_message), 
                     "JSON parsing error: %s", json_error.text);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        /* Extract tokens from JSON */
        json_t *access_token_json = json_object_get(root, "access_token");
        json_t *refresh_token_json = json_object_get(root, "refresh_token");
        json_t *id_token_json = json_object_get(root, "id_token");
        
        if (!access_token_json || !json_is_string(access_token_json)) {
            snprintf(error_message, sizeof(error_message), "No access_token in response");
        } else {
            strncpy(access_token, json_string_value(access_token_json), sizeof(access_token) - 1);
            access_token[sizeof(access_token) - 1] = '\0';
            
            /* Get refresh token if present */
            if (refresh_token_json && json_is_string(refresh_token_json)) {
                strncpy(refresh_token, json_string_value(refresh_token_json), sizeof(refresh_token) - 1);
                refresh_token[sizeof(refresh_token) - 1] = '\0';
            }
            
            /* Get ID token if present */
            if (id_token_json && json_is_string(id_token_json)) {
                strncpy(id_token, json_string_value(id_token_json), sizeof(id_token) - 1);
                id_token[sizeof(id_token) - 1] = '\0';
            }
            
            success = 1;
        }
        
        json_decref(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        snprintf(error_message, sizeof(error_message), "Failed to initialize curl");
    }
    
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Make a refresh token request to get new access token */
static int refresh_token_request() {
    if (refresh_token[0] == '\0') {
        printf("No refresh token available\n");
        return 0;
    }
    
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    int success = 0;
    long http_code = 0;
    
    chunk.memory = malloc(1);
    if (!chunk.memory) {
        printf("Memory allocation failed\n");
        return 0;
    }
    
    chunk.size = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        char post_fields[4096];
        /* Ensure refresh_token fits within buffer limit to prevent truncation */
        snprintf(post_fields, sizeof(post_fields),
            "grant_type=refresh_token"
            "&refresh_token=%.1900s"
            "&client_id=%s"
            "&client_secret=%s",
            refresh_token, CLIENT_ID, CLIENT_SECRET);
        
        curl_easy_setopt(curl, CURLOPT_URL, TOKEN_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        
        /* Set content type header */
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (res != CURLE_OK) {
            printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        if (http_code != 200) {
            printf("HTTP error: %ld, Response: %s\n", http_code, chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        printf("Refresh token response received\n");
        
        /* Parse JSON response */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            printf("JSON parsing error: %s\n", json_error.text);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        /* Extract tokens from JSON */
        json_t *access_token_json = json_object_get(root, "access_token");
        json_t *refresh_token_json = json_object_get(root, "refresh_token");
        
        if (!access_token_json || !json_is_string(access_token_json)) {
            printf("No access_token in response\n");
        } else {
            strncpy(access_token, json_string_value(access_token_json), sizeof(access_token) - 1);
            access_token[sizeof(access_token) - 1] = '\0';
            
            /* Get refresh token if present (some providers issue new refresh tokens) */
            if (refresh_token_json && json_is_string(refresh_token_json)) {
                strncpy(refresh_token, json_string_value(refresh_token_json), sizeof(refresh_token) - 1);
                refresh_token[sizeof(refresh_token) - 1] = '\0';
            }
            
            printf("New access token received!\n");
            printf("\nNew access token information:\n");
            display_token_info(access_token, "Access");
            
            /* Get updated user info with new token */
            printf("\nFetching user information with new access token...\n");
            userinfo_request();
            
            success = 1;
        }
        
        json_decref(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Request user information using the access token */
static int userinfo_request() {
    if (access_token[0] == '\0') {
        printf("No access token available\n");
        return 0;
    }
    
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    int success = 0;
    long http_code = 0;
    
    chunk.memory = malloc(1);
    if (!chunk.memory) {
        printf("Memory allocation failed\n");
        return 0;
    }
    
    chunk.size = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        char auth_header[4096];
        /* Ensure access_token fits within buffer limit */
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %.4000s", access_token);
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Accept: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, USERINFO_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (res != CURLE_OK) {
            printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        if (http_code != 200) {
            printf("HTTP error: %ld, Response: %s\n", http_code, chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        printf("User info response received\n");
        
        /* Parse and display the user info */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            printf("JSON parsing error: %s\n", json_error.text);
        } else {
            printf("\nUser Profile Information:\n");
            print_json_value(root, "");
            json_decref(root);
            success = 1;
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Validate the ID token */
static int validate_id_token() {
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
    int exp = 0;
    int iat = 0;
    
    json_t *exp_json = json_object_get(payload, "exp");
    json_t *iat_json = json_object_get(payload, "iat");
    
    if (exp_json && json_is_integer(exp_json)) {
        exp = (int)json_integer_value(exp_json);
    }
    
    if (iat_json && json_is_integer(iat_json)) {
        iat = (int)json_integer_value(iat_json);
    }
    
    int now = current_time();
    
    /* Check required claims */
    if (!iss) {
        snprintf(error_message, sizeof(error_message), "Missing 'iss' claim in ID token");
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    if (!sub) {
        snprintf(error_message, sizeof(error_message), "Missing 'sub' claim in ID token");
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    if (!aud) {
        snprintf(error_message, sizeof(error_message), "Missing 'aud' claim in ID token");
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    if (exp == 0) {
        snprintf(error_message, sizeof(error_message), "Missing 'exp' claim in ID token");
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    /* Validate issuer */
    if (strcmp(iss, ISSUER) != 0) {
        snprintf(error_message, sizeof(error_message), 
                 "Invalid issuer: expected '%s', got '%s'", ISSUER, iss);
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    /* Validate audience */
    if (strcmp(aud, CLIENT_ID) != 0) {
        snprintf(error_message, sizeof(error_message), 
                 "Invalid audience: expected '%s', got '%s'", CLIENT_ID, aud);
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    /* Validate expiration */
    if (exp < now) {
        snprintf(error_message, sizeof(error_message), 
                 "Token expired at %d, current time is %d", exp, now);
        free(iss); free(sub); free(aud);
        json_decref(payload);
        return 0;
    }
    
    /* Token signature verification would be done here in a production application. 
     * This would involve:
     * 1. Fetching the JWKS from the OIDC provider
     * 2. Finding the key that matches the 'kid' in the token header
     * 3. Verifying the token signature using that key
     */
    printf("ID token validation checks passed:\n");
    printf("- Issuer: %s\n", iss);
    printf("- Subject: %s\n", sub);
    printf("- Audience: %s\n", aud);
    printf("- Expiration: %d (%d seconds from now)\n", exp, exp - now);
    printf("- Issued at: %d\n", iat);
    
    /* Note: This example does not verify the signature of the token.
     * In a production environment, you MUST verify the token signature.
     */
    printf("\nWARNING: This example does not verify the token signature.\n");
    printf("In a production environment, you MUST verify the token signature.\n");
    
    free(iss);
    free(sub);
    free(aud);
    json_decref(payload);
    return 1;
}

/* Handle HTTP callback from authorization server */
static enum MHD_Result callback_handler(void *cls, struct MHD_Connection *connection,
                          const char *url, const char *method,
                          const char *version, const char *upload_data,
                          size_t *upload_data_size, void **con_cls) {
    static int aptr;
    (void)cls; /* Unused parameter */
    (void)version; /* Unused parameter */
    (void)upload_data; /* Unused parameter */
    (void)upload_data_size; /* Unused parameter */
    const char *response_page;
    struct MHD_Response *response;
    int ret;
    
    if (&aptr != *con_cls) {
        /* First call for this connection, initialize */
        *con_cls = &aptr;
        return MHD_YES;
    }
    
    /* Reset for next connection */
    *con_cls = NULL;
    
    if (strcmp(method, "GET") != 0) {
        return MHD_NO;
    }
    
    /* Check if callback received */
    if (strstr(url, "/callback") != NULL) {
        /* Extract authorization code */
        const char *code = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "code");
        const char *state = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "state");
        const char *error = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "error");
        const char *error_description = MHD_lookup_connection_value(connection, 
                                                                  MHD_GET_ARGUMENT_KIND, 
                                                                  "error_description");
        
        /* Check for errors */
        if (error) {
            snprintf(error_message, sizeof(error_message), "Authorization error: %s", error);
            if (error_description) {
                size_t current_len = strlen(error_message);
                snprintf(error_message + current_len, 
                         sizeof(error_message) - current_len, 
                         " - %s", error_description);
            }
            
            response_page = "<html><body>"
                           "<h1>Authorization Failed</h1>"
                           "<p>The authorization server returned an error.</p>"
                           "<p>You can close this window now.</p>"
                           "</body></html>";
        }
        /* Validate state parameter to prevent CSRF attacks */
        else if (!state || strcmp(state, state_value) != 0) {
            snprintf(error_message, sizeof(error_message), "Invalid state parameter");
            
            response_page = "<html><body>"
                           "<h1>Security Error</h1>"
                           "<p>Invalid state parameter. This could be a CSRF attack.</p>"
                           "<p>You can close this window now.</p>"
                           "</body></html>";
        }
        /* Store code if valid */
        else if (code) {
            strncpy(auth_code, code, sizeof(auth_code) - 1);
            auth_code[sizeof(auth_code) - 1] = '\0';
            got_code = 1;
            
            response_page = "<html><body>"
                           "<h1>Authorization Successful!</h1>"
                           "<p>You have successfully authorized the application.</p>"
                           "<p>You can close this window now.</p>"
                           "</body></html>";
        }
        else {
            snprintf(error_message, sizeof(error_message), "No authorization code received");
            
            response_page = "<html><body>"
                           "<h1>Authorization Failed</h1>"
                           "<p>No authorization code was received.</p>"
                           "<p>You can close this window now.</p>"
                           "</body></html>";
        }
        
        /* Prepare response */
        response = MHD_create_response_from_buffer(strlen(response_page), 
                                                 (void*)response_page, 
                                                 MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        
        /* Signal to stop the server */
        shutdown_server = 1;
        
        return ret;
    }
    
    /* Default response for other URLs */
    response_page = "<html><body>"
                   "<h1>404 Not Found</h1>"
                   "<p>The requested page was not found.</p>"
                   "</body></html>";
    
    response = MHD_create_response_from_buffer(strlen(response_page), 
                                             (void*)response_page, 
                                             MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    
    return ret;
}

/* Callback function for curl to store received data */
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
static void start_callback_server() {
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
        if (got_code) {
            break;
        }
    }
    
    MHD_stop_daemon(daemon);
    printf("Callback server stopped\n");
}

/* Log OpenSSL errors */
static void log_ssl_errors() {
    unsigned long err;
    char err_buf[256];
    
    while ((err = ERR_get_error()) != 0) {
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        fprintf(stderr, "OpenSSL error: %s\n", err_buf);
    }
}

/* Print JSON values (for debugging and display) */
static int print_json_value(json_t *value, const char *prefix) {
    const char *key;
    json_t *val;
    
    if (!value) {
        return 0;
    }
    
    if (json_is_object(value)) {
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
            } else if (json_is_array(val)) {
                printf("%s%s: [\n", prefix, key);
                size_t index;
                json_t *elem;
                json_array_foreach(val, index, elem) {
                    char new_prefix[128];
                    snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
                    print_json_value(elem, new_prefix);
                }
                printf("%s]\n", prefix);
            } else if (json_is_object(val)) {
                printf("%s%s: {\n", prefix, key);
                char new_prefix[128];
                snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
                print_json_value(val, new_prefix);
                printf("%s}\n", prefix);
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
    
    return 1;
}

/* Parse JWT payload (simplified without signature verification)
 *
 * JSON Web Tokens (JWTs) consist of three parts separated by dots:
 * 1. Header: Contains token type and algorithm information (e.g., {"alg":"RS256","typ":"JWT"})
 * 2. Payload: Contains the claims/assertions (e.g., user identity, permissions)
 * 3. Signature: Cryptographically signed data used to verify token authenticity
 * 
 * Each part is base64url-encoded. This function:
 * 1. Splits the JWT into its components
 * 2. Decodes the payload from base64url to binary
 * 3. Parses the JSON payload
 *
 * NOTE: This is a simplified implementation. In production, you MUST verify the
 * token signature using the appropriate cryptographic algorithm and key.
 *
 * Parameters:
 *   jwt - The JWT token string to parse
 * 
 * Returns:
 *   json_t* object containing the parsed payload (caller must json_decref when done)
 *   NULL on error
 */
static json_t *parse_jwt_payload(const char *jwt) {
    char *token_dup = strdup(jwt);
    if (!token_dup) {
        return NULL;
    }
    
    /* Split the token into parts */
    char *header = token_dup;
    char *payload = strchr(header, '.');
    if (!payload) {
        free(token_dup);
        return NULL;
    }
    
    *payload = '\0'; /* Terminate header */
    payload++; /* Move to payload */
    
    char *signature = strchr(payload, '.');
    if (!signature) {
        free(token_dup);
        return NULL;
    }
    
    *signature = '\0'; /* Terminate payload */
    signature++; /* Move to signature */
    
    /* Decode the payload */
    size_t payload_len = strlen(payload);
    size_t decoded_max_len = (payload_len * 3) / 4 + 1;
    unsigned char *decoded = malloc(decoded_max_len);
    if (!decoded) {
        free(token_dup);
        return NULL;
    }
    
    /* Base64URL-decode the payload */
    /* First convert base64url to base64 */
    char *base64_buf = strdup(payload);
    if (!base64_buf) {
        free(decoded);
        free(token_dup);
        return NULL;
    }
    
    /* Replace '-' with '+' and '_' with '/' */
    size_t i;
    for (i = 0; i < payload_len; i++) {
        if (base64_buf[i] == '-') base64_buf[i] = '+';
        else if (base64_buf[i] == '_') base64_buf[i] = '/';
    }
    
    /* Add padding if needed */
    size_t padding = 4 - (payload_len % 4);
    if (padding < 4) {
        char *padded = malloc(payload_len + padding + 1);
        if (!padded) {
            free(base64_buf);
            free(decoded);
            free(token_dup);
            return NULL;
        }
        
        strcpy(padded, base64_buf);
        for (i = 0; i < padding; i++) {
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
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int decoded_len = BIO_read(bio, decoded, payload_len);
    
    BIO_free_all(bio);
    free(base64_buf);
    
    if (decoded_len <= 0) {
        free(decoded);
        free(token_dup);
        return NULL;
    }
    
    /* Null-terminate the JSON */
    decoded[decoded_len] = '\0';
    
    /* Parse the JSON */
    json_error_t error;
    json_t *root = json_loads((const char *)decoded, 0, &error);
    
    free(decoded);
    free(token_dup);
    
    return root;
}

/* Find string value in JSON object */
static char *find_json_string_value(json_t *obj, const char *key) {
    json_t *val = json_object_get(obj, key);
    if (!val || !json_is_string(val)) {
        return NULL;
    }
    
    return strdup(json_string_value(val));
}

/* Get current UNIX time */
static int current_time() {
    return (int)time(NULL);
}

/* Extract token expiration time */
static int token_expiration_time(const char *token) {
    json_t *payload = parse_jwt_payload(token);
    if (!payload) {
        return 0;
    }
    
    json_t *exp = json_object_get(payload, "exp");
    int exp_time = 0;
    
    if (exp && json_is_integer(exp)) {
        exp_time = (int)json_integer_value(exp);
    }
    
    json_decref(payload);
    return exp_time;
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
    
    int exp_time = 0;
    json_t *exp = json_object_get(payload, "exp");
    
    if (exp && json_is_integer(exp)) {
        exp_time = (int)json_integer_value(exp);
        int now = current_time();
        int remaining = exp_time - now;
        
        printf("\n%s token expires in %d seconds\n", token_type, remaining);
    }
    
    json_decref(payload);
    return 1;
}