# Hydrogen OIDC Client Examples

This directory contains example code demonstrating how to integrate client applications with Hydrogen's OpenID Connect (OIDC) Identity Provider functionality.

## Directory Structure

- **C/** - Native C examples for server-side implementations
  - `auth_code_flow.c` - Authorization Code flow with PKCE
  - `client_credentials.c` - Client Credentials flow
  - `password_flow.c` - Resource Owner Password flow
- **JavaScript/** - Browser-based implementations
  - `auth_code_flow.html` - Authorization Code flow with PKCE for browsers

## Overview

These examples demonstrate the various OAuth 2.0/OIDC flows supported by Hydrogen:

1. **Authorization Code Flow** - The most common and secure flow for applications with a server-side component
2. **Client Credentials Flow** - For service-to-service authentication without user involvement
3. **Resource Owner Password Flow** - For trusted first-party applications (legacy support)
4. **Implicit Flow** - For browser-based JavaScript applications (when PKCE is not available)

## Prerequisites

- Running Hydrogen service with OIDC functionality enabled
- Client application registered with the Hydrogen OIDC service
- Client credentials (client_id and client_secret)
- For C examples:
  - GCC or compatible C compiler
  - libcurl (HTTP client library)
  - libmicrohttpd (for callback server in Authorization Code flow)
  - OpenSSL (for cryptographic operations)
  - Jansson (for JSON parsing)

## Client Registration

Before using these examples, you need to register your client application with the Hydrogen OIDC service:

```bash
# Using Hydrogen's client registration endpoint
curl -X POST https://hydrogen.example.com/oauth/register \
  -H "Content-Type: application/json" \
  -d '{
    "client_name": "Example Client",
    "redirect_uris": ["https://client.example.com/callback"],
    "grant_types": ["authorization_code", "refresh_token"],
    "response_types": ["code"],
    "token_endpoint_auth_method": "client_secret_basic"
  }'
```

The exact configuration will depend on which flow you're using:

| Flow Type | Required Grant Types | Required Response Types | Redirect URIs Required? |
|-----------|---------------------|------------------------|-------------------------|
| Authorization Code | `authorization_code`, `refresh_token` | `code` | Yes |
| Client Credentials | `client_credentials` | None | No |
| Password | `password`, `refresh_token` | None | No |
| Implicit | `implicit` | `token id_token` | Yes |

## C Examples

### Compiling C Examples

The C examples can be built using the provided Makefile:

```bash
# Navigate to the C examples directory
cd C

# Build all examples
make all

# Build a specific example
make auth_code    # Build the Authorization Code flow example
make client_cred  # Build the Client Credentials flow example
make password     # Build the Resource Owner Password flow example

# Build with debug symbols
make debug

# Clean build artifacts
make clean

# Display help information
make help
```

Alternatively, you can compile each example manually with GCC or a compatible C compiler:

**Authorization Code Flow:**

```bash
gcc -o auth_code_flow auth_code_flow.c -lcurl -lcrypto -lmicrohttpd -ljansson
```

**Client Credentials Flow:**

```bash
gcc -o client_credentials client_credentials.c -lcurl -ljansson
```

**Resource Owner Password Flow:**

```bash
gcc -o password_flow password_flow.c -lcurl -ljansson
```

### Authorization Code Flow (C Implementation)

The most common and secure flow for applications with a server-side component:

1. Application generates PKCE code verifier and code challenge
2. User is redirected to Hydrogen's authorization endpoint
3. User authenticates and grants consent
4. User is redirected back to the application with an authorization code
5. Application exchanges the authorization code for tokens using its client credentials and the code verifier
6. Application validates the received tokens
7. Application uses the access token to access protected resources
8. When the access token expires, the refresh token can be used to get a new one

**Features of our implementation:**

- PKCE (Proof Key for Code Exchange) for enhanced security
- State parameter for CSRF protection
- Proper token validation
- Refresh token handling
- Organized error handling

**Usage:**

```bash
./auth_code_flow
```

The application will:

1. Print a URL to visit in your browser
2. Start a callback server on localhost
3. Wait for the authorization code from the OIDC provider
4. Exchange the code for tokens
5. Validate the tokens
6. Make a sample API call with the access token
7. Demonstrate token refresh

### Client Credentials Flow (C Implementation)

Used for service-to-service authentication without user involvement:

1. Service requests an access token using its client credentials
2. Service validates the received token
3. Service uses the access token to access protected resources

**Features of our implementation:**

- Secure client authentication
- Token validation
- Structured error handling
- JSON parsing of responses

**Usage:**

```bash
./client_credentials
```

The application will:

1. Request an access token using client credentials
2. Validate the received token
3. Make a sample API call with the access token

### Resource Owner Password Flow (C Implementation)

Used for trusted first-party applications where the application directly collects the user's credentials:

1. User provides username and password directly to the application
2. Application requests an access token using these credentials and its client credentials
3. Application validates the received tokens
4. Application uses the access token to access protected resources
5. When the access token expires, the refresh token can be used to get a new one

**Features of our implementation:**

- Secure handling of user credentials
- Token validation
- Refresh token support
- Structured error handling
- JSON parsing of responses

**Usage:**

```bash
./password_flow <username> <password>
```

The application will:

1. Request tokens using the provided credentials
2. Validate the received tokens
3. Make a sample API call with the access token
4. Demonstrate token refresh if a refresh token was provided

## JavaScript Examples

Our JavaScript examples are single-page HTML applications that can be opened directly in a browser. They use modern browser features and don't require any server-side components.

### Authorization Code Flow with PKCE (JavaScript Implementation)

This example implements the Authorization Code flow with PKCE for browser-based applications:

**Features:**

- PKCE for enhanced security
- State parameter for CSRF protection
- Secure token storage in browser session storage
- Token expiration monitoring
- Clean UI with status indicators

**Usage:**

1. Open `auth_code_flow.html` in a browser
2. Update the configuration settings (endpoints, client ID, etc.)
3. Click "Start Authorization Flow"
4. Complete the authentication on the Hydrogen OIDC provider
5. On return, tokens will be displayed and can be used to make API calls

## Security Considerations

When using these examples in your own applications, keep these security best practices in mind:

### All Flows

- Always use HTTPS in production environments
- Validate all tokens before trusting them
- Check token expiration times and handle renewal properly
- Validate the `iss` (issuer) claim to prevent token confusion attacks
- For ID tokens, validate the `aud` (audience) claim matches your client ID
- Consider token revocation for logout scenarios

### Authorization Code Flow

- Always use PKCE, even for confidential clients
- Validate the state parameter to prevent CSRF attacks
- Store the code verifier securely
- Use short-lived authorization codes (Hydrogen enforces this server-side)

### Client Credentials Flow

- Protect client secrets carefully
- Use restricted scopes appropriate for the service
- Consider client certificate authentication for higher security

### Resource Owner Password Flow

- Only use for first-party, highly trusted applications
- Transmit credentials securely
- Never store raw passwords
- Consider migrating to Authorization Code flow if possible

### Implicit Flow

- Avoid if possible in favor of Authorization Code flow with PKCE
- Be aware that access tokens are exposed in the URL
- Use short-lived access tokens
- Implement proper state validation

## Token Handling

### Access Token

JWT format with claims like:

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

JWT format with user identity claims:

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

Opaque token used to obtain new access tokens when they expire.

## Best Practices for Production Use

When adapting these examples for production:

1. **Implement proper error handling and logging**
   - Log all authentication errors
   - Implement retry logic with backoff for transient errors
   - Provide clear error messages to users

2. **Secure token storage**
   - Server-side: Store tokens in secure, encrypted storage
   - Browser: Use secure mechanisms like HttpOnly cookies or session storage
   - Mobile: Use secure storage APIs provided by the platform

3. **Implement proper token lifecycle management**
   - Monitor token expiration
   - Implement background renewal for refresh tokens
   - Handle revocation and logout properly

4. **Add security headers**
   - Content-Security-Policy
   - X-Frame-Options
   - X-XSS-Protection

5. **Consider token binding for additional security**
   - Bind tokens to TLS sessions or device information where appropriate

## Customizing the Examples

To use these examples with your Hydrogen instance:

1. Update all endpoint URLs to match your Hydrogen deployment
2. Replace the client ID and client secret with your registered values
3. Modify redirect URIs to match your application's callback endpoint
4. Adjust scopes based on your application's needs
5. Update protected resource URLs to point to your APIs

## Further Reading

For more information on OAuth 2.0 and OpenID Connect:

- [OAuth 2.0 RFC 6749](https://tools.ietf.org/html/rfc6749)
- [OpenID Connect Core 1.0](https://openid.net/specs/openid-connect-core-1_0.html)
- [OAuth 2.0 Security Best Practices](https://tools.ietf.org/html/draft-ietf-oauth-security-topics)
- [OAuth 2.0 for Browser-Based Apps](https://datatracker.ietf.org/doc/html/draft-ietf-oauth-browser-based-apps)
- [OAuth 2.0 PKCE RFC 7636](https://tools.ietf.org/html/rfc7636)

## Troubleshooting

### Common Issues

1. **"No token received" or authorization errors**
   - Check client ID and secret
   - Verify redirect URI matches exactly (including trailing slashes)
   - Ensure requested scopes are allowed for your client

2. **"Invalid token" errors when calling APIs**
   - Check if token has expired
   - Verify you're sending the token correctly (Bearer prefix)
   - Ensure your client has permission for the resource

3. **Callback failures**
   - Check if server is running and port is open
   - Verify the redirect URI matches the registered one exactly

4. **Compilation errors (C examples)**
   - Install all required dependencies (libcurl, libmicrohttpd, etc.)
   - Check that header files are in expected locations
   - Consider using pkg-config to find libraries
