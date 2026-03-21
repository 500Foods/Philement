# JWT Authentication

JWT (JSON Web Token) authentication in Lithium for secure stateless authentication with the Hydrogen backend.

**Location:** `src/core/jwt.js`

---

## Overview

Lithium uses JWT for secure, stateless authentication. Tokens are issued by the Hydrogen backend upon successful login and included in subsequent API requests. The JWT module provides functions for storing, retrieving, and validating tokens, as well as extracting claims for authorization decisions.

---

## Token Lifecycle

### Generation

JWTs are generated server-side by Hydrogen after successful authentication. The token is returned in the login response:

```javascript
// Login response from Hydrogen
{
  "success": true,
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjoxMjMsInVzZXJuYW1lIjoidGVzdCIsInJvbGVzIjpbImFkbWluIl0sImV4cCI6MTcwNjc2NjAwMH0.signature"
}
```

The token consists of three parts separated by dots:

- **Header** - Algorithm and token type
- **Payload** - Claims (user data)
- **Signature** - Verification hash

### Storage

Tokens are stored in the browser's `localStorage` for persistence across sessions:

```javascript
import { storeJWT } from '../../core/jwt.js';

// Store token after successful login
storeJWT(response.token);
```

> **Security Note:** localStorage is vulnerable to XSS attacks. Consider HttpOnly cookies for enhanced security in high-risk environments.

### Retrieval

Tokens are retrieved for inclusion in API requests:

```javascript
import { getJWT } from '../../core/jwt.js';

// Get token for API calls
const token = getJWT();

// Token is automatically injected by json-request.js when auth: true
```

### Validation

Token validation occurs on two levels:

#### 1. Expiration Check (Client-side)

```javascript
import { isTokenExpired } from '../../core/jwt.js';

// Check if token is expired
if (isTokenExpired()) {
  // Trigger renewal or logout
  handleTokenExpiration();
}
```

#### 2. Server-side Validation

Every authenticated API request validates the token server-side. Invalid or expired tokens result in a 401 response, triggering automatic logout.

---

## Token Renewal

Tokens should be renewed before expiration to maintain continuous sessions.

### Renewal Strategy

```javascript
import { getJWT, isTokenExpired } from '../../core/jwt.js';

// Check token validity before each request
function ensureValidToken() {
  if (!getJWT() || isTokenExpired()) {
    // Redirect to login or attempt refresh
    window.location.href = '/login';
  }
}
```

### Activity-Triggered Renewal

Lithium implements activity-triggered token renewal to keep sessions alive. When a user opens a manager (menu or utility), the app records the activity and attempts to renew the token:

```javascript
// From app.js - triggered on manager open
async _renewOnActivity() {
  if (this._isRenewing) return;

  // Don't renew tokens that were just obtained (within 10s)
  const tokenAge = Date.now() - this._tokenScheduledAt;
  if (tokenAge < 10000) return;

  const token = retrieveJWT();
  if (!token) return;
  const validation = validateJWT(token);
  if (!validation.valid) return;

  try {
    await this.renewToken();
  } catch (error) {
    logAuth(Status.WARN, `Activity-triggered renewal failed: ${error.message}`);
  }
}
```

The renewal request must include an empty JSON object `{}` as the body, otherwise Hydrogen returns a "Request body is empty" error.

### Refresh Token Flow

When a token expires, the user is redirected to login. For seamless renewal, implement a refresh token mechanism:

1. Store a refresh token alongside the JWT
2. On expiration, use the refresh token to obtain a new JWT
3. Update stored JWT without requiring credentials

---

## Claims

The JWT payload contains claims about the authenticated user. Lithium uses specific claims for authorization and user identification.

### Standard Claims

| Claim | Type | Description |
|-------|------|-------------|
| `user_id` | number | Unique user identifier |
| `username` | string | User's login name |
| `email` | string | User's email address |
| `roles` | array | User's roles (e.g., `['admin', 'user']`) |
| `database` | string | Target database name |
| `exp` | number | Expiration time (Unix timestamp) |
| `iat` | number | Issued at time (Unix timestamp) |
| `iss` | string | Token issuer |
| `sub` | string | Subject (user ID) |
| `aud` | string | Audience (app ID) |
| `jti` | string | JWT ID (unique identifier) |
| `ip` | string | Client IP address |
| `tz` | string | Timezone name |
| `tzoffset` | number | Timezone offset (minutes) |
| `system_id` | number | System identifier |
| `app_id` | number | Application identifier |

### Example Claims

```javascript
import { getClaims } from '../../core/jwt.js';

const claims = getClaims();
// {
//   user_id: 123,
//   username: 'testuser',
//   email: 'test@example.com',
//   roles: ['admin', 'manager'],
//   database: 'Lithium',
//   exp: 1706766000,
//   iat: 1706700000,
//   iss: 'hydrogen',
//   sub: '123',
//   aud: 'lithium',
//   jti: 'abc123-def456',
//   ip: '192.168.1.1',
//   tz: 'America/Vancouver',
//   tzoffset: -420,
//   system_id: 1,
//   app_id: 1
// }
```

### Using Claims

```javascript
import { getClaims } from '../../core/jwt.js';

// Get user ID for API requests
const userId = getClaims().user_id;

// Check user roles for authorization
const claims = getClaims();
if (claims.roles.includes('admin')) {
  // Grant admin access
}

// Check if token is still valid
const isExpired = !getClaims() || getClaims().exp * 1000 < Date.now();
```

---

## JWT Functions

| Function | Purpose |
|----------|---------|
| `storeJWT(token)` | Store JWT in localStorage |
| `getJWT()` | Retrieve JWT from localStorage |
| `removeJWT()` | Remove JWT from localStorage (logout) |
| `getClaims()` | Parse JWT payload and return claims object |
| `isTokenExpired()` | Check if token expiration claim indicates expiry |

---

## Usage Examples

### Login Flow

```javascript
import { jsonRequest } from '../../core/json-request.js';
import { storeJWT } from '../../core/jwt.js';

async function login(username, password) {
  const response = await jsonRequest('/api/login', {
    method: 'POST',
    body: { username, password },
    auth: false // Don't include JWT in login request
  });

  if (response.success) {
    storeJWT(response.token);
    return true;
  }
  return false;
}
```

### Authenticated Request

```javascript
import { jsonRequest } from '../../core/json-request.js';

// JWT is automatically included
const data = await jsonRequest('/api/protected-data');
```

### Logout

```javascript
import { removeJWT } from '../../core/jwt.js';
import { eventBus, Events } from '../../core/event-bus.js';

function logout() {
  removeJWT();
  eventBus.emit(Events.AUTH_LOGOUT);
  window.location.href = '/login';
}
```

### Role-Based Access

```javascript
import { getClaims } from '../../core/jwt.js';

function canAccessAdminPanel() {
  const claims = getClaims();
  return claims && claims.roles.includes('admin');
}
```

---

## Integration with json-request.js

The [`json-request.js`](elements/003-lithium/src/core/json-request.js) module automatically includes JWT tokens in authenticated requests:

```javascript
import { jsonRequest } from '../../core/json-request.js';

// JWT automatically included (default auth: true)
const data = await jsonRequest('/api/data');

// Explicitly disable auth for public endpoints
const publicData = await jsonRequest('/api/public', { auth: false });
```

---

## Security Considerations

1. **Token Storage:** Tokens in localStorage are accessible via JavaScript, making them vulnerable to XSS. Consider HttpOnly cookies for sensitive applications.

2. **Token Expiration:** Set reasonable expiration times. Too long increases risk; too short disrupts user experience.

3. **HTTPS Only:** Always transmit tokens over HTTPS to prevent interception.

4. **Token Revocation:** Implement server-side token blacklisting for immediate invalidation when needed.

---

## Related Documentation

- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) - Login Manager implementation
- [LITHIUM-API.md](LITHIUM-API.md) - Hydrogen API endpoints
- [LITHIUM-OTH.md](LITHIUM-OTH.md) - Other utilities (transitions, utils, JSON, log, permissions)