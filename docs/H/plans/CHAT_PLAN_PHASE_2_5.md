# Chat Service - Phase 2.5: Chat Status and Monitoring

## Objective
Augment conduit status endpoint with chat engine metrics and add Prometheus instrumentation for operational visibility.

## Prerequisites
- Phase 2 completed and verified (CEC operational, proxy functional)

## Testable Gate
Before proceeding to Phase 3, the following must be verified:
- Status endpoint returns model information with health status
- Prometheus metrics accessible at `/metrics` with chat-specific gauges/counters
- Health checks run at configured Liveliness interval
- All engines show correct status (healthy/degraded/unavailable)

## Tasks

### 1. Extend CEC with Health Tracking

Add to `ChatEngineConfig` in `chat_engine_cache.h`:
- `time_t last_health_check` - Timestamp of last health check
- `bool is_healthy` - Current health status
- `int consecutive_failures` - Count of consecutive failed health checks
- `time_t last_confirmed_working` - Timestamp of last successful request
- `double avg_response_time_ms` - Rolling average response time
- `unsigned long long conversations_24h` - Conversation count (updated via background query)
- `unsigned long long tokens_24h` - Token usage count

Add to `ChatEngineCache`:
- Health check thread management
- Liveliness interval (from config)

### 2. Add Liveliness Configuration

Extend `DatabaseConnection` in `src/config/config_databases.h`:
```c
int chat_liveliness_seconds;  // Health check interval (0 = disabled)
```

Default: `300` (5 minutes)
Valid range: 0-3600 (0 disables health checks)

Update `src/config/config_defaults.c` with default value.

Update `src/config/config_databases.c` to parse `Liveliness` from JSON:
```json
{
  "Connection": "Acuranzo",
  "Chat": true,
  "Liveliness": 300
}
```

### 3. Implement Health Check Function

Create `chat_health.c/chat_health.h`:

```c
// Perform health check on a single engine
bool chat_health_check_engine(ChatEngineConfig* engine);

// Background health check thread
void* chat_health_monitor_thread(void* arg);

// Start/stop health monitoring for a database
bool chat_health_monitor_start(const char* database_name, ChatEngineCache* cache, int interval_seconds);
void chat_health_monitor_stop(const char* database_name);

// Update engine stats after each request
void chat_health_update_stats(const char* engine_name, bool success, double response_time_ms, int tokens_used);
```

Health check implementation:
- Send lightweight request to engine's API (e.g., models list or minimal completion)
- Timeout: 10 seconds
- Track response time
- Update `last_confirmed_working` on success
- Increment `consecutive_failures` on failure
- Mark unhealthy after 3 consecutive failures
- Mark healthy after 1 successful check

### 4. Augment `/api/conduit/status` Endpoint

Modify existing status endpoint in `src/api/conduit/status/status.c`:

Add `models` array to response (when authenticated):
```json
{
  "success": true,
  "databases": {
    "acuranzo": {
      "ready": true,
      "models": [
        {
          "name": "gpt4",
          "provider": "openai",
          "model": "gpt-4-turbo",
          "status": "healthy",
          "last_confirmed_working": "2026-03-20T20:15:00Z",
          "conversations_24h": 1523,
          "tokens_24h": 4567890,
          "avg_response_time_ms": 842.3,
          "error_rate": 0.02
        }
      ]
    }
  }
}
```

Status values:
- `"healthy"` - Engine responding normally
- `"degraded"` - Responding but with errors or high latency
- `"unavailable"` - Not responding or too many failures

### 5. Prometheus Metrics Integration

Add to existing Prometheus metrics endpoint:

```c
// Gauges
hydrogen_chat_engine_health{database="acuranzo", engine="gpt4", provider="openai"} 1
hydrogen_chat_engine_response_time_ms{database="acuranzo", engine="gpt4"} 842.3

// Counters
hydrogen_chat_conversations_total{database="acuranzo", engine="gpt4"} 1523
hydrogen_chat_tokens_total{database="acuranzo", engine="gpt4", type="prompt"} 1234567
hydrogen_chat_tokens_total{database="acuranzo", engine="gpt4", type="completion"} 3333323
hydrogen_chat_errors_total{database="acuranzo", engine="gpt4", error_type="timeout"} 12

// Histogram
hydrogen_chat_request_duration_seconds_bucket{engine="gpt4", le="0.5"} 234
hydrogen_chat_request_duration_seconds_bucket{engine="gpt4", le="1.0"} 891
hydrogen_chat_request_duration_seconds_bucket{engine="gpt4", le="+Inf"} 1523
hydrogen_chat_request_duration_seconds_sum{engine="gpt4"} 1283.5
hydrogen_chat_request_duration_seconds_count{engine="gpt4"} 1523
```

Create `chat_metrics.c/chat_metrics.h`:
```c
void chat_metrics_engine_health(const char* database, const char* engine, const char* provider, bool healthy);
void chat_metrics_conversation(const char* database, const char* engine);
void chat_metrics_tokens(const char* database, const char* engine, int prompt_tokens, int completion_tokens);
void chat_metrics_request_duration(const char* engine, double duration_seconds);
void chat_metrics_error(const char* database, const char* engine, const char* error_type);
```

### 6. Background Statistics Query

Implement periodic query to update conversation/token counts:
- QueryRef for 24h statistics aggregation
- Update CEC entries with fresh data
- Run every 5 minutes (independent of liveliness setting)

### 7. Unit Tests

Create tests for:
- Health check logic (success/failure transitions)
- Configuration parsing (Liveliness values)
- Status endpoint JSON structure
- Metrics output format
- Thread safety of stats updates

## Verification Steps

1. Configure database with `"Liveliness": 60` and verify health checks run every minute
2. Configure database with `"Liveliness": 0` and verify no health check thread starts
3. Verify status endpoint includes models section when authenticated
4. Verify Prometheus metrics appear at `/metrics` endpoint
5. Simulate engine failure and verify status changes to "unavailable"
6. Verify consecutive failure threshold (3 failures) before marking unhealthy
7. Verify single success marks engine healthy again

## Dependencies

- Phase 2 completion (CEC operational)
- Access to existing `/api/conduit/status` endpoint
- Access to existing Prometheus metrics infrastructure

## Next Phase

After Phase 2.5 completion, proceed to Phase 3 (Public Endpoints) with full monitoring capabilities in place.
