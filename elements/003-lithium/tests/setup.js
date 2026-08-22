// Test setup file - adds polyfills for happy-dom environment
import { vi } from 'vitest';

// Add atob/btoa polyfills for Node.js/Vitest environment
if (typeof global.atob === 'undefined') {
  global.atob = (str) => Buffer.from(str, 'base64').toString('binary');
}

if (typeof global.btoa === 'undefined') {
  global.btoa = (str) => Buffer.from(str, 'binary').toString('base64');
}

// Unit tests run in happy-dom. Default config is http://localhost:8080;
// an unmocked fetch there becomes an unhandled ECONNREFUSED. Integration
// tests use the node environment and keep the real fetch.
if (typeof window !== 'undefined') {
  vi.stubGlobal('fetch', vi.fn(async () => new Response('{}', {
    status: 200,
    headers: { 'content-type': 'application/json' },
  })));
}
