# Conduit Service - auth_query Endpoint Fix Plan

## Problem Analysis

### Current Issues

1. **HTTP 000 Responses**: When JWT validation fails, the server closes connections with HTTP 000 instead of returning proper error responses
2. **Missing JSON Error Responses**: Error cases are not returning JSON-formatted error messages
3. **Test Failures**: Test 52 cannot parse responses due to invalid HTTP status codes
4. **Connection Corruption**: MHD connections may be getting corrupted during error handling

### Root Cause Analysis

The main issues appear to be in the `handle_conduit_auth_query_request()` function in `auth_query.c`:

1. **JWT Validation Integration**: The function is not properly checking the return value from `validate_jwt_from_header()`
2. **Response Creation**: Error responses are not being properly created and queued
3. **Memory Management**: Resources are not being properly cleaned up in error paths
4. **Connection State**: MHD connections may be getting accessed after being cleaned up

## Fix Plan - Step by Step Checklist

### Phase 1: JWT Validation Error Handling

#### 1.1 Fix Missing Authorization Header Response

**Current Issue**: When Authorization header is missing, no proper response is sent

**Fix**:

- Add explicit check for missing Authorization header
- Return HTTP 401 with JSON error: `{"success": false, "error": "Authentication required - include Authorization: Bearer <token> header"}`
- Ensure MHD connection is properly cleaned up after sending response

#### 1.2 Fix Invalid Authorization Header Format Response

**Current Issue**: Invalid Authorization header format (not "Bearer <token>") causes connection closure

**Fix**:

- Add validation for "Bearer <token>" format
- Return HTTP 401 with JSON error: `{"success": false, "error": "Invalid Authorization header format - must be 'Bearer <token>'"}`
- Verify response is sent before connection cleanup

#### 1.3 Fix JWT Validation Failure Response

**Current Issue**: Invalid/expired JWT tokens cause connection closure instead of proper error response

**Fix**:

- Check return value from `validate_jwt_from_header()`
- Return HTTP 401 with JSON error: `{"success": false, "error": "Invalid or expired JWT token"}`
- Ensure `free_jwt_validation_result()` is called before sending response

#### 1.4 Fix Missing Database Claim Response

**Current Issue**: JWT tokens missing database information cause connection closure

**Fix**:

- Validate database claim exists in JWT
- Return HTTP 401 with JSON error: `{"success": false, "error": "JWT token missing database information"}`
- Check all allocated memory is freed before sending response

#### 1.5 Fix Memory Allocation Failure Response

**Current Issue**: Memory allocation failures during JWT validation cause crashes

**Fix**:

- Add error checking for memory allocation failures
- Return HTTP 500 with JSON error: `{"success": false, "error": "Internal server error"}`
- Ensure existing resources are freed before sending error response

### Phase 2: Response Creation and Sending

#### 2.1 Validate JSON Response Creation

**Current Issue**: JSON creation failures may cause crashes or silent failures

**Fix**:

- Add error checking for `json_object_set_new()` calls
- Verify `json_dumps()` returns non-NULL before creating MHD response
- Add error handling for JSON creation failures

#### 2.2 Validate MHD Response Creation

**Current Issue**: MHD response creation failures may cause crashes

**Fix**:

- Ensure `MHD_create_response_from_buffer()` returns non-NULL
- Add error handling if MHD response creation fails
- Verify response headers are properly set before queuing response

#### 2.3 Validate Response Queueing

**Current Issue**: Response queueing failures may cause silent failures

**Fix**:

- Ensure `MHD_queue_response()` returns MHD_YES (success)
- Add error handling if response queueing fails
- Verify `MHD_destroy_response()` is called after successful queueing

#### 2.4 Validate Connection Cleanup

**Current Issue**: Connection cleanup issues may cause crashes or resource leaks

**Fix**:

- Ensure `api_free_post_buffer()` is called appropriately in all error paths
- Verify no double-free occurs in error handling
- Confirm connection is not accessed after being cleaned up

### Phase 3: auth_query Function Error Handling

#### 3.1 Fix JWT Validation Integration

**Current Issue**: JWT validation return value is not properly checked

**Fix**:

- Add proper error checking for `validate_jwt_from_header()` return value
- Ensure all allocated resources are freed when JWT validation fails
- Verify function returns MHD_YES after sending error response

#### 3.2 Fix Database Claim Validation

**Current Issue**: Missing database claims are not properly handled

**Fix**:

- Add validation for database claim existence and non-emptiness
- Return proper HTTP 401 response with appropriate error message
- Ensure all resources are cleaned up before returning

#### 3.3 Fix Database Configuration Validation

**Current Issue**: Database from JWT claims not found in configuration

**Fix**:

- Add validation for database existence in configuration
- Return HTTP 401 with database name in JSON body
- Use appropriate HTTP status codes (401 for auth issues, 404 for not found)

#### 3.4 Fix Query Lookup Error Handling

**Current Issue**: Query lookup failures not properly handled

**Fix**:

- Add proper error response when query lookup fails
- Return HTTP 200 with `success: false` for invalid query references
- Use appropriate HTTP status for database not found

### Phase 4: Memory Management and Cleanup

#### 4.1 Fix JWT Result Memory Management

**Current Issue**: JWT result memory not properly freed in error paths

**Fix**:

- Ensure `free_jwt_validation_result()` is called for all allocated JWT results
- Verify `free()` is called for the allocated `jwt_validation_result_t` struct
- Check for potential double-free scenarios

#### 4.2 Fix Request JSON Memory Management

**Current Issue**: Request JSON not properly freed in error paths

**Fix**:

- Ensure `json_decref(request_json)` is called in all error paths
- Verify JSON is not accessed after being decref'd
- Check for memory leaks in parameter processing

#### 4.3 Fix Parameter Processing Cleanup

**Current Issue**: Parameter processing resources not properly freed in error paths

**Fix**:

- Ensure all allocated parameter resources are freed in error paths
- Verify `free_parameter_list()`, `free(converted_sql)`, and `free(ordered_params)` are called appropriately
- Check for memory leaks in complex error scenarios

#### 4.4 Fix Query ID and Message Cleanup

**Current Issue**: Query ID and message memory not properly freed in error paths

**Fix**:

- Ensure `free(query_id)` and `free(message)` are called in all error paths
- Verify cleanup happens before returning from function
- Check for potential use-after-free issues

### Phase 5: Testing and Validation

#### 5.1 Unit Test JWT Validation

**Test Cases**:

- Missing Authorization header
- Invalid Authorization header format
- Invalid JWT token
- Expired JWT token
- Malformed JWT token
- Missing database claim
- Revoked JWT token

#### 5.2 Integration Test auth_query Endpoint

**Test Cases**:

- Valid JWT with valid query
- Valid JWT with invalid query reference
- Invalid JWT (various failure modes)
- Missing JWT
- Expired JWT
- Revoked JWT
- JWT with missing database claim

#### 5.3 Cross-Engine Testing

**Test Across All 7 Database Engines**:

- PostgreSQL
- MySQL
- SQLite
- DB2
- MariaDB
- CockroachDB
- YugabyteDB

#### 5.4 Performance Testing

**Test Scenarios**:

- Concurrent requests with error cases
- Resource leak detection
- Response time validation for error cases

### Phase 6: Documentation and Monitoring

#### 6.1 Update Swagger Documentation

**Documentation Updates**:

- Add detailed error response examples for auth_query endpoint
- Document all possible HTTP status codes and their meanings
- Include example JSON error responses

#### 6.2 Add Logging Enhancements

**Logging Improvements**:

- Add detailed logging for JWT validation failures
- Include error codes and specific failure reasons in logs
- Add correlation IDs for tracking requests through the system

#### 6.3 Add Health Monitoring

**Monitoring Metrics**:

- JWT validation success/failure rates
- HTTP 401 response rates for auth_query endpoint
- Average response times for error cases

## Implementation Priority

### High Priority (Must Fix for Test 52)

1. Fix missing Authorization header response
2. Fix invalid Authorization header format response  
3. Fix JWT validation failure response
4. Fix missing database claim response
5. Ensure proper HTTP status codes are returned

### Medium Priority (Improve Robustness)

1. Fix memory allocation failure response
2. Add comprehensive error checking
3. Improve logging and monitoring
4. Add unit tests for error cases

### Low Priority (Enhancement)

1. Add correlation IDs for request tracking
2. Implement rate limiting for auth failures
3. Add detailed error response schemas

## Success Criteria

### Functional Requirements

- [ ] All JWT validation error cases return proper HTTP status codes
- [ ] All error responses include valid JSON with `success: false`
- [ ] Test 52 passes all test cases
- [ ] No HTTP 000 responses from server
- [ ] Proper cleanup of all allocated resources

### Performance Requirements

- [ ] Error response times are acceptable (< 100ms)
- [ ] No memory leaks in error handling paths
- [ ] Concurrent error requests don't cause resource exhaustion

### Quality Requirements

- [ ] All error paths have comprehensive unit tests
- [ ] Code follows project coding standards
- [ ] Proper logging and monitoring implemented
- [ ] Documentation is updated with error response examples

## Risk Assessment

### High Risk Items

- **Memory Management**: Improper cleanup could cause memory leaks or crashes
- **Connection State**: Corrupted MHD connections could cause server instability
- **JSON Creation**: Failed JSON creation could cause server crashes

### Mitigation Strategies

- Thorough code review of all error paths
- Comprehensive unit testing of error scenarios
- Memory leak detection with Valgrind
- Integration testing with actual JWT tokens

### Low Risk Items

- **Logging Overhead**: Additional logging should have minimal performance impact
- **Documentation Updates**: Low risk, mainly affects developer experience

## Timeline Estimate

### Phase 1-2 (JWT Error Handling): 2-3 hours

- Fix all JWT validation error responses
- Validate response creation and sending

### Phase 3 (auth_query Integration): 1-2 hours

- Fix error handling in main function
- Ensure proper resource cleanup

### Phase 4 (Memory Management): 1-2 hours

- Fix all memory management issues
- Add comprehensive cleanup

### Phase 5 (Testing): 2-3 hours

- Create unit tests
- Integration testing
- Cross-engine validation

### Phase 6 (Documentation): 1 hour

- Update documentation
- Add monitoring

## Success Metrics

### Immediate Metrics

- Test 52 passes all test cases
- No HTTP 000 responses in server logs
- All JWT validation errors return proper HTTP 401

### Long-term Metrics

- Zero memory leaks in auth_query endpoint
- Consistent error response format across all endpoints
- High test coverage for error scenarios
- Low error rate in production

## Next Steps

1. **Immediate**: Fix JWT validation error responses (Phase 1)
2. **Short-term**: Complete auth_query integration and testing (Phases 2-3)
3. **Medium-term**: Add comprehensive testing and documentation (Phases 4-6)
4. **Long-term**: Monitor production performance and error rates

This plan provides a systematic approach to fixing the `auth_query` endpoint while ensuring robustness, proper error handling, and comprehensive testing coverage.