// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <brotli/decode.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>

// Project headers
#include "web_server_swagger.h"
#include "web_server_core.h"
#include "web_server_compression.h"
#include "../logging/logging.h"
#include "../config/configuration.h"

// Structure to hold in-memory Swagger files
typedef struct {
    char *name;         // File name (e.g., "index.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} SwaggerFile;

// Global state
static SwaggerFile *swagger_files = NULL;
static size_t num_swagger_files = 0;

// Forward declarations
static bool extract_swagger_payload(const char *executable_path, const AppConfig *config);
static bool load_swagger_files_from_tar(const uint8_t *tar_data, size_t tar_size);
static bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size, 
                          const char *private_key_b64, uint8_t **decrypted_data, 
                          size_t *decrypted_size);
static void free_swagger_files(void);
static char* get_server_url(struct MHD_Connection *connection, const WebConfig *config);
static char* create_dynamic_initializer(const char *base_content, const char *server_url, const WebConfig *config);

void cleanup_swagger_support(void);

// Initialize OpenSSL once at startup
static void init_openssl(void) {
    static bool initialized = false;
    if (!initialized) {
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
        initialized = true;
    }
}

bool init_swagger_support(WebConfig *config) {
    if (!config || !config->swagger.enabled) {
        return false;
    }

    // Initialize OpenSSL
    init_openssl();

    // Get executable path
    char *executable_path = get_executable_path();
    if (!executable_path) {
        log_this("WebServer", "Failed to get executable path", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Try to extract Swagger payload
    const AppConfig *app_config = get_app_config();
    bool success = extract_swagger_payload(executable_path, app_config);
    free(executable_path);

    // Update configuration based on payload availability
    config->swagger.payload_available = success;
    
    if (success) {
        log_this("WebServer", "Encrypted payload extracted and decrypted successfully", LOG_LEVEL_INFO, NULL);
        log_this("WebServer", "Swagger UI files available:", LOG_LEVEL_INFO, NULL);
        
        // Log each file's details
        for (size_t i = 0; i < num_swagger_files; i++) {
            char size_display[32];
            if (swagger_files[i].size < 1024) {
                snprintf(size_display, sizeof(size_display), "%zu bytes", swagger_files[i].size);
            } else if (swagger_files[i].size < 1024 * 1024) {
                snprintf(size_display, sizeof(size_display), "%.1fK", swagger_files[i].size / 1024.0);
            } else {
                snprintf(size_display, sizeof(size_display), "%.1fM", swagger_files[i].size / (1024.0 * 1024.0));
            }
            
            log_this("WebServer", "-> %s (%s%s)", LOG_LEVEL_INFO,
                    swagger_files[i].name, size_display, 
                    swagger_files[i].is_compressed ? ", compressed" : "");
        }
    } else {
        log_this("WebServer", "No valid encrypted payload found in executable or decryption failed", LOG_LEVEL_WARN, NULL);
    }

    return success;
}

bool is_swagger_request(const char *url, const WebConfig *config) {
    if (!config || !config->swagger.enabled || !config->swagger.payload_available || 
        !config->swagger.prefix || !url) {
        return false;
    }

    return strncmp(url, config->swagger.prefix, strlen(config->swagger.prefix)) == 0;
}

enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url,
                                     const WebConfig *config) {
    if (!connection || !url || !config) {
        return MHD_NO;
    }

    // First check if this is exactly the swagger prefix with no trailing slash
    // If so, redirect to the same URL with a trailing slash
    // This ensures all relative assets are correctly loaded
    size_t prefix_len = strlen(config->swagger.prefix);
    if (strcmp(url, config->swagger.prefix) == 0) {
        char *redirect_url;
        if (asprintf(&redirect_url, "%s/", url) != -1) {
            log_this("WebServer", "Redirecting %s to %s for proper relative path resolution", 
                    LOG_LEVEL_INFO, url, redirect_url);
                    
            struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Location", redirect_url);
            free(redirect_url);
            
            add_cors_headers(response);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // Skip past the prefix to get the actual file path
    const char *url_path = url + prefix_len;
    
    // Check if this is a request for the root path
    if (!*url_path || strcmp(url_path, "/") == 0) {
        url_path = "index.html";
    } else if (*url_path == '/') {
        url_path++; // Skip leading slash for other paths
    }
    
    // Log the URL processing for debugging
    log_this("WebServer", "Swagger request: Original URL: %s, Processed path: %s", 
             LOG_LEVEL_INFO, url, url_path);

    // Try to find the requested file
    SwaggerFile *file = NULL;
    bool client_accepts_br = client_accepts_brotli(connection);
    
    // First try to find an exact match (handles direct .br requests)
    for (size_t i = 0; i < num_swagger_files; i++) {
        if (strcmp(swagger_files[i].name, url_path) == 0) {
            file = &swagger_files[i];
            break;
        }
    }

    // If not found and client accepts brotli, try the .br version
    if (!file && client_accepts_br && !strstr(url_path, ".br")) {
        char br_path[256];
        snprintf(br_path, sizeof(br_path), "%s.br", url_path);
        
        for (size_t i = 0; i < num_swagger_files; i++) {
            if (strcmp(swagger_files[i].name, br_path) == 0) {
                file = &swagger_files[i];
                break;
            }
        }
    }

    // If still not found and the request was for a .br file,
    // try without the .br extension
    if (!file && strlen(url_path) > 3 && 
        strcmp(url_path + strlen(url_path) - 3, ".br") == 0) {
        char base_path[256];
        size_t path_len = strlen(url_path);
        if (path_len - 3 < sizeof(base_path)) {
            memcpy(base_path, url_path, path_len - 3);
            base_path[path_len - 3] = '\0';
            
            for (size_t i = 0; i < num_swagger_files; i++) {
                if (strcmp(swagger_files[i].name, base_path) == 0) {
                    file = &swagger_files[i];
                    break;
                }
            }
        }
    }

    if (!file) {
        return MHD_NO; // File not found
    }

    // Determine if we should serve compressed content
    bool serve_compressed = file->is_compressed && client_accepts_brotli(connection);

    // If this is swagger.json or swagger-initializer.js, we need to modify it
    struct MHD_Response *response;
    char *dynamic_content = NULL;
    
    if (strcmp(url_path, "swagger.json") == 0) {
        // Parse the original swagger.json
        json_error_t error;
        json_t *spec = json_loadb((const char*)file->data, file->size, 0, &error);
        if (!spec) {
            log_this("WebServer", "Failed to parse swagger.json: %s", LOG_LEVEL_ERROR, error.text);
            return MHD_NO;
        }

        // Get the info object or create it if it doesn't exist
        json_t *info = json_object_get(spec, "info");
        if (!info) {
            info = json_object();
            json_object_set_new(spec, "info", info);
        }

        // Update metadata from config
        if (config->swagger.metadata.title) {
            json_object_set_new(info, "title", json_string(config->swagger.metadata.title));
        }
        if (config->swagger.metadata.description) {
            json_object_set_new(info, "description", json_string(config->swagger.metadata.description));
        }
        if (config->swagger.metadata.version) {
            json_object_set_new(info, "version", json_string(config->swagger.metadata.version));
        }

        // Update contact info if provided
        if (config->swagger.metadata.contact.name || 
            config->swagger.metadata.contact.email || 
            config->swagger.metadata.contact.url) {
            json_t *contact = json_object();
            if (config->swagger.metadata.contact.name) {
                json_object_set_new(contact, "name", json_string(config->swagger.metadata.contact.name));
            }
            if (config->swagger.metadata.contact.email) {
                json_object_set_new(contact, "email", json_string(config->swagger.metadata.contact.email));
            }
            if (config->swagger.metadata.contact.url) {
                json_object_set_new(contact, "url", json_string(config->swagger.metadata.contact.url));
            }
            json_object_set_new(info, "contact", contact);
        }

        // Update license info if provided
        if (config->swagger.metadata.license.name || config->swagger.metadata.license.url) {
            json_t *license = json_object();
            if (config->swagger.metadata.license.name) {
                json_object_set_new(license, "name", json_string(config->swagger.metadata.license.name));
            }
            if (config->swagger.metadata.license.url) {
                json_object_set_new(license, "url", json_string(config->swagger.metadata.license.url));
            }
            json_object_set_new(info, "license", license);
        }

        // Convert the modified spec back to JSON
        dynamic_content = json_dumps(spec, JSON_INDENT(2));
        json_decref(spec);

        if (!dynamic_content) {
            log_this("WebServer", "Failed to serialize modified swagger.json", LOG_LEVEL_ERROR, NULL);
            return MHD_NO;
        }

        response = MHD_create_response_from_buffer(
            strlen(dynamic_content),
            dynamic_content,
            MHD_RESPMEM_MUST_FREE
        );

    } else if (strcmp(url_path, "swagger-initializer.js") == 0) {
        // Get the server's base URL
        char *server_url = get_server_url(connection, config);
        if (!server_url) {
            return MHD_NO;
        }
        
        // Create dynamic content with the correct URL
        dynamic_content = create_dynamic_initializer((const char*)file->data, server_url, config);
        free(server_url);
        
        if (!dynamic_content) {
            return MHD_NO;
        }
        
        response = MHD_create_response_from_buffer(
            strlen(dynamic_content),
            dynamic_content,
            MHD_RESPMEM_MUST_FREE
        );
    } else {
        // For all other files, serve directly from memory
        response = MHD_create_response_from_buffer(
            file->size,
            (void*)file->data,
            MHD_RESPMEM_PERSISTENT
        );
    }

    if (!response) {
        free(dynamic_content);
        return MHD_NO;
    }

    // Get the content type based on the file extension
    const char *content_type = NULL;
    const char *ext = strrchr(url_path, '.');
    if (ext) {
        // Skip .br extension if present
        if (strcmp(ext, ".br") == 0) {
            // Find the real extension before .br
            char clean_path[256];
            strncpy(clean_path, url_path, ext - url_path);
            clean_path[ext - url_path] = '\0';
            ext = strrchr(clean_path, '.');
        }
        
        if (ext) {
            if (strcmp(ext, ".html") == 0) {
                content_type = "text/html";
            } else if (strcmp(ext, ".css") == 0) {
                content_type = "text/css";
            } else if (strcmp(ext, ".js") == 0) {
                content_type = "application/javascript";
            } else if (strcmp(ext, ".json") == 0) {
                content_type = "application/json";
            } else if (strcmp(ext, ".png") == 0) {
                content_type = "image/png";
            }
        }
    }

    // If no extension found or unknown type, default to text/plain
    if (!content_type) {
        content_type = "text/plain";
    }

    // Add content type header
    MHD_add_response_header(response, "Content-Type", content_type);

    // Add compression header if serving compressed content
    if (serve_compressed) {
        add_brotli_header(response);
    }

    // Add CORS headers
    add_cors_headers(response);

    // Queue response
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

void cleanup_swagger_support(void) {
    free_swagger_files();
}

static bool extract_swagger_payload(const char *executable_path, const AppConfig *config) {
    // Open the executable
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        return false;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return false;
    }

    // Map the file
    void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (file_data == MAP_FAILED) {
        return false;
    }

    // Search for marker
    const char *marker = memmem(file_data, st.st_size,
                               SWAGGER_PAYLOAD_MARKER, 
                               strlen(SWAGGER_PAYLOAD_MARKER));

    if (!marker) {
        munmap(file_data, st.st_size);
        return false;
    }

    // Get encrypted payload size from the 8 bytes after marker
    const uint8_t *size_bytes = (uint8_t*)(marker + strlen(SWAGGER_PAYLOAD_MARKER));
    size_t payload_size = 0;
    for (int i = 0; i < 8; i++) {
        payload_size = (payload_size << 8) | size_bytes[i];
    }

    // The encrypted payload (payload.tar.br.enc) is before the marker
    const uint8_t *tar_data = (uint8_t*)marker - payload_size;
    
    // Verify we have enough space and the payload is within bounds
    if (tar_data < (uint8_t*)file_data || tar_data + payload_size > (uint8_t*)marker) {
        log_this("WebServer", "Invalid payload size (%zu bytes) or corrupted payload", 
                 LOG_LEVEL_ERROR, payload_size);
        munmap(file_data, st.st_size);
        return false;
    }

    log_this("WebServer", "Found encrypted payload: %zu bytes before marker", LOG_LEVEL_INFO, payload_size);
    
    // Check if configuration has a payload_key for decryption
    const char *payload_key = config ? config->payload_key : NULL;
    
    // Check if we have an RSA private key available
    if (!payload_key || strncmp(payload_key, "${env.", 6) == 0) {
        log_this("WebServer", "No valid PAYLOAD_KEY configuration, checking environment variable", LOG_LEVEL_WARN, NULL);
        
        // If it's an environment variable reference, extract the variable name
        if (payload_key && strncmp(payload_key, "${env.", 6) == 0) {
            char env_var[256];
            const char *end = strchr(payload_key + 6, '}');
            if (end) {
                size_t len = end - (payload_key + 6);
                strncpy(env_var, payload_key + 6, len);
                env_var[len] = '\0';
                
                // Get the environment variable value
                payload_key = getenv(env_var);
                log_this("WebServer", "Using PAYLOAD_KEY from environment variable %s", LOG_LEVEL_INFO, env_var);
            } else {
                payload_key = NULL;
            }
        }
    }
    
    if (!payload_key) {
        log_this("WebServer", "No valid PAYLOAD_KEY available, cannot decrypt payload", LOG_LEVEL_ERROR, NULL);
        munmap(file_data, st.st_size);
        return false;
    }
    
    // Log that we're attempting decryption (don't log the actual key)
    log_this("WebServer", "Attempting to decrypt payload using configured PAYLOAD_KEY", LOG_LEVEL_INFO, NULL);
    
    // Decrypt the payload (payload.tar.br.enc)
    uint8_t *decrypted_data = NULL;
    size_t decrypted_size = 0;
    bool decrypt_success = decrypt_payload(tar_data, payload_size, payload_key, &decrypted_data, &decrypted_size);
    
    if (!decrypt_success) {
        log_this("WebServer", "Failed to decrypt payload", LOG_LEVEL_ERROR, NULL);
        munmap(file_data, st.st_size);
        return false;
    }
    
    // The decrypted data should be a Brotli-compressed tar file
    char debug_bytes[64] = {0};
    for (size_t i = 0; i < (decrypted_size > 16 ? 16 : decrypted_size); i++) {
        sprintf(debug_bytes + (i * 3), "%02x ", decrypted_data[i]);
    }
    log_this("WebServer", "Processing decrypted payload (%zu bytes) as Brotli-compressed tar", 
             LOG_LEVEL_INFO, decrypted_size);
    log_this("WebServer", "Decrypted data first bytes: %s", LOG_LEVEL_INFO, debug_bytes);
    
    // Load and decompress the files from the decrypted tar data
    bool success = load_swagger_files_from_tar(decrypted_data, decrypted_size);
    
    // Free the decrypted data
    free(decrypted_data);
    
    munmap(file_data, st.st_size);
    return success;
}

/*
 * Decrypt an encrypted payload using RSA+AES hybrid encryption
 * 
 * The format of the encrypted data is:
 * [key_size(4 bytes)] + [encrypted_aes_key] + [encrypted_payload]
 * 
 * 1. Extract the size of the encrypted AES key
 * 2. Decrypt the AES key using the RSA private key (via EVP API)
 * 3. Convert the AES key to hex to match OpenSSL CLI behavior
 * 4. Use the hex AES key with PBKDF2 to derive key and IV
 * 5. Decrypt the payload with AES
 */
static bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size,
    const char *private_key_b64, uint8_t **decrypted_data, 
    size_t *decrypted_size) {
if (!encrypted_data || encrypted_size < 21 || !private_key_b64 || 
!decrypted_data || !decrypted_size) {
return false;
}

bool success = false;
BIO *bio = NULL;
BIO *b64 = NULL;
BIO *mem = NULL;
BIO *bio_chain = NULL;
EVP_PKEY *pkey = NULL;
EVP_PKEY_CTX *pkey_ctx = NULL;
uint8_t *aes_key = NULL;
uint8_t *private_key_data = NULL;
size_t private_key_len = 0;
EVP_CIPHER_CTX *ctx = NULL;
size_t aes_key_len = 0;
const uint8_t *encrypted_payload = NULL;
size_t encrypted_payload_size = 0;
uint8_t iv[16] = {0};
int len = 0;
int final_len = 0;

*decrypted_data = NULL;
*decrypted_size = 0;

uint32_t key_size = ((uint32_t)encrypted_data[0] << 24) |
 ((uint32_t)encrypted_data[1] << 16) |
 ((uint32_t)encrypted_data[2] << 8) |
 ((uint32_t)encrypted_data[3]);

char hex_str[16] = {0};
for (size_t i = 0; i < 4; i++) {
sprintf(hex_str + (i * 3), "%02x ", encrypted_data[i]);
}
log_this("WebServer", "Key size bytes: %s -> %u (0x%08x)", LOG_LEVEL_INFO, 
hex_str, key_size, key_size);

if (key_size == 0 || key_size > 1024 || key_size + 20 >= encrypted_size) {
log_this("WebServer", "Invalid payload structure - key_size: %u, total_size: %zu", 
LOG_LEVEL_ERROR, key_size, encrypted_size);
return false;
}

// Extract IV (16 bytes after encrypted key)
memcpy(iv, encrypted_data + 4 + key_size, 16);
char iv_hex[48] = {0};
for (size_t i = 0; i < 16; i++) {
sprintf(iv_hex + (i * 3), "%02x ", iv[i]);
}
log_this("WebServer", "Extracted IV (16 bytes): %s", LOG_LEVEL_INFO, iv_hex);

log_this("WebServer", "Payload structure:", LOG_LEVEL_INFO, NULL);
log_this("WebServer", "- Total size: %zu bytes", LOG_LEVEL_INFO, encrypted_size);
log_this("WebServer", "- Key size field: 4 bytes (%s)", LOG_LEVEL_INFO, hex_str);
log_this("WebServer", "- Encrypted AES key: %u bytes", LOG_LEVEL_INFO, key_size);
log_this("WebServer", "- IV: 16 bytes", LOG_LEVEL_INFO, NULL);
log_this("WebServer", "- Encrypted payload: %zu bytes", LOG_LEVEL_INFO, encrypted_size - 4 - key_size - 16);

char key_bytes[48] = {0};
for (size_t i = 0; i < 16 && i < key_size; i++) {
sprintf(key_bytes + (i * 3), "%02x ", encrypted_data[4 + i]);
}
log_this("WebServer", "Encrypted AES key starts with: %s", LOG_LEVEL_INFO, key_bytes);
log_this("WebServer", "Encrypted AES key size: %u bytes", LOG_LEVEL_INFO, key_size);

char key_prefix[6] = {0};
strncpy(key_prefix, private_key_b64, 5);
log_this("WebServer", "PAYLOAD_KEY first 5 chars: %s", LOG_LEVEL_INFO, key_prefix);

b64 = BIO_new(BIO_f_base64());
if (!b64) goto cleanup;
BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

mem = BIO_new_mem_buf(private_key_b64, -1);
if (!mem) goto cleanup;

bio_chain = BIO_push(b64, mem);

private_key_len = strlen(private_key_b64) * 3 / 4 + 1;
private_key_data = calloc(1, private_key_len);
if (!private_key_data) goto cleanup;

private_key_len = BIO_read(bio_chain, private_key_data, private_key_len);
if (private_key_len <= 0) goto cleanup;

bio = BIO_new_mem_buf(private_key_data, private_key_len);
if (!bio) goto cleanup;

pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
if (!pkey) {
log_this("WebServer", "Failed to load private key", LOG_LEVEL_ERROR, NULL);
goto cleanup;
}
log_this("WebServer", "Private key loaded successfully", LOG_LEVEL_INFO, NULL);

pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
if (!pkey_ctx) goto cleanup;

if (EVP_PKEY_decrypt_init(pkey_ctx) <= 0) goto cleanup;
if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PADDING) <= 0) goto cleanup;

if (EVP_PKEY_decrypt(pkey_ctx, NULL, &aes_key_len, encrypted_data + 4, key_size) <= 0) goto cleanup;

aes_key = calloc(1, aes_key_len);
if (!aes_key) goto cleanup;

if (EVP_PKEY_decrypt(pkey_ctx, aes_key, &aes_key_len, encrypted_data + 4, key_size) <= 0) {
log_this("WebServer", "Failed to decrypt AES key", LOG_LEVEL_ERROR, NULL);
goto cleanup;
}

if (aes_key_len != 32) {
log_this("WebServer", "Unexpected AES key length: %zu (expected 32)", LOG_LEVEL_ERROR, aes_key_len);
goto cleanup;
}

char aes_key_hex[16] = {0};
for (size_t i = 0; i < 5 && i < aes_key_len; i++) {
sprintf(aes_key_hex + (i * 2), "%02x", aes_key[i]);
}
log_this("WebServer", "AES key decrypted successfully (first 5 bytes: %s)", LOG_LEVEL_INFO, aes_key_hex);

encrypted_payload = encrypted_data + 4 + key_size + 16; // Skip IV
encrypted_payload_size = encrypted_size - 4 - key_size - 16;

char payload_hex_start[16] = {0}, payload_hex_end[16] = {0};
for (size_t i = 0; i < 5 && i < encrypted_payload_size; i++) {
sprintf(payload_hex_start + (i * 2), "%02x", encrypted_payload[i]);
}
for (size_t i = 0; i < 5 && i < encrypted_payload_size; i++) {
sprintf(payload_hex_end + (i * 2), "%02x", encrypted_payload[encrypted_payload_size - 5 + i]);
}
log_this("WebServer", "Encrypted payload size: %zu bytes", LOG_LEVEL_INFO, encrypted_payload_size);
log_this("WebServer", "Encrypted payload first 5 bytes: %s", LOG_LEVEL_INFO, payload_hex_start);
log_this("WebServer", "Encrypted payload last 5 bytes: %s", LOG_LEVEL_INFO, payload_hex_end);

ctx = EVP_CIPHER_CTX_new();
if (!ctx) goto cleanup;

char first_16_hex[64] = {0};
for (size_t i = 0; i < 16 && i < encrypted_payload_size; i++) {
sprintf(first_16_hex + (i * 3), "%02x ", encrypted_payload[i]);
}
log_this("WebServer", "First 16 bytes of encrypted payload: %s", LOG_LEVEL_INFO, first_16_hex);

log_this("WebServer", "AES decryption parameters:", LOG_LEVEL_INFO, NULL);
log_this("WebServer", "- Algorithm: AES-256-CBC", LOG_LEVEL_INFO, NULL);
log_this("WebServer", "- Key: Direct (32 bytes)", LOG_LEVEL_INFO, NULL);
log_this("WebServer", "- IV: Direct (16 bytes)", LOG_LEVEL_INFO, NULL);
log_this("WebServer", "- Padding: PKCS7", LOG_LEVEL_INFO, NULL);

if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, iv) ||
!EVP_CIPHER_CTX_set_padding(ctx, 1)) {
log_this("WebServer", "Failed to initialize AES decryption", LOG_LEVEL_ERROR, NULL);
goto cleanup;
}

*decrypted_data = calloc(1, encrypted_payload_size + EVP_MAX_BLOCK_LENGTH);
if (!*decrypted_data) goto cleanup;

if (!EVP_DecryptUpdate(ctx, *decrypted_data, &len, encrypted_payload, encrypted_payload_size)) {
log_this("WebServer", "Failed to decrypt payload", LOG_LEVEL_ERROR, NULL);
ERR_print_errors_fp(stderr);
free(*decrypted_data);
*decrypted_data = NULL;
goto cleanup;
}

*decrypted_size = len;

char decrypted_hex_start[16] = {0};
for (size_t i = 0; i < 5 && i < (size_t)len; i++) {
sprintf(decrypted_hex_start + (i * 2), "%02x", (*decrypted_data)[i]);
}
log_this("WebServer", "Decrypted data first 5 bytes: %s", LOG_LEVEL_INFO, decrypted_hex_start);

if (!EVP_DecryptFinal_ex(ctx, *decrypted_data + len, &final_len)) {
log_this("WebServer", "Failed to finalize decryption", LOG_LEVEL_ERROR, NULL);
ERR_print_errors_fp(stderr);
free(*decrypted_data);
*decrypted_data = NULL;
goto cleanup;
}

*decrypted_size += final_len;
log_this("WebServer", "Payload decrypted successfully (%zu bytes)", LOG_LEVEL_INFO, *decrypted_size);
success = true;

cleanup:
if (bio) BIO_free(bio);
if (b64) BIO_free(b64);
if (mem) BIO_free(mem);
if (pkey) EVP_PKEY_free(pkey);
if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
if (ctx) EVP_CIPHER_CTX_free(ctx);
if (aes_key) {
memset(aes_key, 0, aes_key_len);
free(aes_key);
}
if (private_key_data) {
memset(private_key_data, 0, private_key_len);
free(private_key_data);
}

return success;
}

static void free_swagger_files(void) {
    if (!swagger_files) {
        return;
    }

    for (size_t i = 0; i < num_swagger_files; i++) {
        free(swagger_files[i].name);
        free(swagger_files[i].data);
    }

    free(swagger_files);
    swagger_files = NULL;
    num_swagger_files = 0;
}

// TAR format constants
#define TAR_BLOCK_SIZE 512
#define TAR_NAME_SIZE 100
#define TAR_SIZE_OFFSET 124
#define TAR_SIZE_LENGTH 12

static bool load_swagger_files_from_tar(const uint8_t *tar_data, size_t tar_size) {
    // Try to decompress with Brotli using streaming API
    uint8_t *decompressed_data = NULL;
    size_t buffer_size = tar_size * 4;  // Initial estimate
    BrotliDecoderState* decoder = NULL;
    bool success = false;
    
    // Create decoder state with custom allocator and configure for high compression
    decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        log_this("WebServer", "Failed to create Brotli decoder instance", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Enable large window support for high compression
    if (!BrotliDecoderSetParameter(decoder, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1)) {
        log_this("WebServer", "Failed to set Brotli large window parameter", LOG_LEVEL_ERROR, NULL);
        BrotliDecoderDestroyInstance(decoder);
        return false;
    }
    
    // Allocate initial buffer with extra space for decompression
    buffer_size = tar_size * 32;  // Increased buffer size for high compression ratio
    decompressed_data = malloc(buffer_size);
    if (!decompressed_data) {
        log_this("WebServer", "Failed to allocate decompression buffer", LOG_LEVEL_ERROR, NULL);
        BrotliDecoderDestroyInstance(decoder);
        return false;
    }

    // Log buffer allocation
    log_this("WebServer", "Allocated decompression buffer: %zu bytes", LOG_LEVEL_INFO, buffer_size);
    
    // Set up streaming variables
    const uint8_t* next_in = tar_data;
    size_t avail_in = tar_size;
    uint8_t* next_out = decompressed_data;
    size_t avail_out = buffer_size;
    size_t total_out = 0;
    BrotliDecoderResult result;
    
    // Log detailed information about the input data
    char debug_bytes[128] = {0};
    char debug_bytes_end[128] = {0};
    for (size_t i = 0; i < (tar_size > 32 ? 32 : tar_size); i++) {
        sprintf(debug_bytes + (i * 3), "%02x ", tar_data[i]);
    }
    for (size_t i = 0; i < 32 && i < tar_size; i++) {
        sprintf(debug_bytes_end + (i * 3), "%02x ", tar_data[tar_size - 32 + i]);
    }
    log_this("WebServer", "Decompression input analysis:", LOG_LEVEL_INFO, NULL);
    log_this("WebServer", "- Input size: %zu bytes", LOG_LEVEL_INFO, tar_size);
    log_this("WebServer", "- First 32 bytes: %s", LOG_LEVEL_INFO, debug_bytes);
    log_this("WebServer", "- Last 32 bytes: %s", LOG_LEVEL_INFO, debug_bytes_end);
    log_this("WebServer", "- Decompression buffer: %zu bytes", LOG_LEVEL_INFO, buffer_size);

    // Check if this looks like a tar file already
    if (tar_size > 262 && memcmp(tar_data + 257, "ustar", 5) == 0) {
        log_this("WebServer", "Input appears to be uncompressed tar (ustar magic detected)", 
                 LOG_LEVEL_INFO, NULL);
        log_this("WebServer", "Skipping Brotli decompression", LOG_LEVEL_INFO, NULL);
        // Copy the tar data directly
        memcpy(decompressed_data, tar_data, tar_size);
        buffer_size = tar_size;
        success = true;
        goto cleanup;
    }

    // Log Brotli decompression parameters
    log_this("WebServer", "Starting Brotli decompression with:", LOG_LEVEL_INFO, NULL);
    log_this("WebServer", "- Large window: enabled", LOG_LEVEL_INFO, NULL);
    log_this("WebServer", "- Initial buffer: %zu bytes", LOG_LEVEL_INFO, buffer_size);
    
    // Decompress in chunks
    do {
        result = BrotliDecoderDecompressStream(decoder, &avail_in, &next_in,
                                              &avail_out, &next_out, &total_out);
        
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            // Need more output space
            size_t new_size = buffer_size * 2;
            uint8_t* new_buffer = realloc(decompressed_data, new_size);
            if (!new_buffer) {
                log_this("WebServer", "Failed to expand decompression buffer", LOG_LEVEL_ERROR, NULL);
                goto cleanup;
            }
            
            // Update pointers after realloc
            next_out = new_buffer + total_out;
            avail_out = new_size - total_out;
            decompressed_data = new_buffer;
            buffer_size = new_size;
        }
    } while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT ||
             result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT ||
             (result == BROTLI_DECODER_RESULT_SUCCESS && avail_in > 0));
    
    if (result != BROTLI_DECODER_RESULT_SUCCESS) {
        const char* error = BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder));
        log_this("WebServer", "Failed to decompress Swagger UI payload: %s", LOG_LEVEL_ERROR, error);
        goto cleanup;
    }
    
    // Set actual size
    buffer_size = total_out;
    success = true;
    
    // Log detailed hex dump of decompressed data
    char decompressed_bytes[128] = {0};
    char decompressed_bytes_end[128] = {0};
    for (size_t i = 0; i < (buffer_size > 32 ? 32 : buffer_size); i++) {
        sprintf(decompressed_bytes + (i * 3), "%02x ", decompressed_data[i]);
    }
    for (size_t i = 0; i < 32 && i < buffer_size; i++) {
        sprintf(decompressed_bytes_end + (i * 3), "%02x ", decompressed_data[buffer_size - 32 + i]);
    }
    log_this("WebServer", "Brotli decompression successful: %zu bytes -> %zu bytes",
             LOG_LEVEL_INFO, tar_size, buffer_size);
    log_this("WebServer", "Decompressed data first 32 bytes: %s", LOG_LEVEL_INFO, decompressed_bytes);
    log_this("WebServer", "Decompressed data last 32 bytes: %s", LOG_LEVEL_INFO, decompressed_bytes_end);
    
cleanup:
    BrotliDecoderDestroyInstance(decoder);
    if (!success) {
        free(decompressed_data);
        return false;
    }
    
    // Verify we have a valid tar file
    if (buffer_size <= 262 || memcmp(decompressed_data + 257, "ustar", 5) != 0) {
        log_this("WebServer", "Invalid tar format (missing ustar magic)", LOG_LEVEL_ERROR, NULL);
        free(decompressed_data);
        return false;
    }
    
    // Process the tar file
    const uint8_t *current = decompressed_data;
    const uint8_t *end = decompressed_data + buffer_size;
    size_t capacity = 16; // Initial capacity for swagger_files array
    
    swagger_files = malloc(capacity * sizeof(SwaggerFile));
    if (!swagger_files) {
        free(decompressed_data);
        return false;
    }
    
    num_swagger_files = 0;

    while (current + TAR_BLOCK_SIZE <= end) {
        // Check for end of tar (empty block)
        bool is_empty = true;
        for (size_t i = 0; i < TAR_BLOCK_SIZE; i++) {
            if (current[i] != 0) {
                is_empty = false;
                break;
            }
        }
        if (is_empty) break;

        // Get file name
        char name[TAR_NAME_SIZE + 1] = {0};
        memcpy(name, current, TAR_NAME_SIZE);
        
        // Get file size (octal string)
        char size_str[TAR_SIZE_LENGTH + 1] = {0};
        memcpy(size_str, current + TAR_SIZE_OFFSET, TAR_SIZE_LENGTH);
        size_t file_size = strtoul(size_str, NULL, 8);

        // Skip empty files or directories
        if (file_size == 0 || name[strlen(name) - 1] == '/') {
            current += TAR_BLOCK_SIZE;
            continue;
        }

        // Grow array if needed
        if (num_swagger_files == capacity) {
            capacity *= 2;
            SwaggerFile *new_files = realloc(swagger_files, capacity * sizeof(SwaggerFile));
            if (!new_files) {
                free_swagger_files();
                free(decompressed_data);
                return false;
            }
            swagger_files = new_files;
        }

        // Store file
        swagger_files[num_swagger_files].name = strdup(name);
        swagger_files[num_swagger_files].size = file_size;
        swagger_files[num_swagger_files].data = malloc(file_size);
        
        if (!swagger_files[num_swagger_files].name || 
            !swagger_files[num_swagger_files].data) {
            free_swagger_files();
            free(decompressed_data);
            return false;
        }

        // Copy file data
        current += TAR_BLOCK_SIZE;
        memcpy(swagger_files[num_swagger_files].data, current, file_size);
        
        // Check if this file is Brotli compressed (ends in .br)
        size_t name_len = strlen(name);
        swagger_files[num_swagger_files].is_compressed = 
            (name_len > 3 && strcmp(name + name_len - 3, ".br") == 0);


        num_swagger_files++;

        // Move to next file (aligned to block size)
        current += (file_size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE * TAR_BLOCK_SIZE;
    }

    free(decompressed_data);
    return num_swagger_files > 0;
}

/**
 * Get the server's base URL from the connection
 * 
 * Constructs the base URL (scheme://host[:port]) from the connection information
 * 
 * @param connection The MHD connection
 * @param config The web server configuration
 * @return Dynamically allocated string with the base URL, or NULL on error
 */
static char* get_server_url(struct MHD_Connection *connection, 
                          const WebConfig *config __attribute__((unused))) {
    // Host header is mandatory in HTTP/1.1
    const char *host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
    if (!host) {
        log_this("WebServer", "No Host header in Swagger UI request", LOG_LEVEL_ERROR, NULL);
        return NULL;
    }

    // Determine protocol (https or http) from X-Forwarded-Proto
    const char *forwarded_proto = MHD_lookup_connection_value(connection, 
                                                            MHD_HEADER_KIND, 
                                                            "X-Forwarded-Proto");
    const char *scheme = (forwarded_proto && strcmp(forwarded_proto, "https") == 0) 
                        ? "https" : "http";

    // Construct the base URL from the request information only
    char *url = NULL;
    if (asprintf(&url, "%s://%s", scheme, host) == -1) {
        return NULL;
    }
    return url;
}

/**
 * Create a dynamic swagger-initializer.js with the correct server URL
 * 
 * @param base_content The original initializer content (unused as we generate content from scratch)
 * @param server_url The server's base URL from the incoming request
 * @param config The web server configuration containing the swagger prefix
 * @return Dynamically allocated string with the modified content, or NULL on error
 */
static char* create_dynamic_initializer(const char *base_content __attribute__((unused)), 
                                      const char *server_url,
                                      const WebConfig *config) {
    // Create the new initializer content with server URL update
    char *dynamic_content = NULL;
    if (asprintf(&dynamic_content,
        "window.onload = function() {\n"
        "  fetch('%s%s/swagger.json').then(response => response.json()).then(spec => {\n"
        "    // Update server URL to match current host\n"
        "    // Using configured API prefix instead of hardcoded value\n"
        "    spec.servers = [{url: '%s%s', description: 'Current server'}];\n"
        "    window.ui = SwaggerUIBundle({\n"
        "      spec: spec,\n"
        "      dom_id: '#swagger-ui',\n"
        "      deepLinking: true,\n"
        "      presets: [\n"
        "        SwaggerUIBundle.presets.apis,\n"
        "        SwaggerUIStandalonePreset\n"
        "      ],\n"
        "      plugins: [\n"
        "        SwaggerUIBundle.plugins.DownloadUrl\n"
        "      ],\n"
        "      layout: \"StandaloneLayout\",\n"
        "      tryItOutEnabled: %s,\n"
        "      displayOperationId: %s,\n"
        "      defaultModelsExpandDepth: %d,\n"
        "      defaultModelExpandDepth: %d,\n"
        "      showExtensions: %s,\n"
        "      showCommonExtensions: %s,\n"
        "      docExpansion: \"%s\",\n"
        "      syntaxHighlight: {\n"
        "        theme: \"%s\"\n"
        "      }\n"
        "    });\n"
        "  });\n"
        "};", 
        server_url, config->swagger.prefix, 
        server_url, config->api_prefix,
        config->swagger.ui_options.try_it_enabled ? "true" : "false",
        config->swagger.ui_options.display_operation_id ? "true" : "false",
        config->swagger.ui_options.default_models_expand_depth,
        config->swagger.ui_options.default_model_expand_depth,
        config->swagger.ui_options.show_extensions ? "true" : "false",
        config->swagger.ui_options.show_common_extensions ? "true" : "false",
        config->swagger.ui_options.doc_expansion,
        config->swagger.ui_options.syntax_highlight_theme) == -1) {
        return NULL;
    }

    return dynamic_content;
}