# Test 31: Database Migration Validation

## Overview

Test 31 validates database migration files using SQL linting tools (sqruff) to ensure migration scripts are syntactically correct and follow best practices for each supported database engine.

## Purpose

This test performs static analysis on database migration files to catch SQL syntax errors, deprecated constructs, and potential issues before they are applied to production databases.

## Test Flow

1. **Discovery**: Finds all migration files in the Helium project directory (`../../../elements/002-helium`)
2. **Validation**: For each migration file, validates against all supported database engines
3. **Linting**: Uses sqruff with engine-specific configuration files to check SQL syntax
4. **Reporting**: Provides detailed results for each engine/schema/migration combination

## Supported Engines

- PostgreSQL (`postgresql`)
- SQLite (`sqlite`)
- MySQL/MariaDB (`mysql`)
- IBM DB2 (`db2`)

## Schema Mapping

Each design (helium, acuranzo) has specific schemas for each engine:

- **helium**: postgresql→`helium`, sqlite→`helium`, mysql→`HELIUM`
- **acuranzo**: postgresql→`app`, sqlite→`acuranzo`, mysql→`ACURANZO`

## Configuration

- **Cache Directory**: `~/.cache/sqruff` for storing validation results
- **Config Files**: `.sqruff_postgresql`, `.sqruff_mysql`, `.sqruff_sqlite`, `.sqruff_db2`
- **Migration Pattern**: Files matching `${design}_????.lua` in design directories

## Output Format

Each validation combination produces a subtest with format:

```parameters
{engine} {design} {schema} {migration_num}
```

For engines without schemas, displays:

```parameters
{engine} {design} (no schema) {migration_num}
```

## Success Criteria

- All migration files pass sqruff validation for their respective engines
- No SQL syntax errors or linting violations
- Cache mechanism works correctly for repeated runs

## Dependencies

- `sqruff` SQL linter must be installed
- Engine-specific sqruff configuration files must exist
- Helium project must be accessible at relative path `../../../elements/002-helium`

## Cache Mechanism

Results are cached based on file hash to avoid re-validation of unchanged files. Cache includes:

- Design name
- Engine type
- Schema name
- Migration file hash
- Validation result (PASSED/FAILED)

## Error Handling

- Missing sqruff: Test is skipped with warning
- Missing config files: Individual validations are skipped
- SQL generation failures: Captured with diagnostic files
- Network/SQL errors: Properly handled with appropriate exit codes

## Related Tests

- **Test 30**: Database connectivity and basic operations
- **Test 32-35**: Migration performance testing for specific engines