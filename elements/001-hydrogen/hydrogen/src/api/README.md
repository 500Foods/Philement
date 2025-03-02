# Hydrogen REST API Implementation

This directory contains the implementation of Hydrogen's REST API endpoints, which provide HTTP-based access to the system's functionality. The API is organized into service categories, each handling a specific domain of functionality.

## Swagger Documentation

The Hydrogen API uses Swagger (OpenAPI) annotations to provide structured documentation for API endpoints. These annotations are added as special comments (`//@ swagger:`) in the header files of API endpoints. The annotations are processed to generate a comprehensive OpenAPI specification file that documents the entire API.

### Annotation Structure

Swagger annotations follow this pattern:

```c
//@ swagger:service ServiceName
//@ swagger:description Service description goes here
//@ swagger:tag tagname Tag description

//@ swagger:path /path/to/endpoint
//@ swagger:method GET
//@ swagger:operationId operationName
//@ swagger:tags tag1,tag2
//@ swagger:summary Short summary of what this endpoint does
//@ swagger:description Longer description with more details
//@ swagger:response 200 application/json {"type":"object","properties":{...}}
//@ swagger:response 400 application/json {"type":"object","properties":{...}}
```

### Generating Documentation

The Swagger documentation is generated using the `swagger-generate.sh` script in the `swagger` directory. This script processes the annotations in the source code and generates a `swagger.json` file that can be used with Swagger UI or other API documentation tools.

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
    └── test/           # Test endpoint implementation
```

## API Overview

The Hydrogen API is organized into the following service categories:

### System Service

Provides endpoints for system-level operations and status monitoring:

- `/api/system/health` - Health check endpoint for service availability verification
- `/api/system/info` - System information endpoint for retrieving comprehensive system status
- `/api/system/test` - Test endpoint for verifying API functionality

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
4. Results are formatted as JSON and returned to the client

API utilities in `api_utils.c` provide common functionality such as:

- Request parameter extraction and validation
- Response formatting
- Error handling
- Authentication verification

## Adding New Endpoints

To add a new endpoint:

1. Determine which service category it belongs to (or create a new one)
2. Create header and implementation files for the endpoint
3. Define the endpoint URL and handler function in the service header
4. Implement the handler function in the implementation file
5. Register the endpoint in the appropriate service initialization function

See the [Developer Onboarding Guide](../../docs/developer_onboarding.md) for more details on the API implementation pattern.

## Documentation

For comprehensive API documentation, refer to:

- [API Documentation](../../docs/api.md) - Complete API reference
- [System API Documentation](../../docs/api/system/system_info.md) - System endpoint details
- [OIDC API Documentation](../../docs/api/oidc/oidc_endpoints.md) - OIDC endpoint details

## Testing

API endpoints can be tested using:

- The [test_system_endpoints.sh](../../tests/test_system_endpoints.sh) script for system endpoints
- OIDC client examples in the [oidc-client-examples](../../oidc-client-examples/) directory for OIDC endpoints
