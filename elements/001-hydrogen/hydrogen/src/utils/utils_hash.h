/*
 * Hash Utilities
 *
 * Provides hash functions for generating statement hashes for prepared statement caching.
 * Uses the djb2 algorithm for consistent, fast hashing of SQL statements.
 */

#ifndef UTILS_HASH_H
#define UTILS_HASH_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Generate a hash for an SQL statement with optional prefix
 * @param prefix Optional prefix string (can be NULL for no prefix)
 * @param statement The SQL statement to hash
 * @param length Desired length of the hash portion (1-16 characters)
 * @param output_buf Buffer to store the formatted hash string (must be large enough)
 * @note The output format is: [prefix][hash] where hash is uppercase hex
 * @note Thread-safe as long as different output_buf are used
 */
void get_stmt_hash(const char* prefix, const char* statement, size_t length, char* output_buf);

#endif // UTILS_HASH_H