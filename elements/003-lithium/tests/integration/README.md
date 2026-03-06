# Lithium Integration Tests

These tests run against a real Hydrogen server to verify end-to-end functionality.

## Requirements

- Hydrogen server running and accessible
- Database "Lithium" configured in Hydrogen
- Valid demo credentials in environment variables

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `HYDROGEN_SERVER_URL` | Hydrogen server base URL | `http://localhost:8080` |
| `HYDROGEN_DEMO_USER_NAME` | Demo username for login tests | - |
| `HYDROGEN_DEMO_USER_PASS` | Demo password for login tests | - |
| `HYDROGEN_DEMO_API_KEY` | Demo API key for login tests | - |

## Running Tests

### With npm

```bash
# Run all tests (unit + integration)
npm test

# Run only integration tests
npm test -- tests/integration/

# Run with specific server
HYDROGEN_SERVER_URL=http://localhost:8081 npm test -- tests/integration/

# Run with full credentials
HYDROGEN_DEMO_USER_NAME=testuser \
HYDROGEN_DEMO_USER_PASS=testpass \
HYDROGEN_DEMO_API_KEY=test-api-key \
npm test -- tests/integration/
```

### With the test script

```bash
cd tests
./test_50_lithium_auth.sh
```

## Test Structure

### auth.integration.test.js

Tests the authentication endpoints:

- **Login**: Success, invalid credentials, missing fields
- **Token Renewal**: Valid token, invalid token
- **Logout**: Valid token, expired token
- **End-to-End Flow**: Complete auth lifecycle
- **JWT Claims**: Verify all expected claims are present

## Skipping Integration Tests

If the Hydrogen server is not available or credentials are missing, tests are automatically skipped with a warning message.

## Notes

- Tests clean up after themselves (logout and clear tokens)
- Tests are independent and can run in any order
- The database name is hardcoded as "Lithium" to match your setup
