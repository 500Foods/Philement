# Lithium Integration Tests

These tests run against a real Hydrogen server to verify end-to-end functionality.

## Requirements

- Hydrogen server running and accessible (`lithium.500courses.com` by default)
- Database `Lithium` configured on that server
- 500 Courses test credentials in environment variables (not Hydrogen's demo account)

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `HYDROGEN_SERVER_URL` | Hydrogen server base URL | `https://lithium.500courses.com` |
| `DEV_500C_USER` | 500 Courses test username (`testuser`) | - |
| `DEV_500C_PASS` | 500 Courses test password | - |
| `HYDROGEN_DEMO_API_KEY` | Hydrogen install API key | - |

`HYDROGEN_DEMO_USER_NAME` / `HYDROGEN_DEMO_USER_PASS` are for Hydrogen's own demo database — leave them alone for Lithium. On angrin they live in `~/.festival.env` alongside `DEV_500C_*`. Password stays out of the repo.

## Running Tests

### With npm

```bash
# Run all tests (unit + integration)
npm test

# Run only integration tests
npm test -- tests/integration/

# Run against a local Hydrogen instead of the live host
HYDROGEN_SERVER_URL=http://localhost:8080 npm test -- tests/integration/
```

With `~/.festival.env` sourced (zshrc does this on angrin), plain `npm test` should pick up `DEV_500C_*` and the API key automatically.

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

If the Hydrogen server is not available or credentials are missing, tests are skipped (`it.skip` / `skip()`), not counted as passed.

The default live host is `lithium.500courses.com` (override with `HYDROGEN_SERVER_URL`).

## Notes

- Tests clean up after themselves (logout and clear tokens)
- Tests are independent and can run in any order
- The database name is hardcoded as "Lithium" to match your setup
