# Chat Streaming Implementation - curl_multi Guide

## Overview

The chat streaming system uses libcurl's multi interface (`curl_multi`) to provide non-blocking, event-driven streaming of AI model responses to WebSocket clients. This document describes the implementation, the challenges encountered during development, and guidance for future modifications.

## Architecture

```
WebSocket Client
       ↑↓
   LWS Event Loop
       ↑↓
  Queue (chunk_queue)
       ↑↓
  Worker Thread (multi_worker_thread)
       ↑↓
  curl_multi handle → AI Model API (SSE streaming)
```

### Components

1. **proxy_multi.c/h** - Main multi-curl streaming implementation
2. **proxy.c** - Legacy single-curl implementation (kept for reference, non-streaming requests)
3. **req_builder.c** - Request body construction for different providers (OpenAI, Anthropic, Ollama)
4. **resp_parser.c** - SSE response chunk parsing

### Data Flow

1. WebSocket client sends chat request with `stream: true`
2. Request is validated and request body is built via `chat_request_build()`
3. `chat_proxy_multi_stream_start()` creates a multi stream context and adds an easy handle to the curl multi handle
4. Worker thread drives the multi handle via `curl_multi_perform()` + `curl_multi_poll()`
5. When SSE data arrives, `multi_stream_write_callback()` processes it and enqueues chunks
6. `chat_proxy_multi_request_writable()` triggers LWS writable callback
7. `handle_chat_writable()` drains the queue and writes chunks to WebSocket

## Critical Implementation Details

### HTTP/2 Requires curl_multi_poll(), NOT curl_multi_wait()

**This is the most important discovery during development.**

When libcurl negotiates HTTP/2 with a server, `curl_multi_wait()` does NOT work correctly. It returns `num_fds=0` because HTTP/2 uses internal framing mechanisms that don't expose all necessary socket file descriptors. This causes the wait to timeout repeatedly instead of blocking until data arrives.

**Solution**: Use `curl_multi_poll()` instead. This function is specifically designed to handle HTTP/2 and other protocols where socket file descriptors may not be directly exposed.

```c
// WRONG - doesn't work with HTTP/2
curl_multi_wait(manager->multi_handle, NULL, 0, 100, &num_fds);

// CORRECT - works with HTTP/2
curl_multi_poll(manager->multi_handle, NULL, 0, 10, NULL);
```

**Reference**: [libcurl multi interface documentation](https://curl.se/libcurl/curl_multi_poll.html)

### curl_multi_perform() Must Be Called After curl_multi_poll()

The libcurl multi interface workflow requires `curl_multi_perform()` to be called both BEFORE and AFTER the wait function:

```c
// Correct workflow:
curl_multi_perform(multi_handle, &still_running);  // Initiate/progress transfers
curl_multi_poll(multi_handle, NULL, 0, timeout, NULL);  // Wait for socket activity
curl_multi_perform(multi_handle, &still_running);  // Process data that arrived
// Then check for completed transfers via curl_multi_info_read()
```

If you only call `curl_multi_perform()` once per loop iteration, data may sit in socket buffers without being read, causing latency.

### Thread Safety Model

- **Worker thread**: Drives curl multi handle, receives data via write callback
- **LWS thread**: Drains chunk queue and writes to WebSocket

Communication between threads uses a mutex-protected queue (`StreamChunkQueue`). The write callback enqueues chunks and calls `lws_callback_on_writable()` to notify the LWS thread.

### Logging Verbosity

The implementation has multiple logging levels to balance visibility with performance:

| Level | Content |
|-------|---------|
| STATE | Key events: stream start, TTFB, completion |
| DEBUG | Connection info, request size, HTTP status |
| TRACE | TLS details, progress (every 50 chunks), headers |

RAW chunk data is logged in batches (every 50 chunks) at TRACE level to avoid log spam.

## Known Issues and Solutions

### Issue: TTFB (Time To First Byte) is very high

**Symptoms**: `Multi-stream TTFB: 50000ms` or similar high values

**Possible causes**:
1. Wrong API URL configuration (missing path like `/api/v1/chat/completions`)
2. Model is actually slow (test with the same prompt in DO playground)
3. Request format issues (verify `"stream": true` is in the request body)

**Debug steps**:
1. Check the API URL in logs - must include full path
2. Check request body at TRACE level - verify `"stream": true` present
3. Compare TTFB with direct curl request to the same endpoint

### Issue: No response data received

**Symptoms**: Transfer completes but no chunks are received

**Possible causes**:
1. Missing `Accept: text/event-stream` header
2. Model doesn't support streaming
3. Request body missing `"stream": true`

### Issue: High CPU usage

**Symptoms**: Worker thread uses significant CPU even when idle

**Cause**: `curl_multi_poll()` timeout is too low

**Solution**: Use 10ms timeout - balances responsiveness with CPU usage:
```c
curl_multi_poll(manager->multi_handle, NULL, 0, 10, NULL);
```

### Issue: Memory growth during long streams

**Symptoms**: Memory usage increases continuously during streaming

**Possible causes**:
1. Queue not being drained (check `handle_chat_writable()` is called)
2. Chunks not being freed after write

## Configuration

### Engine Configuration

Each AI engine must have a complete API URL:

```
# Correct - includes full path
https://w667rc4eyntvzuzyxxog3keh.agents.do-ai.run/api/v1/chat/completions

# Wrong - missing path
https://w667rc4eyntvzuzyxxog3keh.agents.do-ai.run
```

### curl_multi Options

```c
// Connection limits
curl_multi_setopt(manager->multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 50);
curl_multi_setopt(manager->multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, 200);

// Per-request options
curl_easy_setopt(easy_handle, CURLOPT_CONNECTTIMEOUT, 10L);  // 10s connect timeout
curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 600L);        // 10min total timeout
curl_easy_setopt(easy_handle, CURLOPT_NOSIGNAL, 1L);         // Required for threads
```

## Future Development

### Adding a New Provider

1. Add provider constant to `ChatEngineConfig` (e.g., `CEC_PROVIDER_GOOGLE`)
2. Add header construction in `proxy_multi.c` `chat_proxy_multi_stream_start()`
3. Add request builder in `req_builder.c` if format differs from OpenAI/Anthropic
4. Test with streaming enabled

### Modifying the Worker Thread

The worker thread loop is in `multi_worker_thread()`. Key considerations:

1. Always call `curl_multi_perform()` after `curl_multi_poll()` returns
2. Check for completed transfers with `curl_multi_info_read()`
3. Don't block the thread - use short timeouts
4. Log at appropriate levels to avoid spam

### Performance Tuning

Current settings are optimized for reasonable latency without excessive CPU:

- `curl_multi_poll` timeout: 10ms
- Chunk logging: Every 50 chunks at TRACE level
- Queue capacity: 1024 chunks max (backpressure)

For lower latency, reduce the poll timeout (at cost of CPU).
For higher throughput, increase queue capacity.

## References

- [libcurl multi interface](https://curl.se/libcurl/curl_multi.html)
- [curl_multi_poll documentation](https://curl.se/libcurl/curl_multi_poll.html)
- [curl_multi_wait vs curl_multi_poll](https://curl.se/libcurl/curl_multi_wait.html)
- [SSE (Server-Sent Events)](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events)
- [OpenAI Streaming API](https://platform.openai.com/docs/api-reference/streaming)
