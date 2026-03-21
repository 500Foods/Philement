# Chat Service - Phase 6: Additional Provider Support

## Objective
Add support for additional AI providers beyond OpenAI-compatible APIs, specifically Anthropic native format and Ollama native API.

## Prerequisites
- Phase 5 completed and verified (cross-database endpoints working)

## Testable Gate
Before proceeding to Phase 7, the following must be verified:
- Anthropic native format requests are properly built and responses parsed
- Ollama native API support works (when use_native_api: true)
- Provider-specific configuration options are correctly processed
- Unit tests pass for each provider's specific request/response handling
- Existing OpenAI-compatible provider support remains unaffected

## Tasks

### 1. Implement Anthropic native format support
- Extend chat_request_builder.c with Anthropic-specific function
- Convert OpenAI-format messages to Anthropic format when needed
- Add anthropic-version header handling in chat_proxy.c
- Implement Anthropic response parser in chat_response_parser.c
- Handle provider-specific parameters (top_k, etc.)

### 2. Implement Ollama native API support
- Add support for Ollama's native endpoint: http://localhost:11434/api/chat
- Handle Ollama-specific parameters (num_predict instead of max_tokens)
- Implement Ollama native response parser
- Support switching between OpenAI-compatible and native modes via config

### 3. Enhance provider-specific configuration
- Update ChatEngineConfig to store provider-specific settings
- Add supported_modalities array to engine config
- Add max_images_per_message and max_payload_mb limits
- Validate requests against provider limits before proxying

### 4. Auto-detection improvements
- Refine chat_request_build_auto to better handle provider detection
- Improve chat_response_parse_auto accuracy
- Add fallback mechanisms for unsupported features

### 5. Provider-specific error handling
- Map provider-specific error codes to standard response format
- Handle rate limiting (429) with provider-specific retry-after headers
- Parse provider error messages for better debugging

### 6. Unit tests
- Test Anthropic request building with various message formats
- Test Ollama native API request/response handling
- Verify provider limit enforcement
- Test error handling for each provider
- Ensure backward compatibility with OpenAI-compatible providers

## Verification Steps
1. Verify Anthropic native format requests are correctly formed
2. Test Anthropic response parsing with sample responses
3. Verify Ollama native API works when use_native_api: true
4. Confirm Ollama still works with use_native_api: false (OpenAI-compatible)
5. Test provider-specific limits (images, payload size) are enforced
6. Run unit tests for each provider's specific functionality
7. Verify existing OpenAI-compatible provider support unchanged
8. Test error handling and mapping for each provider type