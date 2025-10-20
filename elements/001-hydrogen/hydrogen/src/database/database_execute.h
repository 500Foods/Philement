/*
 * Database Query Execution Header
 *
 * Function declarations for query submission, status checking,
 * result retrieval, and query lifecycle management.
 */

#ifndef DATABASE_EXECUTE_H
#define DATABASE_EXECUTE_H

#include <time.h>

// Query Processing API (Phase 2 integration points)
bool database_submit_query(const char* database_name, const char* query_id,
                          const char* query_template, const char* parameters_json,
                          int queue_type_hint);

DatabaseQueryStatus database_query_status(const char* query_id);

bool database_get_result(const char* query_id, const char* result_buffer, size_t buffer_size);

bool database_cancel_query(const char* query_id);

// Query lifecycle management
time_t database_get_query_age(const char* query_id);

void database_cleanup_old_results(time_t max_age_seconds);

// Queue statistics
int database_get_total_queue_count(void);

void database_get_queue_counts_by_type(int* lead_count, int* slow_count, int* medium_count,
                                      int* fast_count, int* cache_count);

// Helper functions for testing (exposed for unit test coverage)
time_t calculate_queue_query_age(DatabaseQueue* db_queue);
time_t find_max_query_age_across_queues(void);

#endif // DATABASE_EXECUTE_H