# Lithium Testing

This document describes the testing framework, coverage reports, and test writing guide for Lithium.

---

## Overview

Lithium uses **Vitest** as the test runner with **happy-dom** for DOM testing. This provides excellent ES module support.

---

## Running Tests

### Commands

| Command | Description |
|---------|-------------|
| `npm test` | Run all tests (171 tests) |
| `npm run test:watch` | Run tests in watch mode |
| `npm run test:coverage` | Run tests with coverage report |
| `npm run coverage:copy` | Copy coverage to `public/` for deployment |

---

## Test Structure

```structure
tests/
└── unit/                    # Unit tests
    ├── event-bus.test.js    # 6 tests
    ├── jwt.test.js          # 25 tests
    ├── permissions.test.js   # 23 tests
    ├── config.test.js       # 13 tests
    ├── utils.test.js        # 38 tests
    ├── json-request.test.js # 14 tests
    ├── lookups.test.js      # 8 tests
    ├── icons.test.js        # 26 tests
    └── auth.integration.test.js # 12 tests
```

---

## Current Coverage

### Test Results: 171 tests, 0 failures

| Module | Statements | Branch | Functions | Lines |
|--------|-----------|--------|-----------|-------|
| event-bus.js | 100% | 100% | 100% | 100% |
| config.js | 100% | 90% | 100% | 100% |
| permissions.js | 96% | 95% | 100% | 96% |
| utils.js | 88% | 86% | 92% | 88% |
| icons.js | 85% | - | - | - |
| jwt.js | 75% | 73% | 75% | 75% |
| json-request.js | 40% | 31% | 50% | 40% |
| lookups.js | 37% | 18% | 50% | 37% |

### Low Coverage Areas

- **json-request.js (40%)** — ESM mocking complexity with fetch + auth chains
- **lookups.js (37%)** — Fetch + event handling is hard to test in isolation

---

## Coverage Dashboard

### Local

```bash
npm run test:coverage
npm run coverage:copy
```

Then open `coverage/index.html` in a browser.

### Live Dashboard

<https://lithium.philement.com/coverage/>

Features:

- Interactive file tree with coverage percentages
- Line-by-line highlighting (green = covered, red = uncovered)
- Summary statistics
- Sortable tables and search
- Drill-down into individual files

---

## Writing Tests

### Test File Pattern

```javascript
import { describe, it, expect, vi, beforeEach } from 'vitest';

describe('ModuleName', () => {
  beforeEach(() => {
    // Setup
  });

  it('should do something', () => {
    expect(true).toBe(true);
  });
});
```

### Mocking localStorage

```javascript
beforeEach(() => {
  const mockStorage = {};
  vi.stubGlobal('localStorage', {
    getItem: vi.fn((key) => mockStorage[key]),
    setItem: vi.fn((key, value) => { mockStorage[key] = value; }),
    removeItem: vi.fn((key) => { delete mockStorage[key]; }),
    clear: vi.fn(() => { Object.keys(mockStorage).forEach(k => delete mockStorage[k]); }),
  });
});
```

### ESM Mocking

Use `vi.hoisted()` or `vi.mock()` at the top of the file:

```javascript
const { mockFunction } = vi.hoisted(() => ({
  mockFunction: vi.fn(),
}));

vi.mock('./some-module.js', () => ({
  someFunction: mockFunction,
}));
```

---

## Integration Tests

Integration tests run against the live Hydrogen server.

### Environment Variables

```bash
export HYDROGEN_SERVER_URL=https://lithium.philement.com
export HYDROGEN_DEMO_USER_NAME=testuser
export HYDROGEN_DEMO_USER_PASS=usertest
export HYDROGEN_DEMO_API_KEY=EveryGoodBoyDeservesFudge
```

### Running Integration Tests

```bash
npm test -- --run tests/integration/
```

Tests automatically skip with clear message if server or credentials unavailable.

---

## Vitest Configuration

In `vitest.config.js`:

```javascript
import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    environment: 'happy-dom',
    globals: true,
    coverage: {
      provider: 'v8',
      reporter: ['text', 'html'],
      include: ['src/**/*.js'],
    },
  },
});
```

---

## Best Practices

### Test Organization

1. Group tests with `describe()` by module/method
2. Use clear, descriptive test names
3. Follow Arrange-Act-Assert pattern
4. Use `beforeEach()` for common setup

### Code Coverage

1. Aim for 80%+ on core modules
2. Focus on branch coverage
3. Test error paths, not just happy paths
4. Accept lower coverage for HTTP wrappers (complex ESM mocking)

### Avoiding Flaky Tests

1. Don't depend on external services in unit tests
2. Mock time-dependent operations
3. Use consistent test data

---

## Troubleshooting

### Tests Not Running

```bash
# Reinstall dependencies
rm -rf node_modules
npm install
```

### Coverage Not Generating

```bash
# Check vitest config
npx vitest --version

# Run with verbose output
npm run test:coverage -- --reporter=verbose
```

### ESM Import Errors

- Use `vi.hoisted()` for mocking ES modules
- Ensure file extensions are correct (`.js`)
- Check `package.json` has `"type": "module"`

---

## Related Documentation

- Development: [LITHIUM-DEV.md](LITHIUM-DEV.md)
- Libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Managers: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- FAQ: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
