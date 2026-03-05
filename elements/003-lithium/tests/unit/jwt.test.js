/**
 * JWT Unit Tests
 */

// Pure JavaScript atob/btoa polyfills for happy-dom environment
// (doesn't depend on Node.js Buffer or browser APIs)
function base64ToBytes(str) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';
  let output = '';
  
  str = str.replace(/=+$/, '');
  
  for (let bc = 0, bs = 0, buffer, i = 0; (buffer = str.charAt(i++)); ~buffer && (bs = bc % 4 ? bs * 64 + buffer : buffer, bc++ % 4) ? output += String.fromCharCode(255 & bs >> (-2 * bc & 6)) : 0) {
    buffer = chars.indexOf(buffer);
  }
  
  return output;
}

function bytesToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';
  let output = '';
  
  for (let block = 0, charCode, i = 0, map = chars; bytes.charAt(i | 0) || (map = '=', i % 1); output += map.charAt(63 & block >> 8 - i % 1 * 8)) {
    charCode = bytes.charCodeAt(i += 3 / 4);
    if (charCode > 0xFF) throw new Error("'btoa' failed: The string contains characters outside of the Latin1 range.");
    block = block << 8 | charCode;
  }
  
  return output;
}

if (typeof globalThis.atob === 'undefined') {
  globalThis.atob = base64ToBytes;
}
if (typeof globalThis.btoa === 'undefined') {
  globalThis.btoa = bytesToBase64;
}

import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import {
  decodeJWT,
  isExpired,
  validateJWT,
  getClaims,
  storeJWT,
  retrieveJWT,
  clearJWT,
  getTimeUntilExpiry,
  getRenewalTime,
} from '../../src/core/jwt.js';

const STORAGE_KEY = 'lithium_jwt';

describe('JWT', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  afterEach(() => {
    // No cleanup needed
  });

  describe('decodeJWT', () => {
    it('should decode a valid JWT token', () => {
      // Token with payload: {"user_id": 42, "username": "testuser", "exp": 1893456000}
      // exp is far in the future (2030-01-01)
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0MiwidXNlcm5hbWUiOiJ0ZXN0dXNlciIsImV4cCI6MTg5MzQ1NjAwMH0.signature';
      
      const decoded = decodeJWT(token);
      
      expect(decoded.user_id).toBe(42);
      expect(decoded.username).toBe('testuser');
      expect(decoded.exp).toBe(1893456000);
    });

    it('should throw error for invalid token format', () => {
      expect(() => decodeJWT('invalid')).toThrow('Invalid JWT format');
      expect(() => decodeJWT('part1.part2')).toThrow('Invalid JWT format');
    });

    it('should throw error for empty token', () => {
      expect(() => decodeJWT('')).toThrow('Token is required');
      expect(() => decodeJWT(null)).toThrow('Token is required');
    });
  });

  describe('isExpired', () => {
    it('should return false for token that is not expired', () => {
      // Token expiring in 1 hour from now (far in the future: 2030-01-01)
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE4OTM0NTYwMDB9.signature';
      
      expect(isExpired(token)).toBe(false);
    });

    it('should return true for expired token', () => {
      // Token that expired in the past (2020-01-01)
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE1Nzc4MzY4MDB9.signature';
      
      expect(isExpired(token)).toBe(true);
    });

    it('should treat token as expired when within clock skew window', () => {
      // isExpired returns true when now >= (exp - clockSkew)
      // With default clockSkew=60, a token expiring in 30s is "expired"
      vi.useFakeTimers();
      const fakeNow = 1900000000; // arbitrary fixed timestamp
      vi.setSystemTime(new Date(fakeNow * 1000));
      
      // Token expiring in 30 seconds (within the 60s skew window)
      const soonPayload = btoa(JSON.stringify({ exp: fakeNow + 30 }))
        .replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
      const soonToken = `header.${soonPayload}.signature`;
      
      // With default 60s skew: now (1900000000) >= exp - 60 (1900000030 - 60 = 1899999970) → true
      expect(isExpired(soonToken)).toBe(true);
      
      // Token expiring in 120 seconds (well outside the 60s skew window)
      const laterPayload = btoa(JSON.stringify({ exp: fakeNow + 120 }))
        .replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
      const laterToken = `header.${laterPayload}.signature`;
      
      // With default 60s skew: now (1900000000) >= exp - 60 (1900000120 - 60 = 1900000060) → false
      expect(isExpired(laterToken)).toBe(false);
      
      // With explicit 0s skew, the 30s-out token should NOT be expired
      expect(isExpired(soonToken, 0)).toBe(false);
      
      vi.useRealTimers();
    });

    it('should return false for token without exp claim', () => {
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0Mn0.signature';
      
      expect(isExpired(token)).toBe(false);
    });

    it('should return true for invalid token', () => {
      expect(isExpired('invalid')).toBe(true);
    });
  });

  describe('validateJWT', () => {
    it('should validate a non-expired token', () => {
      // Token with user_id: 42, exp: 2030-01-01
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0MiwiZXhwIjoxODkzNDU2MDAwfQ.signature';
      
      const result = validateJWT(token);
      
      expect(result.valid).toBe(true);
      expect(result.claims.user_id).toBe(42);
    });

    it('should invalidate an expired token', () => {
      // Token expired 2020-01-01
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE1Nzc4MzY4MDB9.signature';
      
      const result = validateJWT(token);
      
      expect(result.valid).toBe(false);
      expect(result.error).toBe('Token expired');
    });

    it('should handle empty token', () => {
      const result = validateJWT('');
      
      expect(result.valid).toBe(false);
      expect(result.error).toBe('Token is required');
    });

    it('should handle malformed token', () => {
      const result = validateJWT('invalid.token');
      
      expect(result.valid).toBe(false);
      expect(result.error).toContain('Invalid JWT format');
    });
  });

  describe('getClaims', () => {
    it('should return claims for valid token', () => {
      // Token with user_id: 42, roles: ["admin"], exp: 2030-01-01
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0Miwicm9sZXMiOlsiYWRtaW4iXSwiZXhwIjoxODkzNDU2MDAwfQ.signature';
      
      const claims = getClaims(token);
      
      expect(claims.user_id).toBe(42);
      expect(claims.roles).toEqual(['admin']);
    });

    it('should return null for invalid token', () => {
      const claims = getClaims('invalid');
      
      expect(claims).toBeNull();
    });
  });

  describe('storeJWT and retrieveJWT', () => {
    it('should store and retrieve JWT', () => {
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0Mn0.signature';
      
      storeJWT(token);
      const retrieved = retrieveJWT();
      
      expect(retrieved).toBe(token);
    });

    it('should return null when no token stored', () => {
      const retrieved = retrieveJWT();
      
      expect(retrieved).toBeNull();
    });

    it('should throw error for empty token', () => {
      expect(() => storeJWT('')).toThrow('Token is required');
      expect(() => storeJWT(null)).toThrow('Token is required');
    });
  });

  describe('clearJWT', () => {
    it('should remove stored JWT', () => {
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0Mn0.signature';
      storeJWT(token);
      
      clearJWT();
      const retrieved = retrieveJWT();
      
      expect(retrieved).toBeNull();
    });
  });

  describe('getTimeUntilExpiry', () => {
    it('should return positive time for future expiry', () => {
      // Token expiring 2030-01-01 (far in the future)
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE4OTM0NTYwMDB9.signature';
      
      const timeUntil = getTimeUntilExpiry(token);
      
      expect(timeUntil).toBeGreaterThan(0);
    });

    it('should return negative time for past expiry', () => {
      // Token expired 2020-01-01
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE1Nzc4MzY4MDB9.signature';
      
      const timeUntil = getTimeUntilExpiry(token);
      
      expect(timeUntil).toBeLessThan(0);
    });

    it('should return Infinity for token without exp', () => {
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0Mn0.signature';
      
      const timeUntil = getTimeUntilExpiry(token);
      
      expect(timeUntil).toBe(Infinity);
    });

    it('should return -1 for invalid token', () => {
      const timeUntil = getTimeUntilExpiry('invalid');
      
      expect(timeUntil).toBe(-1);
    });
  });

  describe('getRenewalTime', () => {
    it('should return calculated renewal time for valid token', () => {
      // Token expiring in ~100 seconds from a fixed timestamp
      // Using a relative approach: exp is current time + 100
      const now = Math.floor(Date.now() / 1000);
      const customClaims = { exp: now + 100 };
      // Encode payload manually for this specific case
      // eyJleHAiOjE3OTE5MjAwMTB9 is {"exp":1791920010} - adjust based on mock time
      const payload = btoa(JSON.stringify(customClaims)).replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
      const token = `header.${payload}.signature`;
      
      const renewalTime = getRenewalTime(token);
      
      // Should be approximately 80 seconds (80% of 100)
      expect(renewalTime).toBeGreaterThanOrEqual(75000);
      expect(renewalTime).toBeLessThanOrEqual(80000);
    });

    it('should return 0 for already expired token', () => {
      // Token expired in the past
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE1Nzc4MzY4MDB9.signature';
      
      const renewalTime = getRenewalTime(token);
      
      expect(renewalTime).toBe(0);
    });

    it('should return Infinity for token without exp', () => {
      const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX2lkIjo0Mn0.signature';
      
      const renewalTime = getRenewalTime(token);
      
      expect(renewalTime).toBe(Infinity);
    });
  });
});
