# Hydrogen REST API Implementation

This directory contains the implementation of Hydrogen's REST API endpoints, which provide HTTP-based access to the system's functionality. The API is organized into service categories, each handling a specific domain of functionality.

## Swagger Documentation

The Hydrogen API uses Swagger (OpenAPI) annotations to provide structured documentation for API endpoints. These annotations are added as special comments (`//@ swagger:`) in the header files of API endpoints. The annotations are processed to generate a comprehensive OpenAPI specification file that documents the entire API.

### Annotation Structure

Swagger annotations are organized at two levels:

#### Service Level (in service header files like `system_service.h`)

```c
//@ swagger:tag "System Service" Provides system-level operations and monitoring
```

#### Endpoint Level (in endpoint header files)

```c
//@ swagger:path /system/info
//@ swagger:method GET
//@ swagger:summary System information endpoint
//@ swagger:description Returns comprehensive system information
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string"}}}
```

The `operationId` field is optional and can be added if needed:

```c
//@ swagger:operationId getSystemInfo
```

### Generating Documentation

The Swagger documentation is generated using the `swagger-generate.sh` script in the `payloads` directory. This script processes the annotations in the source code and generates a `swagger.json` file that can be used with Swagger UI or other API documentation tools.

## Directory Structure

```directories
api/
├── api_utils.c         # Shared utility functions for API implementations
├── api_utils.h         # Header file for API utilities
├── oidc/               # OpenID Connect authentication endpoints
│   ├── oidc_endpoints.c # OIDC endpoint implementations
│   └── oidc_endpoints.h # OIDC endpoint declarations
└── system/             # System-level API endpoints
    ├── system_service.c # Core system service implementation
    ├── system_service.h # System service interface definitions
    ├── health/         # Health check endpoint implementation
    ├── info/           # System information endpoint implementation
    ├── test/           # Test endpoint implementation
    └── config/         # Configuration endpoint implementation
```

## API Overview

The Hydrogen API is organized into the following service categories:

### System Service

Provides endpoints for system-level operations and status monitoring:

- `/api/system/health` - Health check endpoint for service availability verification
- `/api/system/info` - System information endpoint for retrieving comprehensive system status
- `/api/system/test` - Test endpoint for verifying API functionality
- `/api/system/config` - Configuration endpoint for retrieving server configuration (brotli compressed)

### OIDC Service

Implements the OpenID Connect authentication protocol with endpoints for identity management:

- `/oauth/authorize` - Authorization endpoint for initiating authentication flows
- `/oauth/token` - Token endpoint for issuing access, ID, and refresh tokens
- `/oauth/userinfo` - UserInfo endpoint for retrieving authenticated user information
- `/oauth/jwks` - JSON Web Key Set endpoint for publishing public keys
- `/oauth/introspect` - Token introspection endpoint for verifying token validity
- `/oauth/revoke` - Token revocation endpoint for invalidating tokens
- `/oauth/register` - Client registration endpoint for registering OIDC clients
- `/.well-known/openid-configuration` - Discovery document endpoint for OIDC metadata

## Implementation Details

Each API endpoint follows a consistent implementation pattern:

1. Endpoint handlers are registered with the web server during initialization
2. Handlers validate request parameters and authorization
3. The requested operation is performed
4. Results are formatted as JSON and returned using api_send_json_response

API utilities in `api_utils.c` provide common functionality such as:

- `api_send_json_response(connection, json_obj, status_code)`: Sends a JSON response with automatic brotli compression when supported by the client
- `api_extract_query_params(connection)`: Extracts and parses URL query parameters
- `api_extract_post_data(connection)`: Extracts and parses POST form data
- `api_get_client_ip(connection)`: Gets the client's IP address
- `api_extract_jwt_claims(connection, jwt_secret)`: Extracts and validates JWT claims
- `api_validate_jwt(token, secret)`: Validates a JWT token
- `api_create_jwt(claims, secret)`: Creates a new JWT token

## Adding New Endpoints

To add a new endpoint:

1. Determine which service category it belongs to (or create a new one)
2. Create header and implementation files for the endpoint
3. Define the endpoint URL and handler function in the service header
4. Implement the handler function in the implementation file
5. Register the endpoint in the appropriate service initialization function

See the [Developer Onboarding Guide](/docs/H/core/developer_onboarding.md) for more details on the API implementation pattern. For information about the payload system and Swagger integration, see [Payloads README](/elements/001-hydrogen/hydrogen/payloads/README.md).

## Documentation

For comprehensive API documentation, refer to:

- [API Documentation](/docs/H/core/subsystems/api/api.md) - Complete API reference

## Testing

API endpoints can be tested using:

### Example API Call

To quickly test the `/api/system/health` endpoint, you can use `curl` from the command line:

```bash
curl -X GET "http://your-hydrogen-server:port/api/system/health"
```

This should return a JSON response indicating the health status of the system.

## Versioning

The Hydrogen API currently does not implement explicit versioning. All endpoints are considered part of the current stable release unless otherwise noted. Check the [RELEASES.md](/RELEASES.md) for information on API changes in each release.

## Tests

- The [test_32_system_endpoints.sh](/elements/001-hydrogen/hydrogen/tests/test_21_system_endpoints.sh) script for system endpoints
