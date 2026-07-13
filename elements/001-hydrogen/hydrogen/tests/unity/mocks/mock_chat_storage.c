/*
 * Mock chat_storage functions implementation
 */

#include "mock_chat_storage.h"

// Static variable to hold mock result
static bool mock_store_media_result = true;

/*
 * Mock implementation of chat_storage_store_media
 */
bool mock_chat_storage_store_media(const char* database, const char* media_hash,
                                   const unsigned char* media_data, size_t media_size,
                                   const char* mime_type) {
    (void)database;      // Unused parameter
    (void)media_hash;    // Unused parameter
    (void)media_data;    // Unused parameter
    (void)media_size;    // Unused parameter
    (void)mime_type;     // Unused parameter
    
    return mock_store_media_result;
}

/*
 * Set the mock result for chat_storage_store_media
 */
void mock_chat_storage_set_store_media_result(bool result) {
    mock_store_media_result = result;
}

/*
 * Reset all mock state to defaults
 */
void mock_chat_storage_reset_all(void) {
    mock_store_media_result = true;
}