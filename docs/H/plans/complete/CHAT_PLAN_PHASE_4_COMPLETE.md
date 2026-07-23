# Chat Service - Phase 4: Authenticated Endpoints (auth_chat, auth_chats)

## Objective

Implement authenticated chat endpoints that extract database from JWT claims.

## Prerequisites

- Phase 3 completed and verified (public chat endpoints working)

## Testable Gate

Before proceeding to Phase 5, the following must be verified:

- POST /api/conduit/auth_chat endpoint is registered and accessible
- Authenticated single chat requests work correctly
- POST /api/conduit/auth_chats endpoint works for batch authenticated chats
- Database is properly extracted from JWT claims
- JWT validation reuses existing helpers from conduit_helpers.h
- Endpoints require valid JWT token (401 for missing/invalid token)
- Unit and integration tests pass for authenticated flows

## Tasks

### 1. Create authenticated single chat endpoint

- Create `auth_chat/auth_chat.h` and `auth_chat/auth_chat.c`
- Implement request handling for `/api/conduit/auth_chat`
- Extract database from `Authorization: Bearer <jwt>` token claims
- Reuse existing JWT validation helpers from `conduit_helpers.h`
- Engine parameter required (no fallback to query string/database config)

### 2. Create authenticated batch chat endpoint

- Create `auth_chats/auth_chats.h` and `auth_chats/auth_chats.c`
- Implement request handling for `/api/conduit/auth_chats`
- Same database extraction as auth_chat
- Process multiple chats in parallel

### 3. Implement JWT validation reuse

- Use existing functions from `conduit_helpers.h` for token validation
- Extract database claim using same mechanism as auth_query endpoints
- Handle token expiration and malformed tokens appropriately

### 4. Register endpoints in api_service.c

- Add to `endpoint_requires_auth()`: `"conduit/auth_chat"`, `"conduit/auth_chats"`
- Add to `endpoint_expects_json()`: `"conduit/auth_chat"`, `"conduit/auth_chats"`
- Add routing `strcmp()` blocks for both paths

### 5. Error handling

- Return 401 for missing/invalid JWT
- Return 400 for missing required fields
- Return 404 for engine not found in database
- Maintain consistent error response format

### 6. Unit and integration tests

- Test with valid/invalid JWT tokens
- Verify database extraction from various JWT formats
- Test authenticated chat flow with mock AI provider
- Test batch authenticated chats
- Verify proper error responses

## Verification Steps

1. Verify endpoint registration in required auth and expects_json arrays
2. Test auth_chat with valid JWT - should extract database and process chat
3. Test auth_chat with missing/invalid JWT - should return 401
4. Test auth_chats with valid JWT - should process multiple chats
5. Verify database extraction matches JWT claims (not request body)
6. Confirm engine parameter works correctly with authenticated flow
7. Run unit and integration tests for authenticated endpoints
8. Ensure existing public chat endpoints remain unaffected
9. Verify no regression in existing auth endpoints (auth_query, etc.)