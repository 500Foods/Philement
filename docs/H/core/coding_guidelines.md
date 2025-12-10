# C Coding Guidelines for Hydrogen

This document outlines the C programming guidelines and patterns used in the Hydrogen project. It serves as a reference for developers who may not work with C on a daily basis.

## Project Structure

### Source File Organization

Each `.c` file should follow this organization:

1. File-level documentation explaining purpose and design decisions
2. Include directives in this order:
   - Core system headers
   - Standard C headers
   - Third-party libraries
   - Project headers
3. Declarations in this order:
   - External declarations (extern functions/variables from other files)
   - Public interface (functions exposed to other files)
   - Private declarations (static functions used only in this file)
4. Implementation of functions

Example organization:

```c
/*
 * Component purpose and design decisions
 */

// Core system headers
#include <sys/types.h>
#include <pthread.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>

// Project headers
#include "component.h"

// External declarations
extern void global_function(void);

// Public interface
int component_initialize(void);
void component_cleanup(void);

// Private declarations
static void helper_function(void);

// Implementation follows...
```

### Header Files

- Each `.c` file should have a corresponding `.h` file (except for main program files)
- Use header guards in all header files
- Keep interface declarations in headers, implementations in source files

Example header guard:

```c
#ifndef HYDROGEN_COMPONENT_H
#define HYDROGEN_COMPONENT_H

// Declarations here

#endif // HYDROGEN_COMPONENT_H
```

## Standard Library Usage

### Common Standard Headers

```c
#include <signal.h>    // Signal handling
#include <pthread.h>   // POSIX threading
#include <errno.h>     // Error codes
#include <time.h>      // Time functions
#include <stdbool.h>   // Boolean type
#include <stdint.h>    // Fixed-width integers
#include <string.h>    // String manipulation
#include <stdlib.h>    // General utilities
```

### Project-Specific Headers

```c
#include "logging.h"   // Logging functions
#include "state.h"     // State management
#include "config.h"    // Configuration
```

## Error Handling

1. Use return codes for function status
2. Set errno when appropriate
3. Log errors with context
4. Clean up resources on error paths

Example:

```c
if (operation_failed) {
    log_this("Component", "Operation failed: specific reason", 3, true, false, true);
    cleanup_resources();
    return false;
}
```

## Threading Practices

1. Use POSIX threads (pthread)
2. Protect shared resources with mutexes
3. Use condition variables for signaling
4. Follow consistent locking order to prevent deadlocks

Example:

```c
pthread_mutex_lock(&resource_mutex);
// Access shared resource
pthread_mutex_unlock(&resource_mutex);
```

## Logging Conventions

Use the project's logging system with appropriate severity levels:

```c
log_this("Component", "Message", severity, to_console, to_file, to_websocket);
```

Severity levels:

- 1: Debug
- 2: Info
- 3: Warning
- 4: Error
- 5: Critical

## Memory Management

1. Always check malloc/calloc return values
2. Free resources in reverse order of allocation
3. Use valgrind for memory leak detection
4. Consider using static allocation for fixed-size resources

Example:

```c
void* ptr = malloc(size);
if (!ptr) {
    log_this("Component", "Memory allocation failed", 4, true, false, true);
    return false;
}
```

## Code Documentation

1. Each file should start with a comment block explaining its purpose
2. Document architectural decisions with "Why" explanations
3. Use clear function comments explaining:
   - Purpose
   - Parameters
   - Return values
   - Error conditions

Example:

```c
/*
 * Why This Architecture Matters:
 * 1. Safety-Critical Design
 *    - Controlled startup sequence
 *    - Graceful shutdown handling
 * 
 * 2. Real-time Requirements
 *    - Immediate command response
 *    - Continuous monitoring
 */
```

## Code Quality and Linting

The project uses automated linting tools to enforce code quality and consistency. These tools are integrated into our test harness (`test_z_codebase.sh`) and run as part of our continuous integration process.

### Linting Tools

1. **C/C++ Code (cppcheck)**
   - Static analysis for C/C++ source files
   - Detects common programming errors
   - Enforces project coding standards
   - Checks memory management and resource handling

2. **Markdown Files (markdownlint)**
   - Ensures consistent documentation formatting
   - Uses `.lintignore-markdown` for configuration
   - Enforces documentation standards and readability
   - Maintains consistent heading hierarchy

3. **JSON Files (jsonlint)**
   - Validates JSON file syntax and structure
   - Ensures proper formatting and indentation
   - Catches common configuration errors
   - Reports precise error locations

4. **JavaScript Files (eslint)**
   - Enforces JavaScript coding standards
   - Checks for potential runtime errors
   - Maintains consistent code style
   - Supports modern JavaScript features

5. **CSS Files (stylelint)**
   - Ensures consistent CSS formatting
   - Catches common styling errors
   - Enforces style guide rules
   - Validates property values

6. **HTML Files (htmlhint)**
   - Validates HTML syntax

7. **XML/SVG Files (xmlstarlet)**
   - Validates XML/SVG syntax and structure
   - Enforces accessibility standards
   - Checks for common markup issues
   - Ensures proper tag nesting

### Linting Configuration Files

The project uses two configuration files to control linting behavior:

1. **.lintignore**
   - Located in the project root directory
   - Uses glob patterns to specify exclusions
   - Applied to all linting tools except Markdown
   - Example patterns:

     ```files
     build/*           # Exclude build directories
     tests/config*     # Exclude test configurations
     src/config/*.inc  # Exclude generated includes
     ```

2. **.lintignore-markdown**
   - Located in the project root directory
   - Specific configuration for Markdown files
   - Controls Markdown-specific rules
   - Maintains documentation consistency

### Running Lint Checks

Linting is integrated into our test suite and can be run in two ways:

```bash
# Run all tests including linting
./tests/test_all.sh

# Run only linting checks
./tests/test_z_codebase.sh
```

The test harness provides:

- Comprehensive linting across all file types
- Clear error reporting with file locations
- Warnings for missing file types (expected for HTML/CSS/JS initially)
- Proper exclusion handling via .lintignore files
- Concise error messages for quick problem identification

### Adding Lint Exclusions

To exclude files from linting:

1. Edit `.lintignore` for general exclusions:

   ```files
   # Add glob patterns, one per line
   path/to/exclude/*
   specific/file.ext
   ```

2. Edit `.lintignore-markdown` for Markdown-specific rules:

   ```list
   # Configure Markdown-specific exclusions
   # and rule customizations
   ```

Remember:

- Use glob patterns similar to .gitignore
- Add comments to explain non-obvious exclusions
- Keep exclusions minimal and justified
- Review exclusions periodically

## Build System

The project uses Make for building. Key targets:

- `make` - Build the project
- `make debug` - Build with debug symbols
- `make clean` - Clean build artifacts
- `make valgrind` - Run with memory checking

## Time and Date Formatting

1. Display time values with millisecond precision (3 decimal places) when reporting durations or timestamps
2. Use ISO 8601 format (YYYY-MM-DD HH:MM:SS.sss) for date-time values where appropriate
3. Be consistent with time unit representations across logs and interfaces

Example:

```c
// Displaying elapsed time with millisecond precision
printf("Operation completed in %.3f seconds\n", elapsed_time);

// Displaying timestamps with millisecond precision
printf("Event occurred at %02d:%02d:%02d.%03d\n", 
       hour, minute, second, millisecond);
```

## Security Coding Standards

The following guidelines apply to security-critical code, such as the OIDC implementation:

1. **Defense in Depth**
   - Never rely on a single security control
   - Implement multiple layers of validation
   - Assume that any single security mechanism might fail

2. **Principle of Least Privilege**
   - Code should operate with minimal required permissions
   - Limit access to sensitive data to only the components that need it
   - Use separate credentials or tokens for different operations

3. **Never Trust External Input**
   - Always validate all inputs from external sources
   - Apply strict validation to all security-critical parameters
   - Sanitize inputs before using them in sensitive operations

4. **Fail Securely**
   - Default to the secure state on any failure
   - Return specific error codes without leaking sensitive information
   - Close any open resources on failure paths

Example of secure input validation:

```c
// Validate all security-critical parameters
if (!validate_client_id(client_id) || !validate_redirect_uri(redirect_uri)) {
    log_this("OIDC", "Invalid client parameters", 4, true, true, true);
    return AUTH_ERROR_INVALID_CLIENT;
}
```

## JWT and Token Handling

When working with JWTs and security tokens:

1. **Token Validation Requirements**
   - Always validate all of the following:
     - Token integrity (signature)
     - Issuer (`iss` claim)
     - Audience (`aud` claim)
     - Expiration time (`exp` claim)
     - Token type and other required claims

2. **Token Generation**
   - Use cryptographically secure random number generators
   - Set appropriate token lifetimes based on security requirements
   - Include minimal necessary claims

3. **Token Storage**
   - Never store sensitive tokens in plain text
   - Use secure storage mechanisms with appropriate access controls
   - Consider using hardware security modules for high-security deployments

4. **Token Transmission**
   - Only transmit tokens over secure channels (TLS)
   - Use appropriate headers and content types
   - Apply additional protections for high-value tokens (e.g., token binding)

Example of proper token validation:

```c
bool validate_token(const char* token) {
    // Check token format first
    if (!is_valid_jwt_format(token)) {
        return false;
    }
    
    // Verify signature
    if (!verify_signature(token, public_key)) {
        log_this("TokenService", "Token signature verification failed", 4, true, true, true);
        return false;
    }
    
    // Extract and validate claims
    int current_time = get_current_time();
    
    if (token_expired(token, current_time)) {
        log_this("TokenService", "Token has expired", 3, true, false, true);
        return false;
    }
    
    if (!validate_issuer(token, expected_issuer)) {
        log_this("TokenService", "Invalid token issuer", 4, true, true, true);
        return false;
    }
    
    if (!validate_audience(token, expected_audience)) {
        log_this("TokenService", "Invalid token audience", 4, true, true, true);
        return false;
    }
    
    return true;
}
```

## Cryptographic Operations

When implementing cryptographic operations:

1. **Use Established Libraries**
   - Prefer well-tested cryptographic libraries (e.g., OpenSSL)
   - Never implement your own cryptographic algorithms
   - Keep cryptographic libraries updated to address known vulnerabilities

2. **Key Management**
   - Implement secure key generation using proper entropy sources
   - Store keys securely, never in source code or plain text configurations
   - Implement key rotation mechanisms with configurable intervals
   - Protect key material in memory (avoid unnecessary copies, clear after use)

3. **Algorithm Selection**
   - Use modern, widely accepted algorithms (e.g., RSA-2048+, ECDSA, SHA-256+)
   - Avoid deprecated or weak algorithms
   - Implement algorithm negotiation where appropriate

4. **Randomness**
   - Use cryptographically secure random number generators
   - Never use `rand()` for security purposes
   - Verify that random number generators are properly seeded

5. **Version Control and Key Protection**
   - Never commit cryptographic keys, credentials, or secrets to Git repositories
   - Use `.gitignore` to exclude key files, compiled binaries, and credential files
   - Consider using environment variables for runtime secrets
   - Implement separate configuration loading for development vs. production keys
   - Create sample configuration files (e.g., `config.sample.json`) without real keys
   - Document key file locations but never include the actual key files in repositories
   - Be vigilant with temporary test keys which might accidentally be committed
   - Regularly audit repositories for accidentally committed secrets

Example of secure key management:

```c
// Generate a new RSA key pair
EVP_PKEY* generate_rsa_key(int key_size) {
    EVP_PKEY* key = NULL;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    
    if (!ctx) {
        log_this("KeyManager", "Failed to create key context", 4, true, true, true);
        return NULL;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        log_this("KeyManager", "Failed to initialize key generation", 4, true, true, true);
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, key_size) <= 0) {
        log_this("KeyManager", "Failed to set key size", 4, true, true, true);
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    
    if (EVP_PKEY_keygen(ctx, &key) <= 0) {
        log_this("KeyManager", "Key generation failed", 4, true, true, true);
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    
    EVP_PKEY_CTX_free(ctx);
    return key;
}
```

## Secure Input Validation

For validating security-critical inputs:

1. **Input Validation Strategies**
   - Apply whitelisting (accept only known-good input) rather than blacklisting
   - Validate both syntax (format) and semantics (meaning/context)
   - Consider context-specific validation (e.g., URL validation for redirect URIs)

2. **Common Validation Patterns**
   - Use regular expressions for format validation
   - Use dedicated parsers for structured data (JSON, XML)
   - Implement multi-stage validation for complex inputs

3. **Client-Supplied Data**
   - Apply strict validation to all client-supplied identifiers
   - Sanitize client data before logging or storage
   - Reject requests with suspicious patterns

Example of redirect URI validation:

```c
bool validate_redirect_uri(const char* uri, const char* allowed_origins[], int num_origins) {
    // Check basic URL format
    if (!is_valid_url_format(uri)) {
        log_this("OIDC", "Invalid redirect URI format", 3, true, false, true);
        return false;
    }
    
    // Extract origin from URI
    char origin[256];
    if (!extract_origin(uri, origin, sizeof(origin))) {
        log_this("OIDC", "Failed to extract origin from redirect URI", 3, true, false, true);
        return false;
    }
    
    // Check against allowed origins
    for (int i = 0; i < num_origins; i++) {
        if (strcmp(origin, allowed_origins[i]) == 0) {
            return true;
        }
    }
    
    log_this("OIDC", "Redirect URI origin not allowed", 3, true, false, true);
    return false;
}
```

## Security Error Handling

For error handling in security-sensitive code:

1. **Error Reporting**
   - Provide clear error messages for developers without leaking sensitive information
   - Use different detail levels for logs vs. client-visible errors
   - Include security-relevant context in internal logs

2. **Comprehensive Cleanup**
   - Always free security-sensitive resources on error paths
   - Zero out sensitive memory before freeing
   - Ensure consistent state even after security failures

3. **Secure Defaults**
   - Always default to the secure option when handling errors
   - Deny access rather than grant it when validation is inconclusive
   - Implement timeouts for all security-critical operations

Example of proper security error handling:

```c
int authenticate_client(const char* client_id, const char* client_secret) {
    Client* client = NULL;
    
    // Look up client
    client = find_client_by_id(client_id);
    if (!client) {
        // Don't leak whether the client exists
        log_this("OIDC", "Authentication failed for client ID", 3, true, false, true);
        return AUTH_ERROR_INVALID_CLIENT;
    }
    
    // Validate client secret using constant-time comparison
    if (!secure_compare(client->secret, client_secret, strlen(client_secret))) {
        // Don't leak that the client exists but password is wrong
        log_this("OIDC", "Authentication failed for client ID", 3, true, false, true);
        return AUTH_ERROR_INVALID_CLIENT;
    }
    
    return AUTH_SUCCESS;
}
```

## Security-Focused Documentation

For security-sensitive components:

1. **Security Documentation Requirements**
   - Document security assumptions and preconditions
   - Explain security design decisions and trade-offs
   - Document threat models and mitigations
   - Include security testing procedures

2. **API Security Documentation**
   - Document security requirements for API callers
   - Specify required validation before calling security-sensitive functions
   - Document expected error handling and recovery

Example of security-focused documentation:

```c
/*
 * Validates and processes an authorization request.
 *
 * Security Considerations:
 * - All parameters must be validated before calling this function
 * - The redirect_uri must match one registered for the client
 * - The state parameter must be cryptographically secure
 * - For public clients, PKCE must be enforced (code_challenge required)
 *
 * Threat Mitigations:
 * - CSRF: Enforced through state parameter validation
 * - Code Interception: Mitigated through PKCE and short code lifetime
 * - Redirect URI Manipulation: Strict validation against registered URIs
 *
 * Parameters:
 * - client_id: The registered client identifier
 * - redirect_uri: The URI to redirect to after authorization
 * - response_type: Must be "code" for authorization code flow
 * - scope: Space-separated list of requested scopes
 * - state: Client-provided state for CSRF protection
 * - code_challenge: PKCE code challenge (required for public clients)
 * - code_challenge_method: PKCE challenge method (S256 recommended)
 *
 * Returns:
 * - AUTH_SUCCESS on success
 * - Error code on failure (see oidc_errors.h)
 */
```

## Secure Configuration

For configuration of security-sensitive components:

1. **Configuration Validation**
   - Validate all security-critical configuration parameters at startup
   - Apply secure defaults when configuration is missing or invalid
   - Implement configuration versioning to handle format changes

2. **Sensitive Configuration Data**
   - Store secrets in dedicated secure storage
   - Support environment variable or external secret providers
   - Encrypt sensitive configuration at rest

3. **Configuration Monitoring**
   - Log all security-critical configuration changes
   - Implement validation for configuration updates
   - Provide mechanisms to test configuration before applying

Example of secure configuration validation:

```c
bool validate_oidc_config(OIDCConfig* config) {
    // Check issuer URL
    if (!config->issuer || !is_valid_url(config->issuer)) {
        log_this("OIDC", "Invalid issuer URL in configuration", 4, true, true, true);
        return false;
    }
    
    // Validate token lifetimes
    if (config->access_token_lifetime <= 0 || config->access_token_lifetime > MAX_TOKEN_LIFETIME) {
        log_this("OIDC", "Invalid access token lifetime", 4, true, true, true);
        return false;
    }
    
    // Enforce minimum key size
    if (config->signing_key_size < MIN_RSA_KEY_SIZE) {
        log_this("OIDC", "Insufficient signing key size", 4, true, true, true);
        return false;
    }
    
    return true;
}
```

## Security Code Review Requirements

For reviewing security-sensitive code:

1. **Security Review Process**
   - Require secondary review for all security-critical components
   - Use a security-focused checklist for reviews
   - Document security design decisions and their review

2. **Common Security Review Items**
   - Input validation completeness
   - Authentication and authorization checks
   - Proper cryptographic operations
   - Error handling and edge cases
   - Resource cleanup
   - Logging of security events

3. **Threat Modeling**
   - Consider potential attack vectors during review
   - Verify that known threats are mitigated
   - Document security assumptions

Example security review checklist item:

```list
Cryptographic Operations Review:
- [ ] Uses approved cryptographic libraries
- [ ] No custom cryptographic implementations
- [ ] Appropriate algorithm selection and parameters
- [ ] Secure key management
- [ ] Proper randomness sources
- [ ] No hardcoded cryptographic secrets
```

## Related Documentation

- [API Documentation](/docs/H/core/subsystems/api/api.md)
- [Configuration Guide](/docs/H/core/configuration.md)
- [Service Documentation](/docs/H/core/service.md)
- [OIDC Architecture](/docs/H/core/reference/oidc_architecture.md)
- [Security Best Practices](/docs/H/core/security_best_practices.md)
