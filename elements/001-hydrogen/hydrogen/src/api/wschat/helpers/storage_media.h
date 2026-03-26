/*
 * Chat Storage Media Functions
 *
 * Provides media asset storage and retrieval functions.
 */

#ifndef STORAGE_MEDIA_H
#define STORAGE_MEDIA_H

#include <src/hydrogen.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Store media asset in media_assets table
 * @param database Database name
 * @param media_hash SHA-256 hash of media content
 * @param media_data Binary media data
 * @param media_size Size of media data in bytes
 * @param mime_type MIME type of media (can be NULL)
 * @return true on success, false on error
 */
bool chat_storage_store_media(const char* database, const char* media_hash,
                              const unsigned char* media_data, size_t media_size,
                              const char* mime_type);

/**
 * @brief Retrieve media asset from media_assets table
 * @param database Database name
 * @param media_hash SHA-256 hash of media content
 * @param media_data Output pointer to allocated media data (caller must free)
 * @param media_size Output size of media data
 * @param mime_type Output MIME type (caller must free, can be NULL if not stored)
 * @return true on success, false on error or not found
 */
bool chat_storage_retrieve_media(const char* database, const char* media_hash,
                                 unsigned char** media_data, size_t* media_size,
                                 char** mime_type);

/**
 * @brief Resolve media:hash references in a JSON content array
 * @param database Database name for media storage
 * @param content_json JSON array string of content parts
 * @param resolved_json Output pointer to newly allocated JSON array string (caller must free)
 * @param error_message Output error message on failure (caller must free)
 * @return true on success, false on error
 */
bool chat_storage_resolve_media_in_content(const char* database,
                                           const char* content_json,
                                           char** resolved_json,
                                           char** error_message);

#endif // STORAGE_MEDIA_H
