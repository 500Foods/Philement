// Global includes
#include "../hydrogen.h"

// Local includes
#include "utils_hash.h"

// Public interface declarations
void get_stmt_hash(const char* prefix, const char* statement, size_t length, char* output_buf);

// Private function declarations
static uint64_t djb2_hash_64(const char* str);

// djb2 hash algorithm implementation (64-bit version)
static uint64_t djb2_hash_64(const char* str) {
    if (str == NULL) {
        return 0;
    }

    uint64_t hash = 5381ULL;
    unsigned char c;

    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }

    return hash;
}

// Generate a hash for an SQL statement with optional prefix formatting
void get_stmt_hash(const char* prefix, const char* statement, size_t length, char* output_buf) {
    if (output_buf == NULL || length == 0) {
        return;  // Safety
    }

    // Start with prefix if provided
    strcpy(output_buf, prefix ? prefix : "");

    // Compute hash
    uint64_t h = djb2_hash_64(statement);

    // Format as uppercase hex, padded to length (e.g., %016llX for 16 chars)
    char hex_buf[32];  // Temp for full 16 max
    sprintf(hex_buf, "%016llX", (unsigned long long)h);  // Full 16 uppercase hex, padded left with 0s

    // Truncate/pad to exact length (if <16, pad; if >, truncate—but cap at 16)
    size_t hash_len = (length > 16) ? 16 : length;
    size_t current_len = strlen(output_buf);
    memcpy(output_buf + current_len, hex_buf, hash_len);
    output_buf[current_len + hash_len] = '\0';

    // Pad with '0' if too short (unlikely, but for safety)
    size_t prefix_len = prefix ? strlen(prefix) : 0;
    size_t total_len = prefix_len + hash_len;  // prefix + hash
    while (strlen(output_buf) < total_len) {
        size_t pad_len = strlen(output_buf);
        output_buf[pad_len] = '0';
        output_buf[pad_len + 1] = '\0';
    }
}