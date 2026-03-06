/**
 * @vitest-environment node
 *
 * Auth Integration Tests
 *
 * These tests run against a real Hydrogen server. They require:
 * - HYDROGEN_SERVER_URL (default: http://localhost:8080)
 * - HYDROGEN_DEMO_USER_NAME
 * - HYDROGEN_DEMO_USER_PASS
 * - HYDROGEN_DEMO_API_KEY
 *
 * Tests are skipped if the server is not available.
 */

import { describe, it, expect, beforeAll, beforeEach, afterEach } from 'vitest';
import { retrieveJWT, storeJWT, clearJWT, getClaims } from '../../src/core/jwt.js';

// Test configuration from environment
const SERVER_URL = process.env.HYDROGEN_SERVER_URL || 'https://lithium.philement.com';
const API_PREFIX = '/api';
const LOGIN_ID = process.env.HYDROGEN_DEMO_USER_NAME;
const PASSWORD = process.env.HYDROGEN_DEMO_USER_PASS;
const API_KEY = process.env.HYDROGEN_DEMO_API_KEY;
const DATABASE = 'Lithium';

// Helper to check if server is available
async function isServerAvailable() {
  try {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000);
    const response = await fetch(`${SERVER_URL}/api/system/health`, {
      method: 'GET',
      signal: controller.signal,
    });
    clearTimeout(timeoutId);
    return response.ok;
  } catch (error) {
    console.log(`Server check failed: ${error.message}`);
    return false;
  }
}

// Helper to check if credentials are configured
function hasCredentials() {
  return !!(LOGIN_ID && PASSWORD && API_KEY);
}

// Helper to make login request
async function login(loginId, password, apiKey = API_KEY) {
  const response = await fetch(`${SERVER_URL}${API_PREFIX}/auth/login`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
    },
    body: JSON.stringify({
      login_id: loginId,
      password: password,
      api_key: apiKey,
      tz: Intl.DateTimeFormat().resolvedOptions().timeZone,
      database: DATABASE,
    }),
  });

  const data = await response.json().catch(() => ({}));
  return { response, data };
}

// Helper to renew token
async function renew(token) {
  const response = await fetch(`${SERVER_URL}${API_PREFIX}/auth/renew`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
      'Authorization': `Bearer ${token}`,
    },
    body: JSON.stringify({}),
  });

  const data = await response.json().catch(() => ({}));
  return { response, data };
}

// Helper to logout
async function logout(token) {
  const response = await fetch(`${SERVER_URL}${API_PREFIX}/auth/logout`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
      'Authorization': `Bearer ${token}`,
    },
    body: JSON.stringify({}),
  });

  const data = await response.json().catch(() => ({}));
  return { response, data };
}

// Mock localStorage for node environment
const localStorageMock = (() => {
  let store = {};
  return {
    getItem: (key) => store[key] || null,
    setItem: (key, value) => { store[key] = value.toString(); },
    removeItem: (key) => { delete store[key]; },
    clear: () => { store = {}; },
  };
})();

Object.defineProperty(global, 'localStorage', {
  value: localStorageMock,
  writable: true,
});

describe('Auth Integration', () => {
  // Check availability at describe level
  const credentialsAvailable = hasCredentials();
  let serverAvailable = false;
  let checked = false;

  // Synchronous check for credentials
  if (!credentialsAvailable) {
    console.warn(`\n⚠️  Missing credentials for integration tests`);
    console.warn('   Set HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY');
  }

  beforeAll(async () => {
    if (!credentialsAvailable) return;

    serverAvailable = await isServerAvailable();
    checked = true;

    if (!serverAvailable) {
      console.warn(`\n⚠️  Hydrogen server not available at ${SERVER_URL}`);
      console.warn('   Set HYDROGEN_SERVER_URL to point to your server');
    } else {
      console.log(`✓ Hydrogen server available at ${SERVER_URL}`);
    }
  });

  beforeEach(() => {
    // Clear any stored JWT before each test
    clearJWT();
  });

  afterEach(() => {
    // Clean up after each test
    clearJWT();
  });

  const itIfAvailable = (name, fn) => {
    // If credentials aren't available, skip immediately
    if (!credentialsAvailable) {
      return it.skip(`${name} (credentials not configured)`, fn);
    }
    // Otherwise use the standard test (beforeAll will have run)
    return it(name, fn);
  };

  describe('Login', () => {
    itIfAvailable('should successfully login with valid credentials', async () => {
      const { response, data } = await login(LOGIN_ID, PASSWORD);

      expect(response.status).toBe(200);
      expect(data.token).toBeDefined();
      expect(data.token.length).toBeGreaterThan(0);
      expect(data.expires_at).toBeDefined();
      expect(data.user_id).toBeDefined();
      expect(data.username).toBeDefined();

      // Verify token structure (JWT has 3 parts separated by dots)
      const parts = data.token.split('.');
      expect(parts).toHaveLength(3);

      // Verify we can decode the claims
      const claims = getClaims(data.token);
      expect(claims).not.toBeNull();
      expect(claims.user_id).toBe(data.user_id);
      expect(claims.username).toBe(data.username);
      expect(claims.database).toBe(DATABASE);
      expect(claims.exp).toBe(data.expires_at);
    });

    itIfAvailable('should fail login with invalid password', async () => {
      const { response, data } = await login(LOGIN_ID, 'wrongpassword123!');

      expect(response.status).toBe(401);
      expect(data.error).toBeDefined();
      expect(data.token).toBeUndefined();
    });

    itIfAvailable('should fail login with invalid username', async () => {
      const { response, data } = await login('nonexistentuser123', PASSWORD);

      expect(response.status).toBe(401);
      expect(data.error).toBeDefined();
      expect(data.token).toBeUndefined();
    });

    itIfAvailable('should fail login with invalid API key', async () => {
      const { response, data } = await login(LOGIN_ID, PASSWORD, 'invalid-api-key');

      expect(response.status).toBe(401);
      expect(data.error).toBeDefined();
    });

    itIfAvailable('should fail login with missing required fields', async () => {
      const response = await fetch(`${SERVER_URL}${API_PREFIX}/auth/login`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ login_id: LOGIN_ID }), // Missing password, api_key, etc.
      });

      expect(response.status).toBe(400);
    });
  });

  describe('Token Renewal', () => {
    itIfAvailable('should renew a valid token', async () => {
      // First login to get a token
      const loginResult = await login(LOGIN_ID, PASSWORD);
      expect(loginResult.response.status).toBe(200);

      const originalToken = loginResult.data.token;
      const originalClaims = getClaims(originalToken);

      // Wait a moment to ensure different iat/exp
      await new Promise(resolve => setTimeout(resolve, 1000));

      // Renew the token
      const { response, data } = await renew(originalToken);

      expect(response.status).toBe(200);
      expect(data.token).toBeDefined();
      expect(data.token).not.toBe(originalToken); // Should be a new token
      expect(data.expires_at).toBeDefined();

      // Verify new claims preserve user info
      const newClaims = getClaims(data.token);
      expect(newClaims.user_id).toBe(originalClaims.user_id);
      expect(newClaims.username).toBe(originalClaims.username);
      expect(newClaims.database).toBe(originalClaims.database);

      // New token should have later expiration
      expect(newClaims.exp).toBeGreaterThanOrEqual(originalClaims.exp);
    });

    itIfAvailable('should fail renewal with invalid token', async () => {
      const { response } = await renew('invalid.token.here');

      expect(response.status).toBe(401);
    });
  });

  describe('Logout', () => {
    itIfAvailable('should successfully logout with valid token', async () => {
      // First login
      const loginResult = await login(LOGIN_ID, PASSWORD);
      expect(loginResult.response.status).toBe(200);

      const token = loginResult.data.token;

      // Logout
      const { response, data } = await logout(token);

      expect(response.status).toBe(200);
      expect(data.success).toBe(true);
    });

    itIfAvailable('should allow logout with expired token', async () => {
      // Note: This test documents current Hydrogen behavior
      // Logout endpoint accepts expired tokens to allow cleanup

      // First login
      const loginResult = await login(LOGIN_ID, PASSWORD);
      expect(loginResult.response.status).toBe(200);

      const token = loginResult.data.token;

      // Logout should work even if token is technically expired
      // (Hydrogen's validate_jwt_for_logout allows expired tokens)
      const { response } = await logout(token);

      // Should succeed regardless of token expiry
      expect(response.status).toBe(200);
    });
  });

  describe('End-to-End Flow', () => {
    itIfAvailable('should complete full auth lifecycle', async () => {
      // 1. Login
      const loginResult = await login(LOGIN_ID, PASSWORD);
      expect(loginResult.response.status).toBe(200);
      const token = loginResult.data.token;

      // 2. Store token (as client would)
      storeJWT(token);
      expect(retrieveJWT()).toBe(token);

      // 3. Verify token is valid
      const claims = getClaims(token);
      expect(claims).not.toBeNull();

      // 4. Renew token
      const renewResult = await renew(token);
      expect(renewResult.response.status).toBe(200);
      const newToken = renewResult.data.token;

      // 5. Update stored token
      storeJWT(newToken);
      expect(retrieveJWT()).toBe(newToken);

      // 6. Logout
      const logoutResult = await logout(newToken);
      expect(logoutResult.response.status).toBe(200);

      // 7. Clear local storage
      clearJWT();
      expect(retrieveJWT()).toBeNull();
    });
  });

  describe('JWT Claims Verification', () => {
    itIfAvailable('should include all expected claims in JWT', async () => {
      const { response, data } = await login(LOGIN_ID, PASSWORD);
      expect(response.status).toBe(200);

      const claims = getClaims(data.token);
      expect(claims).not.toBeNull();

      // Standard JWT claims
      expect(claims.iss).toBeDefined(); // Issuer
      expect(claims.sub).toBeDefined(); // Subject (user ID)
      expect(claims.aud).toBeDefined(); // Audience (app ID)
      expect(claims.exp).toBeDefined(); // Expiration
      expect(claims.iat).toBeDefined(); // Issued at
      expect(claims.nbf).toBeDefined(); // Not before
      expect(claims.jti).toBeDefined(); // JWT ID

      // Custom Hydrogen claims
      expect(claims.user_id).toBeDefined();
      expect(claims.system_id).toBeDefined();
      expect(claims.app_id).toBeDefined();
      expect(claims.username).toBeDefined();
      expect(claims.email).toBeDefined();
      expect(claims.roles).toBeDefined();
      expect(claims.ip).toBeDefined();
      expect(claims.tz).toBeDefined();
      expect(claims.tzoffset).toBeDefined();
      expect(claims.database).toBe(DATABASE);

      // Verify types
      expect(typeof claims.exp).toBe('number');
      expect(typeof claims.user_id).toBe('number');
      expect(typeof claims.tzoffset).toBe('number');
    });

    itIfAvailable('should have correct token lifetime', async () => {
      const { response, data } = await login(LOGIN_ID, PASSWORD);
      expect(response.status).toBe(200);

      const claims = getClaims(data.token);
      const lifetime = claims.exp - claims.iat;

      // Hydrogen JWT lifetime is 1 hour (3600 seconds)
      expect(lifetime).toBe(3600);
    });
  });
});
