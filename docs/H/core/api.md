# Hydrogen API Documentation

This document serves as a table of contents for all available API endpoints in the Hydrogen Project server.

## Security Overview

All sensitive endpoints use encryption and secure key management as documented in [SECRETS.md](SECRETS.md). For implementation details of the encryption systems, see:

- [Payload Encryption](/payloads/README.md)
- [OIDC Security](./oidc_integration.md)
- [Network Security](./reference/network_architecture.md)

## System Service

Endpoints for system-level operations and monitoring:

- [System Information](./api/system/system_info.md) - GET `/api/system/info`  
  Comprehensive system status and information
  
- [Health Check](./api/system/system_health.md) - GET `/api/system/health`  
  Service availability verification
  
- [Version Information](./api/system/system_version.md) - GET `/api/version`  
  API and server version details

## OIDC Service

Endpoints for OpenID Connect (OIDC) authentication and identity management. All OIDC endpoints use RSA key pairs for signing and AES encryption for sensitive data. See [SECRETS.md](SECRETS.md) for key management details.

- [Authorization Endpoint](./api/oidc/oidc_endpoints.md#authorization) - GET/POST `/oauth/authorize`  
  Initiates authentication flows and obtains user consent  
  *Uses TLS encryption*

- [Token Endpoint](./api/oidc/oidc_endpoints.md#token) - POST `/oauth/token`  
  Issues access tokens, ID tokens, and refresh tokens  
  *Uses RSA signing and AES encryption*
  
- [UserInfo Endpoint](./api/oidc/oidc_endpoints.md#userinfo) - GET `/oauth/userinfo`  
  Provides authenticated user profile information
  
- [JSON Web Key Set](./api/oidc/oidc_endpoints.md#jwks) - GET `/oauth/jwks`  
  Publishes public keys for token validation  
  *RSA public keys for token verification*

- [Token Introspection](./api/oidc/oidc_endpoints.md#introspection) - POST `/oauth/introspect`  
  Verifies token validity and provides token metadata  
  *Uses RSA verification*
  
- [Token Revocation](./api/oidc/oidc_endpoints.md#revocation) - POST `/oauth/revoke`  
  Invalidates issued tokens
  
- [Client Registration](./api/oidc/oidc_endpoints.md#registration) - POST `/oauth/register`  
  Registers client applications with the OIDC service
  
- [Discovery Document](./api/oidc/oidc_endpoints.md#discovery) - GET `/.well-known/openid-configuration`  
  Provides service metadata and endpoint URLs

## Additional Resources

- [Configuration Guide](./configuration.md) - Complete guide for server configuration
- [WebSocket API](./web_socket.md) - Real-time communication interface
- [Print Queue Management](./print_queue.md) - Print job scheduling and management
- [OIDC Integration](./oidc_integration.md) - Identity Provider functionality
