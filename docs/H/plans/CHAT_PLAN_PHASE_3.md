# Chat Service - Phase 3: Public Endpoints (chat, chats) with Image Support

## Objective

Implement public chat endpoints with multimodal support from day one.

## Prerequisites

- Phase 2 completed and verified (CEC, proxy, builder, parser working)

## Testable Gate

Before proceeding to Phase 4, the following must be verified:

- POST /api/conduit/chat endpoint is registered and accessible
- Single chat requests work correctly with OpenAI-compatible providers
- Batch chat endpoint (/api/conduit/chats) works with parallel execution
- Image/multimodal support functions correctly (content as string OR array)
- Default engine fallback works when engine parameter is omitted
- Integration with proxy, builder, and parser is functional
- Unit and integration tests pass with mock server

## Tasks

### 1. Create single chat endpoint

- Create `chat/chat.h` and `chat/chat.c`
- Implement request handling for `/api/conduit/chat`
- Extract database parameter (required)
- Make engine parameter optional (fallback to default)

### 2. Create batch chat endpoint

- Create `chats/chats.h` and `chats/chats.c`
- Implement request handling for `/api/conduit/chats`
- Process multiple chats in parallel using curl_multi
- Aggregate results and timing

### 3. Implement database parameter extraction

- Validate database exists and chat is enabled for that database
- Return appropriate error for invalid/disabled databases

### 4. Implement default engine fallback

- When engine parameter omitted, use `chat_engine_cache_get_default(database)`
- Return error if no default engine configured

### 5. Add Image/Multimodal Support (Day One)

- Accept `content` as string OR array of parts (text + images)
- Support OpenAI-compatible vision format:

  ```json
  {
    "messages": [{
      "role": "user",
      "content": [
        {"type": "text", "text": "What's in this image?"},
        {"type": "image_url", "image_url": {"url": "data:image/jpeg;base64,..."}}
      ]
    }]
  }
  ```

- Pass through to provider unchanged (OpenAI, xAI, Ollama support this natively)
- Add engine limits: `max_images_per_message`, `max_payload_mb`

### 6. Default params from engine config

- Use engine `default_params` when client omits `temperature`, `max_tokens`, etc.

### 7. Integrate with proxy, builder, and parser

- Use chat_request_builder_auto to build provider-specific requests
- Use chat_proxy_send to send requests to external APIs
- Use chat_response_parser_auto to parse responses
- Handle errors appropriately and return standardized format

### 8. Register endpoints in api_service.c

- Add to `endpoint_expects_json()`: `"conduit/chat"`, `"conduit/chats"`
- Add routing `strcmp()` blocks for both paths
- Add to appropriate permission checking (public endpoints)

### 9. Unit and integration tests

- Test with mock server simulating AI provider responses
- Verify error handling for invalid requests
- Test image/message format variations
- Test batch processing with mixed success/failure scenarios

## Verification Steps

1. Verify endpoint registration in api_service.c
2. Test single chat with text-only messages
3. Test single chat with multimodal content (image + text)
4. Test batch chat with multiple requests
5. Test default engine fallback functionality
6. Verify engine limits are enforced (images, payload size)
7. Confirm proper error responses for invalid databases/engines
8. Run unit and integration tests with mock server
9. Ensure existing conduit functionality remains unaffected