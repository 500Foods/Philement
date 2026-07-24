# OIDC Architecture

This document outlines the internal architecture of the OpenID Connect (OIDC)
**Identity Provider** implementation in Hydrogen.

## Implemented subset

Canonical status: [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md).
HTTP surface: [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md).
Operators: [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md).

| Layer | Location | MVP reality |
| --- | --- | --- |
| HTTP handlers | `src/api/oidc/` | Registered when `OIDC.Enabled`; `handle_oidc_request` |
| Core protocol | `src/oidc/oidc_service.c` | authorize issue, token, userinfo, introspect, revoke |
| Keys | `src/oidc/oidc_keys.c` | RSA load/generate, JWKS, rotate |
| Tokens | `src/oidc/oidc_tokens.c` | RS256 JWT access + id_token |
| Clients | `src/oidc/oidc_clients.c` | In-memory registry + config seed |
| Auth codes / refresh | `oidc_auth_codes.c`, `oidc_refresh_tokens.c` | In-memory stores |
| Users | `src/oidc/oidc_users.c` | Stub; login uses auth DB, not this module |
| Crypto helpers | `src/utils/utils_crypto.c` | RS256 sign/verify, base64url, SHA-256 |
| RP (separate) | `src/api/auth/oidc_rp/` | Out of scope here |

Historical text below may still describe future modules (e.g. full user CRUD,
dynamic registration). Prefer the table and plan when they disagree.

## Overview

The Hydrogen OIDC **IdP** authenticates resource owners and issues security
tokens for client applications. The architecture aims to be:

- **Modular**: Composed of discrete, single-responsibility components
- **Secure**: Following OIDC security best practices
- **Flexible**: Supporting multiple authentication flows and configurations
- **Extensible**: Allowing additional functionality with minimal changes
- **Maintainable**: With clear component boundaries and well-defined interfaces

## Component Architecture

The OIDC service is implemented as a layered architecture with the following components:

```diagram
┌───────────────────────────────────────────────────────────────┐
│                   OIDC Service Interface                      │
│   (oidc_service.h/c - Main entry point and lifecycle mgmt)    │
└───────────────┬───────────────┬─────────────────┬─────────────┘
                │               │                 │
┌───────────────▼───┐ ┌─────────▼─────────┐ ┌─────▼─────────────┐
│  Token Service    │ │  Client Registry  │ │ User Management   │
│ (oidc_tokens.h/c) │ │ (oidc_clients.h/c)│ │  (oidc_users.h/c) │
└─────────┬─────────┘ └────────┬──────────┘ └─────────┬─────────┘
          │                    │                      │
          │           ┌────────▼──────────┐           │
          └───────────►   Key Management  ◄───────────┘
                      │  (oidc_keys.h/c)  │
                      └────────┬──────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                      Core Services                          │
│      (Configuration, Logging, Utilities, Storage, etc.)     │
└─────────────────────────────────────────────────────────────┘
```

### 1. OIDC Service Interface

The top-level component (`oidc_service.h/c`) serves as the main interface for the OIDC subsystem and manages:

- Service initialization and shutdown
- Flow coordination
- Configuration validation
- Error handling and logging
- Component lifecycle management

This component exposes public functions to the main application for starting, stopping, and configuring the OIDC service. It handles the complex interdependencies between the various subcomponents.

### 2. Token Service

The Token Service (`oidc_tokens.h/c`) manages all token-related operations:

- Generating JWT access tokens
- Creating and validating ID tokens
- Handling refresh tokens
- Managing token lifetimes and expiration
- Implementing token introspection and revocation

The module enforces token validation rules according to the OIDC specification and maintains appropriate security protections.

### 3. Client Registry

The Client Registry (`oidc_clients.h/c`) manages OAuth client information:

- Storing and retrieving client credentials
- Validating client authentication
- Managing redirect URIs
- Enforcing client-specific policies
- Supporting dynamic client registration

This component is responsible for enforcing security boundaries between client applications and maintaining client-specific configurations.

### 4. User Management

The User Management component (`oidc_users.h/c`) handles user identity and authentication:

- User authentication
- Profile management
- Consent tracking
- Session management
- Claims generation

This module integrates with the existing authentication mechanisms and adds OIDC-specific user functionality.

### 5. Key Management

The Key Management component (`oidc_keys.h/c`) is responsible for:

- RSA key pair generation
- Key rotation and lifecycle
- JSON Web Key Set (JWKS) exposure
- Signature creation and verification
- Secure key storage

This component is critical for the security of the entire OIDC implementation.

## Data Flow

The OIDC service handles several distinct flows based on the OAuth 2.0 and OIDC specifications:

### Authorization Code Flow

1. Client redirects user to authorization endpoint
2. User authenticates and grants consent
3. Authorization code is generated and returned to client
4. Client exchanges code for tokens via token endpoint
5. Client receives access token, refresh token, and ID token

```diagram
┌──────┐              ┌───────┐               ┌──────────────┐
│Client│              │Browser│               │Hydrogen OIDC │
└──┬───┘              └───┬───┘               └───────┬──────┘
   │                      │                           │
   │                      │                           │
   │ Redirect to authorize│                           │
   │─────────────────────>                            │
   │                      │                           │
   │                      │   GET /oauth/authorize    │
   │                      │──────────────────────────>│
   │                      │                           │
   │                      │      Login Form           │
   │                      │<──────────────────────────│
   │                      │                           │
   │                      │   User Login & Consent    │
   │                      │──────────────────────────>│
   │                      │                           │
   │                      │   Code in Redirect        │
   │                      │<──────────────────────────│
   │                      │                           │
   │   Code Returned      │                           │
   │<─────────────────────│                           │
   │                      │                           │
   │   POST /oauth/token  │                           │
   │──────────────────────────────────────────────────>
   │                      │                           │
   │   Tokens Response    │                           │
   │<──────────────────────────────────────────────────
   │                      │                           │
```

### Client Credentials Flow

1. Client directly requests token with its credentials
2. Service validates client authentication
3. Access token is returned to client

```diagram
┌──────┐                              ┌──────────────┐
│Client│                              │Hydrogen OIDC │
└──┬───┘                              └───────┬──────┘
   │                                          │
   │    POST /oauth/token (credentials)       │
   │─────────────────────────────────────────>│
   │                                          │
   │           Access Token Response          │
   │<─────────────────────────────────────────│
   │                                          │
```

## Token Structure

The OIDC service issues several types of tokens:

### Access Token

JWT format with claims:

```json
{
  "iss": "https://hydrogen.example.com",
  "sub": "user123",
  "aud": "client456",
  "exp": 1616174395,
  "iat": 1616170795,
  "jti": "abc123unique",
  "scope": "openid profile"
}
```

### ID Token

JWT format with claims:

```json
{
  "iss": "https://hydrogen.example.com",
  "sub": "user123",
  "aud": "client456",
  "exp": 1616174395,
  "iat": 1616170795,
  "auth_time": 1616170795,
  "nonce": "randomnonce",
  "name": "John Doe",
  "email": "john@example.com"
}
```

### Refresh Token

Opaque token stored in the database with associated metadata.

## Configuration

The OIDC service is configurable through the main Hydrogen configuration system:

```json
{
  "OIDC": {
    "Enabled": true,
    "Issuer": "https://hydrogen.example.com",
    "Endpoints": {
      "Authorization": "/oauth/authorize",
      "Token": "/oauth/token",
      "Userinfo": "/oauth/userinfo",
      "Jwks": "/oauth/jwks"
    },
    "Keys": {
      "RotationIntervalDays": 30,
      "StoragePath": "/var/lib/hydrogen/oidc/keys"
    },
    "Tokens": {
      "AccessTokenLifetime": 3600,
      "RefreshTokenLifetime": 2592000,
      "IdTokenLifetime": 3600
    },
    "Security": {
      "RequirePkce": true,
      "AllowImplicitFlow": false
    }
  }
}
```

## Security Considerations

The OIDC implementation includes several security features:

1. **Key Rotation**: Automatic rotation of signing keys
2. **PKCE Support**: Code challenge for public clients
3. **JWT Validation**: Full validation of all JWT claims
4. **TLS Requirement**: All endpoints require TLS in production
5. **Token Expiration**: Enforced token lifetimes
6. **Secure Storage**: Protected storage for keys and tokens

## Integration Points

The OIDC service integrates with several other Hydrogen components:

1. **Web Server**: For serving OIDC endpoints
2. **Configuration System**: For loading settings
3. **Logging System**: For security and operational events
4. **Storage**: For persisting keys, tokens, and user data

## Design Decisions

Several key design decisions guided the implementation:

1. **Standards Compliance**: Strict adherence to OIDC specifications
2. **Modular Design**: Clear separation between components
3. **Defensive Programming**: Careful validation of all inputs
4. **Minimal Dependencies**: Limited external library usage
5. **Performance Optimization**: Efficient token generation and validation

## Future Enhancements

Potential future enhancements include:

1. **Federation**: Supporting identity federation with other providers
2. **Advanced Flows**: Device code flow and hybrid flow
3. **Multi-factor Authentication**: Adding MFA support
4. **User Management UI**: Web interface for user management
5. **Audit System**: Comprehensive security audit logging

## References

- [OpenID Connect Core 1.0](https://openid.net/specs/openid-connect-core-1_0.html)
- [OAuth 2.0 RFC 6749](https://tools.ietf.org/html/rfc6749)
- [JWT RFC 7519](https://tools.ietf.org/html/rfc7519)
- [PKCE RFC 7636](https://tools.ietf.org/html/rfc7636)
- [JWK RFC 7517](https://tools.ietf.org/html/rfc7517)
