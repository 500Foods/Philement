/**
 * JWT Utilities - Client-side JWT handling
 * 
 * Provides functions for decoding, validating, and extracting claims
 * from JWT tokens. Note: This does NOT verify signatures (server-side only).
 */

const STORAGE_KEY = 'lithium_jwt';

/**
 * Base64Url decode a string - works in browser and Node.js/test environments
 * @param {string} base64Url - Base64Url encoded string
 * @returns {string} Decoded string
 */
function base64UrlDecode(base64Url) {
  // Replace Base64Url characters with Base64 characters
  let base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
  
  // Add padding if necessary
  const padding = base64.length % 4;
  if (padding) {
    base64 += '='.repeat(4 - padding);
  }
  
  // Use atob if available (browser), otherwise use a fallback
  if (typeof atob !== 'undefined') {
    try {
      // Decode and handle UTF-8 properly
      const decoded = atob(base64);
      // Convert from binary string to UTF-8
      return decodeURIComponent(decoded.split('').map(c => 
        '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2)
      ).join(''));
    } catch (_e) {
    throw new Error('Invalid Base64Url encoding', { cause: _e });
    }
  }
  
  // Fallback: use Buffer in Node.js or throw error
  if (typeof Buffer !== 'undefined') {
    return Buffer.from(base64, 'base64').toString('utf-8');
  }
  
  throw new Error('No Base64 decoder available');
}

/**
 * Decode a JWT token without verifying signature
 * @param {string} token - JWT token string
 * @returns {Object} Decoded payload
 */
export function decodeJWT(token) {
  if (!token || typeof token !== 'string') {
    throw new Error('Token is required');
  }

  const parts = token.split('.');
  if (parts.length !== 3) {
    throw new Error('Invalid JWT format');
  }

  try {
    const payload = base64UrlDecode(parts[1]);
    return JSON.parse(payload);
  } catch (_e) {
    throw new Error('Failed to decode JWT payload: ' + _e.message, { cause: _e });
  }
}

/**
 * Check if a JWT token is expired
 * @param {string|Object} token - JWT token string or decoded payload
 * @param {number} [clockSkew=60] - Clock skew tolerance in seconds
 * @returns {boolean} True if expired
 */
export function isExpired(token, clockSkew = 60) {
  try {
    const claims = typeof token === 'string' ? decodeJWT(token) : token;
    
    if (!claims.exp || typeof claims.exp !== 'number') {
      return false; // No expiry = never expires
    }

    const now = Math.floor(Date.now() / 1000);
    return now >= (claims.exp - clockSkew);
  } catch (_err) {
    return true; // Invalid token = treat as expired
  }
}

/**
 * Validate a JWT token (format and expiry only, not signature)
 * @param {string} token - JWT token string
 * @returns {Object} Validation result
 */
export function validateJWT(token) {
  if (!token || typeof token !== 'string') {
    return { valid: false, error: 'Token is required' };
  }

  try {
    const claims = decodeJWT(token);
    
    if (isExpired(claims)) {
      return { valid: false, error: 'Token expired', claims };
    }

    return { valid: true, claims };
  } catch (_e) {
    return { valid: false, error: _e.message };
  }
}

/**
 * Get claims from a JWT token
 * @param {string} token - JWT token string
 * @returns {Object|null} Claims object or null if invalid
 */
export function getClaims(token) {
  try {
    return decodeJWT(token);
  } catch (_e) {
    return null;
  }
}

/**
 * Store JWT in localStorage
 * @param {string} token - JWT token string
 */
export function storeJWT(token) {
  if (!token) {
    throw new Error('Token is required');
  }
  localStorage.setItem(STORAGE_KEY, token);
}

/**
 * Retrieve JWT from localStorage
 * @returns {string|null} JWT token or null
 */
export function retrieveJWT() {
  return localStorage.getItem(STORAGE_KEY);
}

/**
 * Clear JWT from localStorage
 */
export function clearJWT() {
  localStorage.removeItem(STORAGE_KEY);
}

/**
 * Get time until token expires in milliseconds
 * @param {string|Object} token - JWT token string or decoded payload
 * @returns {number} Milliseconds until expiry (negative if expired)
 */
export function getTimeUntilExpiry(token) {
  try {
    const claims = typeof token === 'string' ? decodeJWT(token) : token;
    
    if (!claims.exp || typeof claims.exp !== 'number') {
      return Infinity;
    }

    const now = Math.floor(Date.now() / 1000);
    return (claims.exp - now) * 1000;
  } catch (_e) {
    return -1;
  }
}

/**
 * Calculate optimal renewal time (80% of token lifetime)
 * @param {string|Object} token - JWT token string or decoded payload
 * @returns {number} Milliseconds until renewal should occur
 */
export function getRenewalTime(token) {
  const timeUntilExpiry = getTimeUntilExpiry(token);
  
  if (timeUntilExpiry === Infinity) {
    return Infinity;
  }
  
  if (timeUntilExpiry <= 0) {
    return 0;
  }

  // Renew at 80% of lifetime
  return Math.floor(timeUntilExpiry * 0.8);
}

// Default export for convenience
export default {
  decode: decodeJWT,
  isExpired,
  validate: validateJWT,
  getClaims,
  store: storeJWT,
  retrieve: retrieveJWT,
  clear: clearJWT,
  getTimeUntilExpiry,
  getRenewalTime,
};
