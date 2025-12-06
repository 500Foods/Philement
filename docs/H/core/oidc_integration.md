# OpenID Connect (OIDC) Integration

This document outlines the architecture and implementation plan for adding OpenID Connect (OIDC) functionality to Hydrogen, enabling it to function as an Identity Provider (IdP).

## Overview

The OIDC integration will allow Hydrogen to:

1. Function as a standalone Identity Provider (IdP)
2. Serve as an umbrella authentication service for the organization
3. Support flexible deployment where instances can serve as IdPs for different web applications

This functionality will provide a secure, standards-compliant authentication and authorization system that can be used across the organization's services and applications.

## Architecture

The OIDC implementation will follow Hydrogen's existing architectural patterns while adding new components specific to identity management and authentication flows.

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     Application Layer                         │
│                                                               │
│   ┌───────────┐   ┌───────────┐   ┌───────────┐   ┌────────┐  │
│   │ Print     │   │ Web       │   │ WebSocket │   │ OIDC   │  │
│   │ Queue     │   │ Server    │   │ Server    │   │ Service│  │
│   └───────────┘   └───────────┘   └───────────┘   └────────┘  │
│                                                               │
└───────────────────────────────────────────────────────────────┘
                          │               
                          ▼               
                ┌─────────────────────────┐
                │    Identity Services    │
                │  ┌─────────────────────┐│
                │  │- User Management    ││
                │  │- Client Registry    ││
                │  │- Token Service      ││
                │  │- Key Management     ││
                │  └─────────────────────┘│
                └─────────────────────────┘
```

### Key Components

#### 1. OIDC Service

The core service handling OpenID Connect flows and endpoints:

- Coordinates authentication and authorization processes
- Manages OIDC protocol flows (Authorization Code, Implicit, Client Credentials, etc.)
- Implements the required OIDC endpoints (authorization, token, userinfo, etc.)
- Validates requests and enforces security policies

#### 2. User Management

Manages user identities and authentication:

- User creation, retrieval, updating, and deletion
- Password management and validation
- User profile information
- Role and permission assignment

#### 3. Client Registry

Manages OIDC client applications:

- Client registration and configuration
- Client authentication
- Redirect URI validation
- Scope and permission management

#### 4. Token Service

Handles JWT token operations:

- Token generation (ID tokens, access tokens, refresh tokens)
- Token validation and verification
- Token revocation
- Token introspection

#### 5. Key Management

Manages cryptographic keys for OIDC operations:

- RSA key pair generation for token signing
- Key rotation and versioning
- JWKS (JSON Web Key Set) endpoint implementation
- Secure key storage

### Integration with Existing Components

The OIDC functionality will integrate with Hydrogen's existing components:

- **Web Server**: Extended to handle OIDC-specific endpoints
- **Configuration System**: Enhanced with OIDC-specific configuration options
- **Logging System**: Utilized for security logging and audit trails
- **State Management**: Integrated for proper startup/shutdown sequencing

## OIDC Endpoints

The following endpoints will be implemented:

| Endpoint | Purpose |
|----------|---------|
| `/.well-known/openid-configuration` | OIDC discovery document |
| `/oauth/authorize` | Authorization endpoint for initiating authentication flows |
| `/oauth/token` | Token endpoint for obtaining tokens |
| `/oauth/userinfo` | UserInfo endpoint for retrieving authenticated user information |
| `/oauth/jwks` | JSON Web Key Set endpoint for public keys |
| `/oauth/introspect` | Token introspection endpoint |
| `/oauth/revoke` | Token revocation endpoint |
| `/oauth/register` | Dynamic client registration endpoint |
| `/oauth/clients` | Client management API |
| `/oauth/users` | User management API |

## Data Flow

### Authentication Flow (Authorization Code)

```diagram
┌─────────┐          ┌────────────┐          ┌────────────┐          ┌────────────┐
│  User   │          │  Client    │          │  Hydrogen  │          │  Resource  │
│ Browser │          │ Application│          │  OIDC IdP  │          │  Server    │
└────┬────┘          └─────┬──────┘          └─────┬──────┘          └─────┬──────┘
     │                     │                       │                       │
     │                     │                       │                       │
     │                     │ Authorization Request │                       │
     │                     │──────────────────────►│                       │
     │                     │                       │                       │
     │ Authentication and  │                       │                       │
     │ Consent Redirect    │                       │                       │
     │◄────────────────────┼───────────────────────┤                       │
     │                     │                       │                       │
     │ User Authenticates  │                       │                       │
     │ and Grants Consent  │                       │                       │
     │─────────────────────┼───────────────────────►                       │
     │                     │                       │                       │
     │                     │ Authorization Code    │                       │
     │                     │◄──────────────────────┤                       │
     │                     │                       │                       │
     │                     │ Token Request         │                       │
     │                     │ (with auth code)      │                       │
     │                     │──────────────────────►│                       │
     │                     │                       │                       │
     │                     │ ID Token +            │                       │
     │                     │ Access Token          │                       │
     │                     │◄──────────────────────┤                       │
     │                     │                       │                       │
     │                     │ Resource Request      │                       │
     │                     │ (with access token)   │                       │
     │                     │──────────────────────────────────────────────►│
     │                     │                       │                       │
     │                     │ Protected Resource    │                       │
     │                     │◄──────────────────────────────────────────────┤
     │                     │                       │                       │
```

### Token Validation Flow

```diagram
┌────────────┐          ┌────────────┐          ┌────────────┐
│  Client    │          │  Resource  │          │  Hydrogen  │
│ Application│          │  Server    │          │  OIDC IdP  │
└─────┬──────┘          └─────┬──────┘          └─────┬──────┘
      │                       │                       │
      │                       │                       │
      │ API Request           │                       │
      │ (with access token)   │                       │
      │──────────────────────►│                       │
      │                       │                       │
      │                       │ Token Introspection   │
      │                       │ Request               │
      │                       │──────────────────────►│
      │                       │                       │
      │                       │ Token Validation      │
      │                       │ Response              │
      │                       │◄──────────────────────┤
      │                       │                       │
      │ API Response          │                       │
      │◄──────────────────────┤                       │
      │                       │                       │
```

## Implementation Plan

### Phase 1: Core OIDC Infrastructure

1. Create key management system:
   - Implement RSA key pair generation
   - Add key rotation mechanisms
   - Develop JWKS endpoint

2. Build token service:
   - Implement JWT token creation
   - Add token validation mechanisms
   - Create token storage and retrieval

3. Add basic configuration options:
   - OIDC service settings
   - Signing key configuration
   - Endpoint URIs

### Phase 2: User and Client Management

1. Implement user management:
   - User data storage
   - User authentication
   - User profile management

2. Create client registry:
   - Client data storage
   - Client authentication
   - Client configuration management

### Phase 3: OIDC Protocol Implementation

1. Implement OIDC endpoints:
   - Discovery endpoint
   - Authorization endpoint
   - Token endpoint
   - UserInfo endpoint
   - Introspection endpoint
   - Revocation endpoint

2. Implement OIDC flows:
   - Authorization Code flow
   - Implicit flow
   - Client Credentials flow
   - Refresh Token flow

3. Add security features:
   - PKCE support
   - Request validation
   - Scope enforcement
   - Rate limiting

### Phase 4: User Interface and Integration

1. Create user interfaces:
   - Login page
   - Consent page
   - User profile page
   - Client management pages

2. Integrate with existing Hydrogen components:
   - Web server routing
   - Logging system
   - WebSocket notifications
   - Startup/shutdown processes

### Phase 5: Testing and Documentation

1. Develop comprehensive tests:
   - Unit tests for all components
   - Integration tests for OIDC flows
   - Security testing
   - Conformance testing

2. Create documentation:
   - Implementation guide
   - API reference
   - Configuration reference
   - Security best practices
   - Client integration guide

## File Structure

```files
src/
├── oidc/                       # OIDC implementation
│   ├── oidc_service.c          # Core OIDC service
│   ├── oidc_service.h          # Public OIDC interface
│   ├── oidc_endpoints.c        # OIDC endpoint handlers
│   ├── oidc_endpoints.h        # Endpoint definitions
│   ├── oidc_keys.c             # Key management
│   ├── oidc_keys.h             # Key interfaces
│   ├── oidc_tokens.c           # Token service
│   ├── oidc_tokens.h           # Token interfaces
│   ├── oidc_users.c            # User management
│   ├── oidc_users.h            # User interfaces
│   ├── oidc_clients.c          # Client registry
│   ├── oidc_clients.h          # Client interfaces
│   ├── oidc_data.c             # Data storage
│   ├── oidc_data.h             # Data interfaces
│   ├── oidc_utils.c            # Utility functions
│   └── oidc_utils.h            # Utility interfaces
├── webserver/
│   ├── web_server_oidc.c       # OIDC-specific web handlers
│   └── web_server_oidc.h       # OIDC web interfaces
```

## Configuration Options

The OIDC functionality will be configured through the standard Hydrogen configuration file:

```json
{
  "oidc": {
    "enabled": true,
    "issuer": "https://hydrogen.example.com",
    "endpoints": {
      "authorization": "/oauth/authorize",
      "token": "/oauth/token",
      "userinfo": "/oauth/userinfo",
      "jwks": "/oauth/jwks",
      "introspection": "/oauth/introspect",
      "revocation": "/oauth/revoke"
    },
    "keys": {
      "rotation_interval_days": 30,
      "storage_path": "/path/to/keys",
      "encryption_enabled": true
    },
    "tokens": {
      "access_token_lifetime_seconds": 3600,
      "refresh_token_lifetime_seconds": 86400,
      "id_token_lifetime_seconds": 3600
    },
    "security": {
      "require_pkce": true,
      "allow_implicit_flow": false,
      "allow_client_credentials": true,
      "require_consent": true
    }
  }
}
```

## External Dependencies

The OIDC implementation will require the following dependencies:

- **OpenSSL**: For cryptographic operations
- **Jansson**: For JSON processing
- **SQLite or similar**: For data storage (user, client, token databases)

## Documentation Plan

A comprehensive set of documentation will be created:

1. **OIDC Overview**: Introduction to OIDC concepts and Hydrogen's implementation
2. **Installation Guide**: Step-by-step setup instructions
3. **Configuration Reference**: Detailed configuration options
4. **API Reference**: Endpoint descriptions and usage examples
5. **Client Integration Guide**: How to integrate applications with Hydrogen OIDC
6. **Security Considerations**: Best practices for secure deployment
7. **Troubleshooting Guide**: Common issues and solutions
8. **Development Guide**: How to extend or customize the OIDC functionality

## Libraries

Two additional libraries will be developed:

1. **hydrogen-oidc-client**: Client library for applications integrating with Hydrogen as an IdP
   - Authentication helpers
   - Token management
   - API wrappers

2. **hydrogen-oidc-admin**: Administration library for managing the OIDC service
   - User management
   - Client registration
   - Settings configuration
   - Monitoring tools

## Conclusion

The OIDC integration will significantly enhance Hydrogen's capabilities, allowing it to serve as a secure identity provider for the organization's services. By following this implementation plan, we can ensure a standards-compliant, secure, and flexible OIDC solution that meets the requirements specified.

The modular design will allow Hydrogen to function as either a standalone IdP or as part of a larger authentication ecosystem, providing the flexibility needed to support various deployment scenarios.
