/*
 * Mock chat_storage functions for unit testing
 *
 * This file provides mock implementations of chat_storage_* functions
 * to enable testing of media upload functionality without real database connections.
 */

#ifndef MOCK_CHAT_STORAGE_H
#define MOCK_CHAT_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

// Mock function declarations - these will override the real ones when USE_MOCK_CHAT_STORAGE is defined
#ifdef USE_MOCK_CHAT_STORAGE

// Override chat_storage functions with our mocks
#define chat_storage_store_media mock_chat_storage_store_media

// Always declare mock function prototypes for the .c file
bool mock_chat_storage_store_media(const char* database, const char* media_hash,
                                   const unsigned char* media_data, size_t media_size,
                                   const char* mime_type);

#endif // USE_MOCK_CHAT_STORAGE

// Mock control functions for tests - always available
void mock_chat_storage_set_store_media_result(bool result);
void mock_chat_storage_reset_all(void);

#endif // MOCK_CHAT_STORAGE_H
