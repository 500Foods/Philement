/*
 * Client Credentials Flow Example for Hydrogen OIDC
 *
 * This example demonstrates how to implement the OAuth 2.0 Client Credentials flow
 * for service-to-service authentication without user involvement.
 *
 * --- What is the Client Credentials Flow? ---
 * The Client Credentials flow is designed specifically for server-to-server 
 * authentication scenarios where a human user is not directly involved. Instead of 
 * authenticating a user, this flow authenticates the application itself using its
 * client credentials (client ID and client secret).
 *
 * When to use Client Credentials flow:
 * - Microservices communicating with each other
 * - Backend services accessing protected APIs
 * - Scheduled jobs/batch processes needing API access
 * - Any scenario where a user is not present to authenticate
 *
 * Features demonstrated:
 * - Client credentials authentication
 * - Access token request
 * - Token response parsing with proper JSON library
 * - Error handling and reporting
 * - Making authenticated API requests
 * - Token validation
 *
 * --- The Authentication Flow Explained ---
 * The flow involves:
 * 1. Application directly requests an access token using client credentials (client ID and secret)
 * 2. Application validates the received token (checking expiration time, scope, etc.)
 * 3. Application uses the access token to access protected resources
 *
 * This flow is typically used for microservices, background processes, and
 * other scenarios where a specific user is not involved.
 *
 * Compile with:
 *   gcc -o client_credentials client_credentials.c -lcurl -ljansson
 *
 * Usage:
 *   ./client_credentials
 */

/* --- Differences from Other OAuth Flows ---
 * Unlike Authorization Code flow and Password flow, the Client Credentials flow:
 * - Does not involve redirects or user interfaces
 * - Does not authenticate a specific end-user
 * - Usually receives tokens with a more limited scope
 * - Typically does not receive refresh tokens (tokens are short-lived and reacquired)
 * - Focuses on application-level permissions rather than user-level permissions
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <time.h>

/* Configuration - Replace with your actual values */
#define CLIENT_ID "your_service_client_id"      /* Client identifier for the service/application */
#define CLIENT_SECRET "your_service_client_secret" /* Secret key known only to the client and server */
#define TOKEN_ENDPOINT "https://hydrogen.example.com/oauth/token" /* OIDC token endpoint URL */
#define API_ENDPOINT "https://hydrogen.example.com/api/protected-resource" /* Protected API URL */
#define EXPECTED_SCOPE "service"                /* Expected token scope for access control */
#define REQUEST_TIMEOUT_SECONDS 30              /* Network request timeout */

/* Structure for curl HTTP responses
 * 
 * This structure is used to store HTTP response data received from curl.
 * The memory is allocated dynamically and grows as needed to accommodate
 * the response data.
 */
struct MemoryStruct {
    char *memory;   /* Dynamically allocated buffer for response data */
    size_t size;    /* Current size of the data stored in memory */
};

/* Function prototypes */
static int get_access_token(char *access_token, size_t token_size, char *error_message, size_t error_size);
static int call_protected_api(const char *access_token, char *error_message, size_t error_size);
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static int validate_token(const char *token, char *error_message, size_t error_size);
static json_t *parse_jwt_payload(const char *jwt);
static char *find_json_string_value(json_t *obj, const char *key);
static int display_token_info(const char *token);
static int current_time(void);

int main(void) {
    printf("Client Credentials Flow Example\n");
    printf("===============================\n\n");

    /* Get access token using client credentials */
    char access_token[4096] = {0};
    char error_message[1024] = {0};
    
    printf("Requesting access token...\n");
    
    if (get_access_token(access_token, sizeof(access_token), error_message, sizeof(error_message))) {
        printf("Access token received!\n\n");
        
        /* Display token information */
        printf("Access token information:\n");
        display_token_info(access_token);
        
        /* Validate the token */
        printf("\nValidating token...\n");
        if (validate_token(access_token, error_message, sizeof(error_message))) {
            printf("Token validation successful!\n");
            
            /* Use access token to call protected API */
            printf("\nCalling protected API...\n");
            if (!call_protected_api(access_token, error_message, sizeof(error_message))) {
                printf("API call failed: %s\n", error_message);
            }
        } else {
            printf("Token validation failed: %s\n", error_message);
        }
    } else {
        printf("Failed to get access token: %s\n", error_message);
    }

    return 0;
}

/* Request an access token using client credentials
 * 
 * This function implements the core of the Client Credentials flow by making
 * a POST request to the token endpoint with the client credentials. It handles
 * the HTTP request, JSON response parsing, and token extraction.
 *
 * The token endpoint request format follows the OAuth 2.0 specification:
 * - grant_type=client_credentials (identifies the flow type)
 * - client_id and client_secret (authenticates the application)
 * - scope (optional, requests specific permissions)
 *
 * The response is a JSON object typically containing:
 * - access_token: The token to use for API access
 * - token_type: Usually "Bearer"
 * - expires_in: Lifetime of the token in seconds
 * - scope: The scopes granted (may differ from requested)
 *
 * Parameters:
 *   access_token - Buffer to store the received access token
 *   token_size - Size of the access_token buffer
 *   error_message - Buffer to store error details if the function fails
 *   error_size - Size of the error_message buffer
 * 
 * Returns:
 *   1 on success, 0 on failure (with error_message populated)
 */
static int get_access_token(char *access_token, size_t token_size, char *error_message, size_t error_size) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    int success = 0;
    long http_code = 0;
    
    /* Initialize memory for response */
    chunk.memory = malloc(1);
    if (!chunk.memory) {
        snprintf(error_message, error_size, "Memory allocation failed");
        return 0;
    }
    chunk.size = 0;
    
    /* Initialize curl */
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        /* Prepare request body */
        char post_fields[1024];
        snprintf(post_fields, sizeof(post_fields),
            "grant_type=client_credentials"
            "&client_id=%s"
            "&client_secret=%s"
            "&scope=%s",
            CLIENT_ID, CLIENT_SECRET, EXPECTED_SCOPE);
        
        /* Configure curl */
        curl_easy_setopt(curl, CURLOPT_URL, TOKEN_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)REQUEST_TIMEOUT_SECONDS);

        /* Set content type header */
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        /* Enable verbose output for debugging */
        #ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        #endif
        
        /* Perform request */
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        /* Check for curl errors */
        if (res != CURLE_OK) {
            snprintf(error_message, error_size, "curl_easy_perform() failed: %s", 
                     curl_easy_strerror(res));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        /* Check HTTP status code */
        if (http_code != 200) {
            snprintf(error_message, error_size, "HTTP error: %ld, Response: %s", 
                     http_code, chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        printf("Token response received (%ld bytes)\n", (long)chunk.size);
        
        /* Parse JSON response with Jansson library
         *
         * The Jansson library provides a simple API for working with JSON data:
         * - json_loads(): Parse JSON string into a JSON object
         * - json_object_get(): Get a value from a JSON object by key
         * - json_is_string(), json_is_integer(), etc.: Check value types
         * - json_string_value(): Extract string value from a JSON string object
         * - json_decref(): Free the JSON object when done
         */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            snprintf(error_message, error_size, "JSON parsing error: %s", json_error.text);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        /* Extract tokens from JSON */
        json_t *access_token_json = json_object_get(root, "access_token");
        json_t *token_type_json = json_object_get(root, "token_type");
        json_t *expires_in_json = json_object_get(root, "expires_in");
        json_t *scope_json = json_object_get(root, "scope");
        
        if (!access_token_json || !json_is_string(access_token_json)) {
            snprintf(error_message, error_size, "No access_token in response");
        } else {
            const char *token_value = json_string_value(access_token_json);
            if (strlen(token_value) >= token_size) {
                snprintf(error_message, error_size, "Access token too large for buffer");
            } else {
                strcpy(access_token, token_value);
                
                /* Display token details */
                printf("Token details:\n");
                
                if (token_type_json && json_is_string(token_type_json)) {
                    printf("- Type: %s\n", json_string_value(token_type_json));
                }
                
                if (expires_in_json && json_is_integer(expires_in_json)) {
                    printf("- Expires in: %lld seconds\n", json_integer_value(expires_in_json));
                }
                
                if (scope_json && json_is_string(scope_json)) {
                    printf("- Scope: %s\n", json_string_value(scope_json));
                }
                
                success = 1;
            }
        }
        
        json_decref(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        snprintf(error_message, error_size, "Failed to initialize curl");
    }
    
    /* Clean up */
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Call a protected API using the access token
 * 
 * This function demonstrates how to use an access token to call a protected API.
 * It sets the Authorization header with the Bearer token and makes an HTTP request.
 * 
 * In OAuth 2.0, protected APIs require a valid access token in the Authorization
 * header using the Bearer authentication scheme:
 *   Authorization: Bearer <token>
 *
 * Parameters:
 *   access_token - The access token to use for authentication
 *   error_message - Buffer to store error details if the function fails
 *   error_size - Size of the error_message buffer
 * 
 * Returns:
 *   1 on success, 0 on failure (with error_message populated)
 */
static int call_protected_api(const char *access_token, char *error_message, size_t error_size) {
    CURL *curl;
    struct MemoryStruct chunk;
    int success = 0;
    long http_code = 0;
    
    /* Initialize memory for response */
    chunk.memory = malloc(1);
    if (!chunk.memory) {
        snprintf(error_message, error_size, "Memory allocation failed");
        return 0;
    }
    chunk.size = 0;
    
    /* Initialize curl */
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        /* Prepare authorization header */
        char auth_header[4096];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);
        
        /* Configure curl */
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Accept: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, API_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)REQUEST_TIMEOUT_SECONDS);
        
        /* Perform request */
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        /* Check for curl errors */
        if (res != CURLE_OK) {
            snprintf(error_message, error_size, "curl_easy_perform() failed: %s", 
                     curl_easy_strerror(res));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return 0;
        }
        
        /* Handle HTTP status codes */
        if (http_code == 200) {
            printf("API call successful\n");
            printf("Response: %s\n", chunk.memory);
            success = 1;
        } else if (http_code == 401) {
            snprintf(error_message, error_size, "Authentication failed. Token may be invalid or expired.");
        } else if (http_code == 403) {
            snprintf(error_message, error_size, "Access forbidden. Insufficient permissions.");
        } else {
            snprintf(error_message, error_size, "HTTP error: %ld, Response: %s", http_code, chunk.memory);
        }
        
        /* Clean up */
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        snprintf(error_message, error_size, "Failed to initialize curl");
    }
    
    free(chunk.memory);
    curl_global_cleanup();
    
    return success;
}

/* Callback function for curl to store received data
 *
 * This function is called by curl when data is received from an HTTP request.
 * It dynamically resizes the memory buffer to store all incoming data.
 *
 * Parameters:
 *   contents - Pointer to the received data
 *   size - Size of each data element
 *   nmemb - Number of data elements
 *   userp - User-provided pointer (points to a MemoryStruct)
 *
 * Returns:
 *   Number of bytes processed (should be size*nmemb if successful)
 */
// cppcheck-suppress[constParameterCallback] - libcurl callback signature requires void*
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    /* Reallocate memory for the new data */
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("Not enough memory\n");
        return 0;
    }
    
    /* Store the data */
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

/* Validate the token's claims
 * 
 * This function checks that the access token contains the expected claims
 * and that those claims have valid values. For the Client Credentials flow,
 * important claims to check include:
 * - exp (Expiration Time): When the token expires
 * - scope: What permissions the token grants
 * - client_id: Which client the token was issued to
 * 
 * Parameters:
 *   token - The access token to validate
 *   error_message - Buffer to store validation error details
 *   error_size - Size of the error_message buffer
 * 
 * Returns:
 *   1 if validation passed, 0 if validation failed (with error_message populated)
 */
static int validate_token(const char *token, char *error_message, size_t error_size) {
    if (!token || token[0] == '\0') {
        snprintf(error_message, error_size, "Token is empty");
        return 0;
    }
    
    /* Parse the token payload */
    json_t *payload = parse_jwt_payload(token);
    if (!payload) {
        snprintf(error_message, error_size, "Failed to parse token");
        return 0;
    }
    
    int valid = 1;
    int now = current_time();
    
    /* Check required claims */
    json_t *exp_json = json_object_get(payload, "exp");
    if (!exp_json || !json_is_integer(exp_json)) {
        snprintf(error_message, error_size, "Token missing 'exp' claim");
        valid = 0;
    } else {
        int exp = (int)json_integer_value(exp_json);
        if (exp < now) {
            snprintf(error_message, error_size, "Token expired at %d, current time is %d", exp, now);
            valid = 0;
        }
    }
    
    /* Check scope if present */
    json_t *scope_json = json_object_get(payload, "scope");
    if (scope_json && json_is_string(scope_json)) {
        const char *scope = json_string_value(scope_json);
        if (strstr(scope, EXPECTED_SCOPE) == NULL) {
            snprintf(error_message, error_size, 
                     "Token scope '%s' does not include expected scope '%s'", 
                     scope, EXPECTED_SCOPE);
            valid = 0;
        }
    }
    
    /* Check client ID if present */
    json_t *client_id_json = json_object_get(payload, "client_id");
    if (client_id_json && json_is_string(client_id_json)) {
        const char *client_id = json_string_value(client_id_json);
        if (strcmp(client_id, CLIENT_ID) != 0) {
            snprintf(error_message, error_size, 
                     "Token client_id '%s' does not match expected client_id '%s'", 
                     client_id, CLIENT_ID);
            valid = 0;
        }
    }
    
    /* Note: This example does not verify the token signature.
     * In a production environment, you MUST verify the token signature.
     */
    if (valid) {
        printf("WARNING: This example does not verify the token signature.\n");
        printf("In a production environment, you MUST verify the token signature.\n");
    }
    
    json_decref(payload);
    return valid;
}

/* Parse JWT payload (simplified without signature verification)
 * 
 * JSON Web Tokens (JWTs) consist of three parts separated by dots:
 * 1. Header: Contains token type and algorithm information (e.g., {"alg":"RS256","typ":"JWT"})
 * 2. Payload: Contains the claims/assertions (e.g., permissions, client identity)
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
    
    /* Base64 handling */
    /* 1. Convert base64url to base64: Replace '-' with '+' and '_' with '/' */
    size_t len = strlen(payload);
    for (size_t i = 0; i < len; i++) {
        if (payload[i] == '-') payload[i] = '+';
        else if (payload[i] == '_') payload[i] = '/';
    }
    
    /* 2. Add padding if needed */
    size_t padding = 4 - (len % 4);
    if (padding < 4) {
        char *padded = malloc(len + padding + 1);
        if (!padded) {
            free(token_dup);
            return NULL;
        }
        
        strcpy(padded, payload);
        for (size_t i = 0; i < padding; i++) {
            padded[len + i] = '=';
        }
        padded[len + padding] = '\0';
        
        /* Decode using a temporary buffer */
        size_t decoded_len = (len * 3) / 4 + 10; /* Add some extra space */
        char *decoded = malloc(decoded_len);
        if (!decoded) {
            free(padded);
            free(token_dup);
            return NULL;
        }
        
        /* Use base64 command-line tool for decoding (simplified for this example) */
        FILE *fp;
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "echo '%s' | base64 -d", padded);
        
        fp = popen(cmd, "r");
        if (fp == NULL) {
            free(decoded);
            free(padded);
            free(token_dup);
            return NULL;
        }
        
        size_t bytes_read = fread(decoded, 1, decoded_len - 1, fp);
        pclose(fp);
        
        if (bytes_read == 0) {
            free(decoded);
            free(padded);
            free(token_dup);
            return NULL;
        }
        
        decoded[bytes_read] = '\0';
        
        /* Parse the JSON */
        json_error_t error;
        json_t *root = json_loads(decoded, 0, &error);
        
        free(decoded);
        free(padded);
        free(token_dup);
        
        return root;
    }
    
    /* Simplified fallback approach */
    FILE *fp;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "echo '%s' | base64 -d", payload);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        free(token_dup);
        return NULL;
    }
    
    char buffer[4096];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    pclose(fp);
    
    if (bytes_read == 0) {
        free(token_dup);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    
    /* Parse the JSON */
    json_error_t error;
    json_t *root = json_loads(buffer, 0, &error);
    
    free(token_dup);
    
    return root;
}

/* Find string value in JSON object */
/* Marked as possibly unused since it may be needed in future extensions */
static char *find_json_string_value(json_t *obj, const char *key) __attribute__((unused));
static char *find_json_string_value(json_t *obj, const char *key) {
    json_t *val = json_object_get(obj, key);
    if (!val || !json_is_string(val)) {
        return NULL;
    }
    
    return strdup(json_string_value(val));
}

/* Get current UNIX time */
static int current_time(void) {
    return (int)time(NULL);
}

/* Display token information */
static int display_token_info(const char *token) {
    if (!token || token[0] == '\0') {
        printf("Token not available\n");
        return 0;
    }
    
    json_t *payload = parse_jwt_payload(token);
    if (!payload) {
        printf("Failed to parse token\n");
        return 0;
    }
    
    printf("Token payload:\n");
    
    /* Display key claims */
    const char *key;
    json_t *value;
    
    json_object_foreach(payload, key, value) {
        if (json_is_string(value)) {
            printf("  %s: %s\n", key, json_string_value(value));
        } else if (json_is_integer(value)) {
            if (strcmp(key, "exp") == 0 || strcmp(key, "iat") == 0 || 
                strcmp(key, "nbf") == 0 || strcmp(key, "auth_time") == 0) {
                time_t timestamp = (time_t)json_integer_value(value);
                char timestr[64];
                const struct tm *tm_info = localtime(&timestamp);
                strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm_info);
                
                int now = current_time();
                int time_value = (int)json_integer_value(value);
                
                if (strcmp(key, "exp") == 0) {
                    int remaining = time_value - now;
                    printf("  %s: %lld (%s, %d seconds remaining)\n", 
                           key, json_integer_value(value), timestr, remaining);
                } else {
                    printf("  %s: %lld (%s)\n", key, json_integer_value(value), timestr);
                }
            } else {
                printf("  %s: %lld\n", key, json_integer_value(value));
            }
        } else if (json_is_true(value)) {
            printf("  %s: true\n", key);
        } else if (json_is_false(value)) {
            printf("  %s: false\n", key);
        } else if (json_is_null(value)) {
            printf("  %s: null\n", key);
        } else if (json_is_array(value)) {
            printf("  %s: [array]\n", key);
            
            size_t index;
            json_t *element;
            
            json_array_foreach(value, index, element) {
                if (json_is_string(element)) {
                    printf("    - %s\n", json_string_value(element));
                } else if (json_is_integer(element)) {
                    printf("    - %lld\n", json_integer_value(element));
                } else {
                    printf("    - [complex value]\n");
                }
            }
        } else if (json_is_object(value)) {
            printf("  %s: [object]\n", key);
        }
    }
    
    json_decref(payload);
    return 1;
}
