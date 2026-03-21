# Chat Service - Phase 5: Cross-Database Endpoints (alt_chat, alt_chats)

## Objective
Implement cross-database chat endpoints that allow admin users to override database from JWT claims.

## Prerequisites
- Phase 4 completed and verified (authenticated endpoints working)

## Testable Gate
Before proceeding to Phase 6, the following must be verified:
- POST /api/conduit/alt_chat endpoint is registered and accessible
- Authenticated admin users can override database via request body
- POST /api/conduit/alt_chats endpoint works for batch cross-database chats
- Proper authorization checks ensure only admin users can use these endpoints
- Database override works correctly while maintaining security boundaries
- Unit and integration tests pass for cross-database flows

## Tasks

### 1. Create cross-database single chat endpoint
- Create `alt_chat/alt_chat.h` and `alt_chat/alt_chat.c`
- Implement request handling for `/api/conduit/alt_chat`
- Require valid JWT token (authenticated endpoint)
- Extract database from request body (overrides JWT claims)
- Verify user has admin privileges for the requested database
- Engine parameter required

### 2. Create cross-database batch chat endpoint
- Create `alt_chats/alt_chats.h` and `alt_chats/alt_chats.c`
- Implement request handling for `/api/conduit/alt_chats`
- Same authentication and authorization as alt_chat
- Process multiple chats in parallel with potential different databases

### 3. Implement admin authorization
- Reuse existing authorization helpers or implement new checks
- Verify user role permits cross-database access
- Log admin override attempts for audit trail
- Return 403 for non-admin users attempting to use alt endpoints

### 4. Database validation
- Validate requested database exists and chat is enabled
- Verify requesting admin has access to target database
- Return appropriate error for invalid/unauthorized databases

### 5. Register endpoints in api_service.c
- Add to `endpoint_requires_auth()`: `"conduit/alt_chat"`, `"conduit/alt_chats"`
- Add to `endpoint_expects_json()`: `"conduit/alt_chat"`, `"conduit/alt_chats"`
- Add routing `strcmp()` blocks for both paths

### 6. Security considerations
- Ensure database override cannot bypass intended restrictions
- Maintain separation between user's default database and override capability
- Audit all alt_* endpoint usage for security monitoring

### 7. Unit and integration tests
- Test with valid admin JWT and authorized database override
- Test with non-admin JWT attempting to use alt endpoints (should get 403)
- Test with invalid/non-existent database in override
- Test batch cross-database chats
- Verify proper error responses for authorization failures
- Test that regular auth endpoints still work normally

## Verification Steps
1. Verify endpoint registration in required auth and expects_json arrays
2. Test alt_chat with valid admin JWT and authorized database - should succeed
3. Test alt_chat with valid non-admin JWT - should return 403
4. Test alt_chat with invalid database in request body - should return appropriate error
5. Test alt_chats with multiple databases and valid admin privileges
6. Verify admin override actions are logged appropriately
7. Run unit and integration tests for cross-database endpoints
8. Ensure existing auth endpoints (auth_chat, auth_chats) remain unaffected
9. Confirm regular users cannot access alt endpoints even with valid JWT