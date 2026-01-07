/*
 * Resource Owner Password Flow Example for Hydrogen OIDC
 *
 * This example demonstrates how to implement the OAuth 2.0 Resource Owner Password
 * Credentials flow to authenticate a user and obtain access tokens from a
 * Hydrogen OIDC provider.
 *
 * Features demonstrated:
 * - Username and password credential handling
 * - Token request with client authentication
 * - Token response parsing with proper JSON library
 * - Error handling and reporting
 * - Making authenticated API requests
 * - Token validation and refreshing
 *
 * The flow involves:
 * 1. Application collects username and password from the user
 * 2. Application exchanges these credentials for access and refresh tokens
 * 3. Application validates the received tokens
 * 4. Application uses the access token to access protected resources
 * 5. Application refreshes the access token when it expires
 *
 * SECURITY NOTICE: This flow should only be used for first-party, highly-trusted
 * applications where the Authorization Code flow cannot be used. It requires the
 * application to handle the user's credentials directly, which is generally not
 * recommended for security reasons.
 *
 * Compile with:
 *   gcc -o password_flow password_flow.c -lcurl -ljansson
 *
 * Usage:
 *   ./password_flow <username> <password>
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <time.h>

/* Configuration - Replace with your actual values */
#define CLIENT_ID "your_client_id"
#define CLIENT_SECRET "your_client_secret"
#define TOKEN_ENDPOINT "https://hydrogen.example.com/oauth/token"
#define USERINFO_ENDPOINT "https://hydrogen.example.com/oauth/userinfo"
#define API_ENDPOINT "https://hydrogen.example.com/api/protected-resource"
#define REQUEST_TIMEOUT_SECONDS 30

/* Structure for curl responses */
struct MemoryStruct {
    char *memory;
    size_t size;
};

/* Function prototypes */
static int get_tokens(const char *username, const char *password, 
                    char *access_token, size_t access_token_size,
                    char *refresh_token, size_t refresh_token_size,
                    char *id_token, size_t id_token_size,
                    char *error_message, size_t error_size);
static int refresh_access_token(const char *refresh_token,
                              char *access_token, size_t access_token_size,
                              char *new_refresh_token, size_t refresh_token_size,
                              char *error_message, size_t error_size);
static int call_userinfo(const char *access_token, char *error_message, size_t error_size);
static int call_protected_api(const char *access_token, char *error_message, size_t error_size);
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static json_t *parse_jwt_payload(const char *jwt);
static int validate_token(const char *token, char *error_message, size_t error_size);
static int display_token_info(const char *token, const char *token_type);
static int current_time(void);
static void print_json_value(json_t *value, const char *prefix);

int main(int argc, const char *const argv[]) {
    printf("Resource Owner Password Flow Example\n");
    printf("====================================\n\n");

    /* Check command line arguments */
    if (argc != 3) {
        printf("Usage: %s <username> <password>\n", argv[0]);
        printf("Example: %s user@example.com mypassword\n", argv[0]);
        return 1;
    }

    const char *username = argv[1];
    const char *password = argv[2];
    
    /* Initialize token storage */
    char access_token[4096] = {0};
    char refresh_token[4096] = {0};
    char id_token[8192] = {0};
    char error_message[1024] = {0};
    
    printf("Requesting tokens for user: %s\n", username);
    
    /* Get tokens using password flow */
    if (get_tokens(username, password, 
                 access_token, sizeof(access_token),
                 refresh_token, sizeof(refresh_token),
                 id_token, sizeof(id_token),
                 error_message, sizeof(error_message))) {
                     
        printf("Tokens received successfully!\n\n");
        
        /* Display token information */
        if (access_token[0] != '\0') {
            printf("Access token information:\n");
            display_token_info(access_token, "Access");
        }
        
        if (id_token[0] != '\0') {
            printf("\nID token information:\n");
            display_token_info(id_token, "ID");
        }
        
        /* Validate the access token */
        printf("\nValidating access token...\n");
        if (validate_token(access_token, error_message, sizeof(error_message))) {
            printf("Token validation successful!\n");
            
            /* Use access token to get user information */
            printf("\nFetching user information...\n");
            if (call_userinfo(access_token, error_message, sizeof(error_message))) {
                /* Call protected API */
                printf("\nCalling protected API...\n");
                if (!call_protected_api(access_token, error_message, sizeof(error_message))) {
                    printf("API call failed: %s\n", error_message);
                }
                
                /* Demonstrate token refresh if we got a refresh token */
                if (refresh_token[0] != '\0') {
                    printf("\nSimulating token expiry and refresh...\n");
                    printf("Refreshing tokens...\n");
                    
                    char new_access_token[4096] = {0};
                    char new_refresh_token[4096] = {0};
                    
                    if (refresh_access_token(refresh_token,
                                          new_access_token, sizeof(new_access_token),
                                          new_refresh_token, sizeof(new_refresh_token),
                                          error_message, sizeof(error_message))) {
                        printf("Token refresh successful!\n");
                        
                        /* Display new token info */
                        printf("\nNew access token information:\n");
                        display_token_info(new_access_token, "Access");
                        
                        /* Use new access token */
                        printf("\nFetching user information with new token...\n");
                        if (!call_userinfo(new_access_token, error_message, sizeof(error_message))) {
                            printf("User info request with new token failed: %s\n", error_message);
                        }
                    } else {
                        printf("Token refresh failed: %s\n", error_message);
                    }
                } else {
                    printf("\nNo refresh token received, skipping token refresh demonstration.\n");
                }
            } else {
                printf("User info request failed: %s\n", error_message);
            }
        } else {
            printf("Token validation failed: %s\n", error_message);
        }
    } else {
        printf("Failed to get tokens: %s\n", error_message);
    }

    return 0;
}

/* Request tokens using the Resource Owner Password Flow */
static int get_tokens(const char *username, const char *password, 
                    char *access_token, size_t access_token_size,
                    char *refresh_token, size_t refresh_token_size,
                    char *id_token, size_t id_token_size,
                    char *error_message, size_t error_size) {
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
        char post_fields[4096];
        snprintf(post_fields, sizeof(post_fields),
            "grant_type=password"
            "&username=%s"
            "&password=%s"
            "&client_id=%s"
            "&client_secret=%s"
            "&scope=openid profile email",
            username, password, CLIENT_ID, CLIENT_SECRET);
        
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
        
        /* Parse JSON response */
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
        json_t *refresh_token_json = json_object_get(root, "refresh_token");
        json_t *id_token_json = json_object_get(root, "id_token");
        json_t *token_type_json = json_object_get(root, "token_type");
        json_t *expires_in_json = json_object_get(root, "expires_in");
        
        if (!access_token_json || !json_is_string(access_token_json)) {
            snprintf(error_message, error_size, "No access_token in response");
        } else {
            const char *token_value = json_string_value(access_token_json);
            if (strlen(token_value) >= access_token_size) {
                snprintf(error_message, error_size, "Access token too large for buffer");
            } else {
                strcpy(access_token, token_value);
                
                /* Get refresh token if present */
                if (refresh_token_json && json_is_string(refresh_token_json)) {
                    const char *refresh_value = json_string_value(refresh_token_json);
                    if (strlen(refresh_value) < refresh_token_size) {
                        strcpy(refresh_token, refresh_value);
                    }
                }
                
                /* Get ID token if present */
                if (id_token_json && json_is_string(id_token_json)) {
                    const char *id_value = json_string_value(id_token_json);
                    if (strlen(id_value) < id_token_size) {
                        strcpy(id_token, id_value);
                    }
                }
                
                /* Display token details */
                printf("Token details:\n");
                
                if (token_type_json && json_is_string(token_type_json)) {
                    printf("- Type: %s\n", json_string_value(token_type_json));
                }
                
                if (expires_in_json && json_is_integer(expires_in_json)) {
                    printf("- Expires in: %lld seconds\n", json_integer_value(expires_in_json));
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

/* Refresh the access token using a refresh token */
static int refresh_access_token(const char *refresh_token,
                               char *access_token, size_t access_token_size,
                               char *new_refresh_token, size_t refresh_token_size,
                               char *error_message, size_t error_size) {
    if (!refresh_token || refresh_token[0] == '\0') {
        snprintf(error_message, error_size, "No refresh token provided");
        return 0;
    }
    
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
        /* Prepare request body */
        char post_fields[4096];
        snprintf(post_fields, sizeof(post_fields),
            "grant_type=refresh_token"
            "&refresh_token=%s"
            "&client_id=%s"
            "&client_secret=%s",
            refresh_token, CLIENT_ID, CLIENT_SECRET);
        
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
        
        printf("Token refresh response received\n");
        
        /* Parse JSON response */
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
        json_t *refresh_token_json = json_object_get(root, "refresh_token");
        
        if (!access_token_json || !json_is_string(access_token_json)) {
            snprintf(error_message, error_size, "No access_token in response");
        } else {
            const char *token_value = json_string_value(access_token_json);
            if (strlen(token_value) >= access_token_size) {
                snprintf(error_message, error_size, "Access token too large for buffer");
            } else {
                strcpy(access_token, token_value);
                
                /* Get new refresh token if present */
                if (refresh_token_json && json_is_string(refresh_token_json)) {
                    const char *refresh_value = json_string_value(refresh_token_json);
                    if (strlen(refresh_value) < refresh_token_size) {
                        strcpy(new_refresh_token, refresh_value);
                    }
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

/* Call userinfo endpoint to get user information */
static int call_userinfo(const char *access_token, char *error_message, size_t error_size) {
    if (!access_token || access_token[0] == '\0') {
        snprintf(error_message, error_size, "No access token provided");
        return 0;
    }
    
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
        
        curl_easy_setopt(curl, CURLOPT_URL, USERINFO_ENDPOINT);
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
        
        printf("User info response received\n");
        
        /* Parse and display the user info */
        json_error_t json_error;
        json_t *root = json_loads(chunk.memory, 0, &json_error);
        
        if (!root) {
            snprintf(error_message, error_size, "JSON parsing error: %s", json_error.text);
        } else {
            printf("\nUser Profile Information:\n");
            print_json_value(root, "  ");
            json_decref(root);
            success = 1;
        }
        
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

/* Call protected API using the access token */
static int call_protected_api(const char *access_token, char *error_message, size_t error_size) {
    if (!access_token || access_token[0] == '\0') {
        snprintf(error_message, error_size, "No access token provided");
        return 0;
    }
    
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

/* Callback function for curl to store received data */
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

/* Parse JWT payload (simplified without signature verification) */
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

/* Validate the token's claims */
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

/* Get current UNIX time */
static int current_time(void) {
    return (int)time(NULL);
}

/* Display token information */
static int display_token_info(const char *token, const char *token_type) {
    if (!token || token[0] == '\0') {
        printf("No %s token available\n", token_type);
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
        int remaining = exp_time - now;

        printf("\n%s token expires in %d seconds\n", token_type, remaining);
    }

    json_decref(payload);
    return 1;
}

/* Print JSON values (for debugging and display) */
static void print_json_value(json_t *value, const char *prefix) {
    if (!value) {
        return;
    }

    if (json_is_object(value)) {
        const char *key;
        json_t *val;
        json_object_foreach(value, key, val) {
            if (json_is_string(val)) {
                printf("%s%s: %s\n", prefix, key, json_string_value(val));
            } else if (json_is_integer(val)) {
                if (strcmp(key, "exp") == 0 || strcmp(key, "iat") == 0 || 
                    strcmp(key, "nbf") == 0 || strcmp(key, "auth_time") == 0) {
                    time_t timestamp = (time_t)json_integer_value(val);
                    char timestr[64];
                    const struct tm *tm_info = localtime(&timestamp);
                    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm_info);
                    printf("%s%s: %lld (%s)\n", prefix, key, json_integer_value(val), timestr);
                } else {
                    printf("%s%s: %lld\n", prefix, key, json_integer_value(val));
                }
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
    }
}
