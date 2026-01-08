# AUTH ENDPOINTS - SUBSYSTEM INTEGRATION

## Integration Points

- **Database**: Uses existing DQM (Database Queue Manager) architecture with query ID system
- **Logging**: Integrates with Hydrogen logging system using SR_AUTH component
- **Configuration**: JWT secrets and timeouts from app_config->auth section
- **API Framework**: Follows existing endpoint patterns with CORS headers and JSON responses
- **Subsystem Registry**: Registers with launch/landing system for proper initialization

## Detailed Subsystem Integration

### Database Queue Manager (DQM) Integration

```c
// Auth service registers with DQM for database access
bool init_auth_database_queues(void) {
    // Register auth-specific queues with Acuranzo database
    dqm_register_queue("auth-fast", DATABASE_ACURANZO, QUEUE_TYPE_FAST);
    dqm_register_queue("auth-slow", DATABASE_ACURANZO, QUEUE_TYPE_SLOW);

    // Set queue priorities for auth operations
    dqm_set_priority("auth-fast", PRIORITY_HIGH); // Login attempts
    dqm_set_priority("auth-slow", PRIORITY_NORMAL); // Registration, logging

    return true;
}
```

### Logging System Integration

```c
// Auth-specific logging component
#define SR_AUTH "AUTH"

// Structured logging for auth events
void log_auth_event(const char* event_type, const char* user_id,
                   const char* ip_address, const char* details) {
    log_this_structured(SR_AUTH, LOG_LEVEL_INFO, json_object({
        "event": event_type,
        "user_id": user_id,
        "ip": ip_address,
        "timestamp": time(NULL),
        "details": details
    }));
}
```

### Configuration Integration

```c
// Auth configuration structure
typedef struct {
    int jwt_lifetime_seconds;
    int max_failed_attempts;
    int rate_limit_window_minutes;
    int ip_block_duration_minutes;
    bool enable_registration;
    char* jwt_secret_env_var;
    char* allowed_origins; // CORS
} auth_config_t;

// Load from app_config->auth section
auth_config_t* load_auth_config(void) {
    return config_get_section("auth");
}
```

### API Framework Integration

```c
// Register auth endpoints with API subsystem
bool register_auth_endpoints(void) {
    // Register each endpoint with the API router
    api_register_endpoint(HTTP_METHOD_POST, "/api/auth/login", handle_auth_login);
    api_register_endpoint(HTTP_METHOD_POST, "/api/auth/renew", handle_auth_renew);
    api_register_endpoint(HTTP_METHOD_POST, "/api/auth/logout", handle_auth_logout);
    api_register_endpoint(HTTP_METHOD_POST, "/api/auth/register", handle_auth_register);

    // Add CORS headers for auth endpoints
    api_add_cors_headers("/api/auth/*", "POST", auth_config.allowed_origins);

    return true;
}
```

### Launch/Landing System Integration

```c
// Auth subsystem launch function
bool launch_auth_subsystem(void) {
    log_this(SR_AUTH, "Launching auth subsystem");

    // Phase 1: Initialize dependencies
    if (!init_auth_database_queues()) return false;
    if (!load_auth_config()) return false;

    // Phase 2: Register endpoints
    if (!register_auth_endpoints()) return false;

    // Phase 3: Start background services
    if (!start_auth_background_services()) return false;

    log_this(SR_AUTH, "Auth subsystem launched successfully");
    return true;
}

// Auth subsystem landing function
bool land_auth_subsystem(void) {
    log_this(SR_AUTH, "Landing auth subsystem");

    // Phase 1: Stop accepting new requests
    api_disable_endpoints("/api/auth/*");

    // Phase 2: Complete pending operations
    wait_for_pending_auth_operations();

    // Phase 3: Cleanup resources
    cleanup_auth_resources();

    log_this(SR_AUTH, "Auth subsystem landed successfully");
    return true;
}