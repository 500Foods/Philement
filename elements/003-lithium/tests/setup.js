// Test setup file - adds polyfills for happy-dom environment

// Add atob/btoa polyfills for Node.js/Vitest environment
if (typeof global.atob === 'undefined') {
  global.atob = (str) => Buffer.from(str, 'base64').toString('binary');
}

if (typeof global.btoa === 'undefined') {
  global.btoa = (str) => Buffer.from(str, 'binary').toString('base64');
}
