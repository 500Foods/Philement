#ifndef MOCK_PREPARE_AND_SUBMIT_QUERY_H
#define MOCK_PREPARE_AND_SUBMIT_QUERY_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
struct DatabaseQueue;
struct TypedParameter;
struct QueryCacheEntry;

// Enable mock override
#ifdef USE_MOCK_PREPARE_AND_SUBMIT_QUERY
#define prepare_and_submit_query mock_prepare_and_submit_query
#endif

// Mock function prototype
bool mock_prepare_and_submit_query(const struct DatabaseQueue* selected_queue, const char* query_id,
                                  const char* sql_template, struct TypedParameter** ordered_params,
                                  size_t param_count, const struct QueryCacheEntry* cache_entry);

// Mock control functions
void mock_prepare_and_submit_query_set_result(bool result);
void mock_prepare_and_submit_query_reset(void);

#endif // MOCK_PREPARE_AND_SUBMIT_QUERY_H