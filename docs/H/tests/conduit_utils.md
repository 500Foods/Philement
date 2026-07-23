# Conduit Utilities Library

## Overview

The [`conduit_utils.sh`](/elements/001-hydrogen/hydrogen/tests/lib/conduit_utils.sh) library provides reusable functions for testing the Conduit Service endpoints in the Hydrogen test suite. It handles server lifecycle management, database readiness checking, JWT acquisition, and endpoint testing across all 7 supported database engines.

## Purpose

This library standardizes conduit testing operations including:

- Server startup with automatic migration waiting
- Database readiness detection for selective test execution
- JWT token acquisition for authenticated endpoint testing
- Conduit endpoint request validation and response parsing
- Graceful server shutdown and cleanup

## Key Features

### Database Engine Mapping

The library defines a canonical mapping between database engine names and their configuration names in the demo Acuranzo schema:

```bash
declare -A DATABASE_NAMES
DATABASE_NAMES=(
    ["PostgreSQL"]="Demo_PG"
    ["MySQL"]="Demo_MY"
    ["SQLite"]="Demo_SQL"
    ["DB2"]="Demo_DB2"
    ["MariaDB"]="Demo_MR"
    ["CockroachDB"]="Demo_CR"
    ["YugabyteDB"]="Demo_YB"
)
```

This mapping ensures consistent naming across tests 50, 51, 52, 53, 54, and 55.

### Query References

The library maintains test query references for public (no auth required) and non-public conduit queries:

```bash
declare -A PUBLIC_QUERY_REFS
PUBLIC_QUERY_REFS=(
    ["53"]="Get Themes:false:{}"
    ["54"]="Get Icons:false:{}"
    ["55"]="Get Number Range:false:{\"INTEGER\":{\"START\":500,\"FINISH\":600}}"
)

declare -A NON_PUBLIC_QUERY_REFS
NON_PUBLIC_QUERY_REFS=(
    ["30"]="Get Lookups List:true"
    ["45"]="Get Lookup:true"
)
```

Format: `"query_ref"="description:requires_auth:params_json"`

## Library Functions

### validate_conduit_request

Issues an HTTP request to a conduit endpoint and validates the response.

**Signature**:

```bash
validate_conduit_request() {
    local endpoint="$1"      # Full URL including /api/conduit/...
    local method="$2"        # HTTP method (GET, POST, etc.)
    local data="$3"          # Request body for POST requests
    local expected_status="$4" # Expected HTTP status code
    local output_file="$5"     # File to write response body
    local jwt_token="${6:-}"  # Optional: JWT for Authorization header
    local description="$7"     # Subtest description
    local expected_success="${8:-true}"  # Optional: Expected success field value
}
```

**Behavior**:

- Sends request via `curl` with appropriate headers
- For non-GET methods, adds `Content-Type: application/json`
- If JWT provided, adds `Authorization: Bearer <token>`
- Extracts `success`, `row_count`, `error`, and `message` fields for reporting
- Compares HTTP status against expected value
- Checks `success` field if `expected_success` is specified

### check_database_readiness

Polls server logs to determine which databases completed migration.

**Signature**:

```bash
check_database_readiness() {
    local log_file="$1"      # Path to server log
    local result_file="$2"   # File to write readiness markers
}
```

**Behavior**:

- Greps log for each database name followed by "Migration completed"
- Writes `DATABASE_READY_{Engine}=true/false` for each engine
- YugabyteDB receives an extra 5-second grace period due to slower startup

**Result File Format**:

```text
DATABASE_READY_PostgreSQL=true
DATABASE_READY_MySQL=true
DATABASE_READY_SQLite=true
DATABASE_READY_DB2=false
DATABASE_READY_MariaDB=true
DATABASE_READY_CockroachDB=true
DATABASE_READY_YugabyteDB=false
```

### acquire_jwt_tokens

Obtains JWT tokens for all ready databases via the login endpoint.

**Signature**:

```bash
acquire_jwt_tokens() {
    local base_url="$1"      # Server base URL (e.g., http://localhost:5510)
    local result_file="$2"   # Path to result file with DATABASE_READY markers
}
```

**Behavior**:

- Iterates through all databases in `DATABASE_NAMES`
- Skips databases not marked as ready
- Calls `/api/auth/login` with demo credentials for each ready database
- Extracts JWT token from successful responses
- Stores tokens in a test-specific global variable

**Environment Variables Used**:

- `HYDROGEN_DEMO_USER_NAME` - Login ID
- `HYDROGEN_DEMO_USER_PASS` - Password
- `HYDROGEN_DEMO_API_KEY` - API key

### test_conduit_endpoint

Tests a conduit endpoint across all ready databases.

**Signature**:

```bash
test_conduit_endpoint() {
    local base_url="$1"            # Server base URL
    local result_file="$2"         # Path to result file
    local endpoint="$3"            # Endpoint path (e.g., /api/conduit/query)
    local payload_template="$4"    # JSON payload with {{DB_NAME}} placeholder
    local jwt_token="${5:-}"       # Optional: JWT for auth endpoint
    local description_prefix="$6"  # Description prefix for subtests
    local expected_success="${7:-true}"  # Optional: Expected success value
}
```

**Returns**: `"passed:total"` count string

### test_conduit_single_queries

Tests single query endpoints across all databases.

**Signature**:

```bash
test_conduit_single_queries() {
    local base_url="$1"
    local result_file="$2"
    local endpoint="$3"            # e.g., /api/conduit/query or /api/conduit/auth_query
    local jwt_token="${4:-}"       # JWT for authenticated queries
    local description_prefix="$5"
}
```

**Behavior**:

- Iterates through `PUBLIC_QUERY_REFS` for public endpoints
- Iterates through `NON_PUBLIC_QUERY_REFS` for authenticated endpoints
- Replaces `{{DB_NAME}}` placeholder with actual database name
- Captures per-database results

### test_conduit_multiple_queries_endpoint

Tests batch query endpoint across all ready databases.

**Signature**:

```bash
test_conduit_multiple_queries_endpoint() {
    local base_url="$1"
    local result_file="$2"
    local endpoint="$3"            # e.g., /api/conduit/queries or /api/conduit/auth_queries
    local jwt_token="${4:-}"       # Optional: JWT for auth endpoint
    local description_prefix="$5"
    local queries_json="$6"        # JSON array of query objects
}
```

### run_conduit_server

Manages the complete server lifecycle for conduit testing.

**Signature**:

```bash
run_conduit_server() {
    local config_file="$1"         # Path to Hydrogen config JSON
    local log_suffix="$2"          # Suffix for log filename
    local description="$3"         # Description for output
    local result_file="$4"           # Path to result file
}
```

**Behavior**:

1. Starts Hydrogen server with configuration file
2. Waits for `STARTUP COMPLETE` (default 120s, configurable via `CONDUIT_STARTUP_TIMEOUT`)
3. Waits for all database migrations to complete (300s timeout)
4. Calls `check_database_readiness()` for database status
5. Waits for webserver to respond to HTTP requests (default 120s, configurable via `CONDUIT_WEBSERVER_READY_TIMEOUT`)
6. Returns `"base_url:pid"` on success or `"FAILED:0"` on failure

**Timeout Configuration**:

```bash
CONDUIT_STARTUP_TIMEOUT=180        # For ASAN or slow multi-DB configs
CONDUIT_WEBSERVER_READY_TIMEOUT=180 # For ASAN builds
```

### shutdown_conduit_server

Gracefully terminates a conduit server.

**Signature**:

```bash
shutdown_conduit_server() {
    local hydrogen_pid="$1"        # Process ID of Hydrogen server
    local result_file="$2"           # Path to result file (unused but kept for interface symmetry)
}
```

**Behavior**:

- Sends SIGINT to initiate graceful shutdown
- Waits up to 10 seconds for process termination
- Sends SIGKILL if process doesn't exit gracefully

### analyze_conduit_results

Aggregates and reports conduit test results.

**Signature**:

```bash
analyze_conduit_results() {
    local log_suffix="$1"            # Suffix for result file
    local description="$2"           # Test description
}
```

**Behavior**:

- Checks for `STARTUP_SUCCESS` in result file
- Aggregates results for all test categories:
  - Single public query tests
  - Single invalid query tests
  - Multiple queries tests
  - Comprehensive queries tests
  - Auth query comprehensive tests
  - Auth multiple query tests
  - Alt single query tests
  - Alt multiple query tests
  - JWT acquisition tests
  - Status endpoint tests
- Reports pass/total ratios for each category

## Configuration Requirements

### Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `HYDROGEN_ROOT` | Yes | Path to Hydrogen project root |
| `HYDROGEN_BIN` | Yes | Path to Hydrogen binary |
| `HYDROGEN_DEMO_USER_NAME` | Yes | Demo login ID |
| `HYDROGEN_DEMO_USER_PASS` | Yes | Demo password |
| `HYDROGEN_DEMO_API_KEY` | Yes | API key |
| `CONDUIT_STARTUP_TIMEOUT` | No | Startup timeout (default: 120s) |
| `CONDUIT_WEBSERVER_READY_TIMEOUT` | No | Webserver ready timeout (default: 120s) |

### Framework Variables

These are set by `framework.sh` before sourcing this library:

- `TEST_NUMBER` - Current test number
- `TEST_COUNTER` - Current subtest counter
- `LOG_PREFIX` - Log file path prefix
- `TIMESTAMP` - Current timestamp for log files
- `SECONDS` - Bash timer for elapsed time tracking

## Integration with Other Libraries

This library works in conjunction with:

- **`framework.sh`**: Provides `print_subtest`, `print_message`, `print_result`
- **`lifecycle.sh`**: Alternative server lifecycle functions (see below)

## Related Documentation

- [test_50_conduit_query.md](/docs/H/tests/test_50_conduit_query.md) - Conduit single query testing
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Conduit comprehensive testing
- [framework.md](/docs/H/tests/framework.md) - Test framework utilities
- [CONDUIT.md](/docs/H/plans/complete/CONDUIT_COMPLETE.md) - Conduit service implementation plan