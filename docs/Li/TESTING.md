# Lithium Testing Framework

## Overview

Lithium uses a comprehensive testing approach inspired by the Hydrogen project,
featuring Mocha as the test framework, Chai for assertions, and NYC (Istanbul)
for code coverage. This document provides detailed information about the testing
infrastructure.

## Table of Contents

- [**Testing Philosophy**](#-testing-philosophy)
- [**Test Structure**](#-test-structure)
- [**Test Types**](#-test-types)
- [**Running Tests**](#-running-tests)
- [**Code Coverage**](#-code-coverage)
- [**Best Practices**](#-best-practices)
- [**Continuous Integration**](#-continuous-integration)
- [**Troubleshooting**](#-troubleshooting)

## 🎯 Testing Philosophy

Lithium follows these testing principles:

1. **Test-Driven Development**: Write tests before implementation
2. **Comprehensive Coverage**: Aim for 80%+ code coverage
3. **Isolation**: Tests should be independent
4. **Deterministic**: Tests should produce consistent results
5. **Fast Execution**: Tests should run quickly
6. **Clear Documentation**: Tests should document behavior

## 📁 Test Structure

```text
tests/
├── package.json            # Test dependencies and configuration
├── test-runner.js          # Test orchestration and reporting
└── tests/                  # Test files organized by module
    ├── app.test.js         # Main application tests
    ├── router.test.js      # Router module tests
    ├── storage.test.js     # Storage module tests
    ├── network.test.js     # Network module tests
    ├── utils.test.js       # Utility function tests
    └── ...                 # Additional test files
```

## 🧪 Test Types

### Unit Tests

Test individual functions and methods in isolation:

```javascript
import { expect } from 'chai';
import { Logger } from '../../src/utils/logger.js';

describe('Logger', () => {
  let logger;

  beforeEach(() => {
    logger = new Logger('Test');
  });

  describe('log()', () => {
    it('should log messages at different levels', () => {
      // Test implementation
    });
  });
});
```

### Integration Tests

Test interactions between modules:

```javascript
import { expect } from 'chai';
import { Router } from '../../src/router/router.js';
import { Storage } from '../../src/storage/storage.js';

describe('Router + Storage Integration', () => {
  let router, storage;

  beforeEach(() => {
    router = new Router();
    storage = new Storage();
    // Set up integration
  });

  it('should handle route changes with storage updates', () => {
    // Test integration
  });
});
```

### Component Tests

Test UI components and behavior:

```javascript
import { expect } from 'chai';
import { App } from '../../src/app.js';

describe('App Component', () => {
  let app;

  beforeEach(() => {
    // Set up DOM
    document.body.innerHTML = '<div id="app"></div>';
    app = new App();
  });

  it('should render the main application', () => {
    // Test rendering
  });
});
```

### End-to-End Tests

Test complete user flows (coming soon with Cypress integration):

```javascript
// E2E tests will be added when Cypress is integrated
describe('User Authentication Flow', () => {
  it('should allow users to login and access protected routes', () => {
    // E2E test implementation
  });
});
```

## 🚀 Running Tests

### Basic Commands

```bash
# Run all tests once
npm test

# Run tests with watch mode (auto-rerun on changes)
npm run test:watch

# Run tests with coverage analysis
npm run test:coverage

# Generate coverage reports
npm run test:report
```

### Test Runner

The `test-runner.js` provides additional functionality:

```bash
# Run complete test suite with reports
cd tests && node test-runner.js

# Run specific test files
mocha tests/router.test.js

# Run tests with specific reporter
mocha --reporter spec
mocha --reporter nyan
```

### Test Options

```bash
# Run tests with grep filter
mocha --grep "Router"

# Run tests with timeout
mocha --timeout 10000

# Run tests with bail on first failure
mocha --bail
```

## ⚙️ Test Configuration

### package.json Configuration

```json
{
  "scripts": {
    "test": "mocha --recursive --exit",
    "test:watch": "mocha --watch --recursive",
    "test:coverage": "nyc --reporter=html --reporter=text mocha --recursive --exit",
    "test:report": "nyc report --reporter=lcov"
  },
  "nyc": {
    "include": [
      "../src/**/*.js"
    ],
    "exclude": [
      "**/tests/**",
      "**/dist/**",
      "**/node_modules/**"
    ],
    "reporter": [
      "html",
      "text",
      "lcov"
    ],
    "all": true,
    "check-coverage": true,
    "per-file": true,
    "statements": 80,
    "branches": 80,
    "functions": 80,
    "lines": 80
  },
  "mocha": {
    "require": [
      "@babel/register"
    ],
    "spec": "tests/**/*.test.js",
    "timeout": 5000,
    "reporter": "spec",
    "ui": "bdd"
  }
}
```

### Configuration Options

- **include**: Files to include in coverage analysis
- **exclude**: Files to exclude from coverage
- **reporter**: Coverage report formats
- **check-coverage**: Enforce minimum coverage thresholds
- **per-file**: Show coverage per file
- **statements/branches/functions/lines**: Minimum coverage percentages

## 📊 Code Coverage

### Coverage Thresholds

Lithium enforces minimum coverage requirements:

- **Statements**: 80% minimum
- **Branches**: 80% minimum
- **Functions**: 80% minimum
- **Lines**: 80% minimum

### Coverage Reports

Three report formats are generated:

1. **HTML Report**: Interactive report in `tests/coverage/`
   - Open `tests/coverage/index.html` in browser
   - Detailed file-by-file coverage
   - Line-by-line coverage visualization

2. **Text Report**: Console output
   - Summary of overall coverage
   - Per-file coverage statistics
   - Color-coded results

3. **LCov Report**: For CI/CD integration
   - Standard coverage format
   - Can be uploaded to coverage services
   - Used by GitHub Actions, etc.

### Coverage Badges

Add coverage badges to your README:

```markdown
![Coverage](https://img.shields.io/badge/coverage-85%25-brightgreen)
```

### Improving Coverage

To improve test coverage:

1. **Identify gaps**: Run `npm run test:coverage` and check the HTML report
2. **Add missing tests**: Write tests for uncovered code paths
3. **Focus on branches**: Pay special attention to conditional branches
4. **Edge cases**: Test error conditions and edge cases
5. **Refactor**: Consider refactoring complex, hard-to-test code

## ✍️ Writing Tests

### Test File Structure

```javascript
// Import testing libraries
import { expect } from 'chai';
import { describe, it, beforeEach, afterEach } from 'mocha';

// Import module to test
import { ModuleName } from '../../src/module/module.js';

// Optionally import utilities
import sinon from 'sinon';
import sinonChai from 'sinon-chai';

// Enhance Chai with Sinon
chai.use(sinonChai);

describe('ModuleName', () => {
  let moduleInstance;
  let sandbox;

  beforeEach(() => {
    // Set up test environment
    sandbox = sinon.createSandbox();
    moduleInstance = new ModuleName();
  });

  afterEach(() => {
    // Clean up
    sandbox.restore();
  });

  describe('methodName()', () => {
    it('should do something', () => {
      // Arrange
      const input = 'test';
      const expected = 'result';

      // Act
      const result = moduleInstance.methodName(input);

      // Assert
      expect(result).to.equal(expected);
    });

    it('should handle errors', () => {
      // Test error conditions
      expect(() => moduleInstance.methodName(null)).to.throw();
    });
  });
});
```

### Test Patterns

#### Testing Asynchronous Code

```javascript
it('should handle async operations', async () => {
  const result = await moduleInstance.asyncMethod();
  expect(result).to.equal('expected');
});

it('should handle async with done callback', (done) => {
  moduleInstance.asyncMethod().then((result) => {
    expect(result).to.equal('expected');
    done();
  }).catch(done);
});
```

#### Testing with Mocks

```javascript
it('should call external service', () => {
  // Create mock
  const mockService = {
    call: sandbox.stub().returns(Promise.resolve('success'))
  };

  // Inject mock
  moduleInstance.service = mockService;

  // Test
  return moduleInstance.useService().then((result) => {
    expect(result).to.equal('success');
    expect(mockService.call).to.have.been.calledOnce;
  });
});
```

#### Testing DOM Manipulation

```javascript
it('should manipulate DOM correctly', () => {
  // Set up DOM
  document.body.innerHTML = '<div id="test"></div>';

  // Call method
  moduleInstance.updateDOM();

  // Assert
  const element = document.getElementById('test');
  expect(element.textContent).to.equal('updated');
});
```

### Assertion Styles

Lithium uses Chai's expect style:

```javascript
// Equality
expect(value).to.equal(expected);
expect(value).to.deep.equal(object);

// Truthiness
expect(value).to.be.true;
expect(value).to.be.false;
expect(value).to.be.null;
expect(value).to.exist;

// Type checking
expect(value).to.be.a('string');
expect(value).to.be.an('array');
expect(value).to.be.an.instanceof(Class);

// Collections
expect(array).to.have.lengthOf(3);
expect(array).to.include('item');
expect(object).to.have.property('key');

// Exceptions
expect(fn).to.throw();
expect(fn).to.throw(Error);
expect(fn).to.throw('message');

// Sinon-Chai assertions
expect(spy).to.have.been.called;
expect(spy).to.have.been.calledOnce;
expect(spy).to.have.been.calledWith(args);
```

## 🏆 Best Practices

### Test Organization

1. **Group related tests**: Use `describe()` to group tests by feature/module
2. **Clear test names**: Use descriptive names that document behavior
3. **Single assertion**: Each test should have one main assertion
4. **Arrange-Act-Assert**: Follow the AAA pattern
5. **Test setup**: Use `beforeEach()` for common setup
6. **Test cleanup**: Use `afterEach()` for cleanup

### Test Quality

1. **Fast**: Tests should run quickly (< 100ms each)
2. **Isolated**: Tests should not depend on each other
3. **Deterministic**: Same input should always produce same result
4. **Comprehensive**: Test both happy paths and edge cases
5. **Maintainable**: Keep tests simple and readable
6. **Documentation**: Tests should document expected behavior

### Code Coverage

1. **Aim for 80%+**: Minimum coverage threshold
2. **Focus on branches**: Complex logic needs thorough testing
3. **Test edge cases**: Don't just test the happy path
4. **Avoid trivial tests**: Don't test framework/library code
5. **Balance**: Don't sacrifice readability for coverage

### Performance

1. **Avoid slow operations**: Don't test file I/O in unit tests
2. **Mock external services**: Use mocks for API calls
3. **Use test doubles**: Stubs, spies, and mocks appropriately
4. **Parallel testing**: Consider parallel test execution
5. **Optimize setup**: Minimize test setup time

## 🤖 Continuous Integration

### GitHub Actions Example

```yaml
name: Lithium CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: 18
      - run: npm install
      - run: cd tests && npm install
      - run: npm test
      - run: npm run test:coverage

  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: 18
      - run: npm install
      - run: npm run lint

  build:
    needs: [test, lint]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: 18
      - run: npm install
      - run: npm run build:prod
```

### CI Best Practices

1. **Run tests on every commit**: Catch issues early
2. **Enforce coverage**: Fail builds that don't meet thresholds
3. **Parallel testing**: Speed up test execution
4. **Test matrix**: Test across multiple Node.js versions
5. **Artifact caching**: Cache dependencies for faster builds
6. **Test reporting**: Upload test results and coverage

## 🐛 Troubleshooting

### Common Issues

#### Tests not running

```bash
# Check Mocha installation
npx mocha --version

# Reinstall dependencies
rm -rf node_modules tests/node_modules
npm install
cd tests && npm install
```

#### Coverage not working

```bash
# Check NYC installation
npx nyc --version

# Verify include/exclude patterns
# Check that source files are being instrumented
```

#### Slow tests

```bash
# Identify slow tests
npm test -- --reporter json | jq '.[] | select(.duration > 100)'

# Optimize slow tests
# Consider mocking external dependencies
```

#### Flaky tests

```bash
# Run tests multiple times
for i in {1..10}; do npm test; done

# Identify flaky tests
# Fix by removing external dependencies or adding retries
```

### Debugging Tests

```bash
# Run tests with debug output
DEBUG=* npm test

# Use Node.js inspector
node --inspect-brk node_modules/mocha/bin/_mocha

# Add debug statements
console.log('Debug info:', variable);
```

## 📚 Resources

### Testing Libraries

- **Mocha**: <https://mochajs.org/>
- **Chai**: <https://www.chaijs.com/>
- **NYC**: <https://github.com/istanbuljs/nyc>
- **Sinon**: <https://sinonjs.org/>
- **Sinon-Chai**: <https://github.com/domenic/sinon-chai>

### Testing Guides

- **Mocha Documentation**: <https://mochajs.org/#getting-started>
- **Chai Assertions**: <https://www.chaijs.com/api/bdd/>
- **NYC Configuration**: <https://github.com/istanbuljs/nyc#readme>
- **Testing JavaScript**: <https://github.com/airbnb/javascript#testing>

### Advanced Testing

- **Cypress**: <https://www.cypress.io/> (for E2E testing)
- **Jest**: <https://jestjs.io/> (alternative test framework)
- **Puppeteer**: <https://pptr.dev/> (browser automation)
- **Playwright**: <https://playwright.dev/> (browser testing)

## 🎯 Testing Goals

### Short-Term Goals

- [x] Set up Mocha + Chai testing framework
- [x] Configure NYC for code coverage
- [x] Create basic test structure
- [x] Add sample tests for core modules
- [x] Set up coverage thresholds

### Medium-Term Goals

- [ ] Add integration tests for module interactions
- [ ] Implement component testing
- [ ] Add performance testing
- [ ] Set up CI/CD integration
- [ ] Add test coverage badges

### Long-Term Goals

- [ ] Implement end-to-end testing with Cypress
- [ ] Add visual regression testing
- [ ] Implement accessibility testing
- [ ] Add automated performance monitoring
- [ ] Implement test impact analysis

## 📝 License

MIT License - Free to use, modify, and distribute.

## 🎉 Happy Testing

The Lithium testing framework provides a solid foundation for building reliable,
well-tested Progressive Web Apps. By following these guidelines and best
practices, you can ensure high code quality and maintainability.
