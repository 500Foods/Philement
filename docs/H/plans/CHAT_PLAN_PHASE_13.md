# Chat Service - Phase 13: Advanced Multi-modal Features

## Objective
Implement advanced multi-modal support including media upload endpoint and extended segment metadata for various content types.

## Prerequisites
- Phase 12 completed and verified (streaming support working)

## Testable Gate
Before proceeding to Phase 14, the following must be verified:
- POST /api/conduit/upload endpoint registered and functional
- Media assets table created with proper schema
- Conversation segments table extended with content_type, mime_type, metadata
- Provider-specific handling works (especially Anthropic translation)
- Engine limits enforced (max_images_per_message, max_payload_mb)
- Unit and integration tests pass for multi-modal features
- No regression in existing chat or streaming functionality

## Tasks

### 1. Create media upload endpoint
- Create `upload/upload.h` and `upload/upload.c`
- Handle POST /api/conduit/upload and /api/conduit/auth_upload
- Accept multipart/form-data file uploads
- Compute SHA-256 hash of file content
- Store in media_assets table (or S3/minio if configured)
- Return media_hash for use in chat messages
- Support both public and authenticated variants

### 2. Create media_assets table (Helium migration)
- media_hash VARCHAR(64) PRIMARY KEY (SHA-256 of file content)
- media_data BLOB (binary data or S3 URL reference)
- media_size INTEGER
- mime_type VARCHAR(100)
- created_at, last_accessed, access_count tracking
- Appropriate indexes for performance

### 3. Extend conversation_segments table
- Add content_type VARCHAR(20) DEFAULT 'text' (text|image|pdf|audio|document)
- Add mime_type VARCHAR(100) (image/jpeg, application/pdf, etc.)
- Add metadata JSONB for provider-specific data
- Store additional info like dimensions, duration, token estimates

### 4. Implement media URL handling
- Support media:hash format in chat messages
- Server resolves media:hash to actual base64 or URL before proxying
- Cache resolved media to avoid repeated lookups
- Handle missing media gracefully (broken image/icons)

### 5. Provider-specific multi-modal handling
- Extend chat_request_builder for provider-specific translation
- Implement Anthropic vision format translation (as in original plan)
- Handle varying vision capabilities across providers
- Graceful degradation for providers with limited multi-modal support

### 6. Implement engine limits enforcement
- Validate max_images_per_message per engine configuration
- Validate max_payload_mb before processing requests
- Return clear error messages when limits exceeded
- Log limit violations for monitoring and abuse detection

### 7. Unit and integration tests
- Test media upload and hash generation
- Test media:hash resolution in chat messages
- Verify provider-specific format translation (especially Anthropic)
- Test engine limits enforcement
- Test storage and retrieval of various media types
- Ensure existing text-only chat unaffected

## Verification Steps
1. Verify media upload endpoint registration
2. Test uploading various file types (images, PDFs, etc.)
3. Confirm media:hash resolution works in chat messages
4. Verify Anthropic vision format translation accuracy
5. Test engine limits are properly enforced
6. Run unit and integration tests for multi-modal features
7. Ensure existing chat, streaming, and cache functionality unaffected
8. Test media asset deduplication (same file stored once)