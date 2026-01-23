#include "mock_prepare_and_submit_query.h"

// Mock state
static bool mock_result = true;

// Mock implementation
bool mock_prepare_and_submit_query(const struct DatabaseQueue* selected_queue, const char* query_id,
                                  const char* sql_template, struct TypedParameter** ordered_params,
                                  size_t param_count, const struct QueryCacheEntry* cache_entry) {
    // Validate required parameters (same as real implementation)
    if (!selected_queue || !query_id || !sql_template || !cache_entry) {
        return false;
    }

    // Validate parameter count to prevent excessive memory usage
    if (param_count > 100) {
        return false;
    }

    (void)ordered_params; // Not validated in mock
    return mock_result;
}

// Mock control functions
void mock_prepare_and_submit_query_set_result(bool result) {
    mock_result = result;
}

void mock_prepare_and_submit_query_reset(void) {
    mock_result = true;
}