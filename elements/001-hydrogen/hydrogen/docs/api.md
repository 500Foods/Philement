# Hydrogen API Documentation

This document serves as a table of contents for all available API endpoints in the Hydrogen Project server.

## System Service

Endpoints for system-level operations and monitoring:

- [System Information](./api/system/system_info.md) - GET `/api/system/info`  
  Comprehensive system status and information
  
- [Health Check](./api/system/system_health.md) - GET `/api/system/health`  
  Service availability verification
  
- [Version Information](./api/system/system_version.md) - GET `/api/version`  
  API and server version details

## OIDC Service

Endpoints for OpenID Connect (OIDC) authentication and identity management:

- [Authorization Endpoint](./api/oidc/oidc_endpoints.md#authorization) - GET/POST `/oauth/authorize`  
  Initiates authentication flows and obtains user consent
  
- [Token Endpoint](./api/oidc/oidc_endpoints.md#token) - POST `/oauth/token`  
  Issues access tokens, ID tokens, and refresh tokens
  
- [UserInfo Endpoint](./api/oidc/oidc_endpoints.md#userinfo) - GET `/oauth/userinfo`  
  Provides authenticated user profile information
  
- [JSON Web Key Set](./api/oidc/oidc_endpoints.md#jwks) - GET `/oauth/jwks`  
  Publishes public keys for token validation
  
- [Token Introspection](./api/oidc/oidc_endpoints.md#introspection) - POST `/oauth/introspect`  
  Verifies token validity and provides token metadata
  
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
