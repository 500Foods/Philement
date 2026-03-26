# Chat Service - Phase 13: Advanced Features

## Objective

Implement advanced features including function calling, response caching, load balancing, and analytics dashboard.

## Prerequisites

- Phase 12 completed and verified (advanced multi-modal features working)

## Testable Gate

Before considering the implementation complete, the following must be verified:

- Function calling support works with OpenAI-compatible providers
- Response caching reduces costs for frequent queries
- Load balancing distributes across multiple API keys effectively
- Fallback engines provide graceful degradation
- Usage analytics dashboard provides meaningful insights
- Unit and integration tests pass for all advanced features
- No regression in existing functionality

## Tasks

### 1. Implement Function Calling

- Add support for OpenAI function calling format
- Extend chat_request_builder to handle function definitions
- Implement function result parsing in chat_response_parser
- Support iterative function calling (multiple rounds)
- Provide clear error handling for function execution failures

### 2. Implement Response Caching

- Cache frequent queries to reduce API costs
- Hash-based cache key (request parameters + context)
- Configurable TTL and cache size limits
- Cache invalidation strategies
- Performance measurement of cache effectiveness

### 3. Implement Load Balancing

- Distribute requests across multiple API keys for same provider
- Round-robin, weighted, or least-used selection algorithms
- Automatic failover when keys hit rate limits or errors
- Key health monitoring and rotation
- Configuration per engine for multiple keys

### 4. Implement Fallback Engines

- Configure backup engines per database
- Automatic failover on primary engine failure
- Fallback chain (primary -> secondary -> tertiary)
- Failback when primary recovers (optional)
- Clear logging of failover/failback events

### 5. Implement Usage Analytics Dashboard

- Track requests, tokens, costs per user/database/engine
- Real-time and historical views
- Cost attribution and forecasting
- Performance metrics (latency, error rates)
- Export capabilities for billing and analysis

### 6. Implement Prompt Templates

- Pre-defined prompt templates per engine/use-case
- Template variable substitution
- Template management (create, update, delete)
- Usage statistics for templates
- Sharing capabilities between users/databases

### 7. Implement Conversation Management

- API endpoints for listing, retrieving, deleting conversations
- Conversation search and filtering
- Archive and restore functionality
- Bulk operations
- Retention policies and automatic cleanup

### 8. Implement Real-time Cost Tracking

- Accurate per-request cost calculation based on provider pricing
- Currency conversion and formatting
- Budget alerts and notifications
- Cost breakdown by prompt/completion tokens
- Historical cost analysis and trends

### 9. Implement A/B Testing Framework

- Compare responses from different engines/configurations
- Traffic splitting capabilities
- Statistical significance calculation
- Experiment management and tracking
- Result analysis and reporting

### 10. Unit and Integration Tests

- Test function calling with various function definitions
- Verify response caching reduces external API calls
- Test load balancing distribution and failover
- Verify fallback engine behavior
- Test analytics data collection and reporting
- Ensure all advanced features work together correctly

### 11. Streaming Architecture (Completed 2026-03-25)

The WebSocket streaming implementation was updated to address blocking issues and improve robustness:

#### Problem Identified
- `curl_easy_perform()` was blocking the LWS event loop
- Server became unresponsive after multiple concurrent streaming requests
- TTFB was slow compared to provider playgrounds

#### Solution Implemented
- **Detached thread streaming**: Each streaming request spawns a detached worker thread
- **Non-blocking LWS loop**: Function returns immediately, LWS callback completes
- **CURLOPT_NOSIGNAL**: Thread-safe CURL configuration
- **SSE headers**: Added `Accept: text/event-stream` header required by SSE endpoints
- **Request body fix**: Proper request construction via `chat_request_build()` instead of raw payload
- **Stream tracking**: Active streams tracked for monitoring and cleanup

#### Known Limitation: Thread Safety
The worker thread writes chunks directly to the WebSocket via `lws_write()`. This is technically not thread-safe per LWS documentation (writes should occur in the service thread). Works in practice for moderate loads.

**Future improvement**: Implement queue-based streaming:
1. Thread-safe chunk queue
2. `lws_callback_on_writable()` to trigger writes
3. Write from LWS service thread in writable callback

Or migrate to libcurl multi interface integrated with LWS event loop.

#### Connection Diagnostics
Added CURL debug callback for troubleshooting connection issues:
- Logs TCP connection establishment
- Logs TLS/SSL handshake events
- Logs HTTP protocol negotiation
- Logs outgoing request headers

#### Architecture Decision: Threads vs Multi Interface

**Current approach (Threads):**
- One thread per streaming request
- Simple, robust implementation
- Works well for moderate concurrency (tens of simultaneous streams)
- Appropriate for current use case where multi-host requests are uncommon

**Future consideration (libcurl Multi Interface):**
- Event-driven, single-threaded approach
- Scales to thousands of concurrent streams
- Integrates with LWS event loop via `curl_multi_socket_action()`
- Recommended when concurrency exceeds thread capacity
- Implementation reference: `multi-uv.c` or `multi-event.c` examples

The multi interface would provide better scalability but adds complexity. The threaded approach is sufficient for current needs and provides better isolation between requests.

## Verification Steps

1. Verify function calling works with sample functions
2. Test response caching shows reduced external API calls
3. Confirm load balancing distributes requests appropriately
4. Test fallback engine activates on primary failure
5. Verify analytics dashboard shows accurate metrics
6. Test conversation management API endpoints
7. Confirm real-time cost tracking accuracy
8. Run unit and integration tests for all advanced features
9. Ensure existing core chat functionality unaffected
10. Test advanced features work with streaming when applicable

## Completion Criteria

All 14 phases completed with their respective testable gates passed.
The chat service provides:

- Secure, scalable API proxy to multiple AI providers
- Efficient storage with content-addressable segments and Brotli compression
- Performance optimizations including caching and context hashing
- Resilience through fallback mechanisms and cross-server recovery
- Advanced features for enterprise use cases
- Comprehensive monitoring and analytics
