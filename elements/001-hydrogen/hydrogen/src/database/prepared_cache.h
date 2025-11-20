#ifndef PREPARED_CACHE_H
#define PREPARED_CACHE_H

#include <src/database/database.h>

typedef struct PreparedStatementCache PreparedStatementCache;

PreparedStatementCache* prepared_cache_create(size_t max_size, const char* engine_name);

void prepared_cache_destroy(PreparedStatementCache* cache);

PreparedStatement* prepared_cache_find_by_hash(PreparedStatementCache* cache, const char* hash);

bool prepared_cache_add(PreparedStatementCache* cache, PreparedStatement* stmt);

void prepared_cache_update_lru(PreparedStatementCache* cache, const char* hash);

bool prepared_cache_evict_lru(PreparedStatementCache* cache);

void prepared_cache_log_stats(PreparedStatementCache* cache, const char* label);

#endif