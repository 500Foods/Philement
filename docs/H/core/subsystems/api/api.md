# Hydrogen API Documentation

This document serves as a table of contents for all available API endpoints in the Hydrogen Project server.

## Security Overview

All sensitive endpoints use encryption and secure key management as documented in [SECRETS.md](/docs/H/SECRETS.md). For implementation details of the encryption systems, see:

- [Payload Encryption](/elements/001-hydrogen/hydrogen/payloads/README.md)
- [OIDC Security](/docs/H/core/subsystems/oidc/oidc.md)
- [Network Security](/docs/H/core/reference/network_architecture.md)

## System Service

Endpoints for system-level operations and monitoring:

- [System Information](/docs/H/api/system/system_info.md) - GET `/api/system/info`  
  Comprehensive system status and information
  
- [Health Check](/docs/H/api/system/system_health.md) - GET `/api/system/health`  
  Service availability verification
  
- [Version Information](/docs/H/api/system/system_version.md) - GET `/api/version`  
  API and server version details

## OIDC Service

Endpoints for OpenID Connect (OIDC) authentication and identity management. All OIDC endpoints use RSA key pairs for signing and AES encryption for sensitive data. See [SECRETS.md](/docs/H/SECRETS.md) for key management details.

- [Authorization Endpoint](/docs/H/api/oidc/oidc_endpoints.md#authorization) - GET/POST `/oauth/authorize`  
  Initiates authentication flows and obtains user consent  
  *Uses TLS encryption*

- [Token Endpoint](/docs/H/api/oidc/oidc_endpoints.md#token) - POST `/oauth/token`  
  Issues access tokens, ID tokens, and refresh tokens  
  *Uses RSA signing and AES encryption*
  
- [UserInfo Endpoint](/docs/H/api/oidc/oidc_endpoints.md#userinfo) - GET `/oauth/userinfo`  
  Provides authenticated user profile information
  
- [JSON Web Key Set](/docs/H/api/oidc/oidc_endpoints.md#jwks) - GET `/oauth/jwks`  
  Publishes public keys for token validation  
  *RSA public keys for token verification*

- [Token Introspection](/docs/H/api/oidc/oidc_endpoints.md#introspection) - POST `/oauth/introspect`  
  Verifies token validity and provides token metadata  
  *Uses RSA verification*
  
- [Token Revocation](/docs/H/api/oidc/oidc_endpoints.md#revocation) - POST `/oauth/revoke`  
  Invalidates issued tokens
  
- [Client Registration](/docs/H/api/oidc/oidc_endpoints.md#registration) - POST `/oauth/register`  
  Registers client applications with the OIDC service
  
- [Discovery Document](/docs/H/api/oidc/oidc_endpoints.md#discovery) - GET `/.well-known/openid-configuration`  
  Provides service metadata and endpoint URLs

## Additional Resources

- [Configuration Guide](/docs/H/core/configuration.md) - Complete guide for server configuration
- [WebSocket API](/docs/H/core/subsystems/websocket/websocket.md) - Real-time communication interface
- [Print Queue Management](/docs/H/core/subsystems/print/print.md) - Print job scheduling and management
- [OIDC Integration](/docs/H/core/subsystems/oidc/oidc.md) - Identity Provider functionality
