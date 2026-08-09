# Testing Guide for Helium Migrations

This guide explains how to validate, test, and debug Helium migrations before deployment.

## Prerequisites

- Lua 5.1+ with luarocks
- luacheck: `luarocks install luacheck`
- Database engines: PostgreSQL, MySQL, SQLite, or DB2
- Hydrogen test suite access

## Syntax Validation

### Basic Lua Syntax Check

Validate your migration file syntax:

```bash
lua -e "require 'acuranzo_9999'"
```

If no errors, the Lua syntax is valid.

### Macro Expansion Test

Test macro expansion without database:

```bash
lua database.lua postgresql acuranzo public < acuranzo_9999.lua > /dev/null
```

This will show any undefined macros or expansion errors.

## Linting with Luacheck

Luacheck validates Lua code quality and catches common issues.

### Basic Linting

```bash
luacheck acuranzo_9999.lua
```

### Migration-Specific Linting

Migrations use special luacheck directives:

```lua
-- luacheck: no max line length
-- luacheck: no unused args
```

Run luacheck with migration allowances:

```bash
luacheck --config .luacheckrc acuranzo_9999.lua
```

### Common Luacheck Issues

- **Max line length**: Use `-- luacheck: no max line length` for long SQL lines
- **Unused arguments**: Use `-- luacheck: no unused args` for function parameters
- **Global variables**: Avoid globals; use locals
- **Undefined variables**: Ensure all macros are defined or will be resolved at runtime

## Markdown Linting

The project uses `markdownlint` for markdown formatting validation. Configuration is provided via `.lintignore-markdown` in the project root.

### Configuration File: .lintignore-markdown

Location: `elements/002-helium/.lintignore-markdown`

This file disables specific markdownlint rules to accommodate the project's documentation style:

- **MD013**: Allows lines longer than 80 characters (for code blocks, tables, etc.)
- **MD033**: Allows raw HTML tags in Markdown (for advanced formatting)
- **MD047**: Doesn't require newline at the end of the file

### Running Markdown Linting

```bash
# Lint all markdown files
markdownlint docs/He/*.md docs/He/**/*.md

# With custom configuration
markdownlint --config .lintignore-markdown docs/He/*.md docs/He/**/*.md
```

This is automatically run by `helium_update.sh` as part of the documentation quality checks.

## Testing on Database Engines

### Manual Testing

Test migration on each supported database:

#### PostgreSQL

```bash
lua database.lua postgresql acuranzo public < acuranzo_9999.lua | psql -h localhost -U user -d db
```

#### MySQL

```bash
lua database.lua mysql acuranzo schema < acuranzo_9999.lua | mysql -h localhost -u user -p db
```

#### SQLite

```bash
lua database.lua sqlite acuranzo '' < acuranzo_9999.lua | sqlite3 database.db
```

#### DB2

```bash
lua database.lua db2 acuranzo schema < acuranzo_9999.lua | db2 -f -
```

### Verification Steps

After running a migration:

1. **Check queries table**:

   ```sql
   SELECT * FROM queries WHERE query_ref = '9999';
   ```

2. **Verify schema changes**:

   ```sql
   \d table_name  -- PostgreSQL
   DESCRIBE table_name;  -- MySQL
   .schema table_name  -- SQLite
   ```

3. **Test data integrity**:

   ```sql
   SELECT COUNT(*) FROM your_table;
   ```

### Rollback Testing

Test the reverse migration:

```bash
# Run reverse (same command, system detects reverse type)
lua database.lua postgresql acuranzo public < acuranzo_9999.lua | psql ...

# Verify rollback
SELECT * FROM queries WHERE query_ref = '9999';  -- Should be type 1000 again
```

## Important: Payload Rebuilds When Changing Migrations

The vast majority of Hydrogen tests (including all the migration-related ones: Test 30, 31–38, 71, etc.) execute against a built Hydrogen binary. That binary embeds the current Helium migrations inside its **payload**.

After editing any files under `elements/002-helium/` (migrations, database.lua, etc.):

- You **must** rebuild the payload before running Hydrogen tests that exercise migrations, otherwise the tests will run against stale embedded migrations.
- Use the shell aliases defined in `~/.zshrc`:
  - `mkt` — "make trial" (faster incremental build, typical for development)
  - `mka` — "make all" (full build)
- These aliases expand to something like:

  ```bash
  cdh && extras/make-trial.sh ; cd - > /dev/null 2>&1
  ```

  (where `cdh` changes to the Hydrogen directory).

The Hydrogen build itself (see Test 01 - Compilation) also detects when migration files are newer than the payload and will automatically regenerate it (and run `helium_update.sh`).

Only after the payload is current should you run the individual migration tests described below.

## Running Hydrogen Test Suite

Hydrogen includes comprehensive tests for Helium migrations.

### Test 30: Database Subsystem DQM Startup

Validates database connectivity and startup:

```bash
cd elements/001-hydrogen/hydrogen
./tests/test_30_database.sh
```

> **Remember**: If you have edited any Helium migrations since the last build, run `mkt` or `mka` first (see "Important: Payload Rebuilds When Changing Migrations" above) so the tests see the updated migrations inside the Hydrogen payload.

### Test 31: Migration Validation

Tests migration processing across all engines:

```bash
cd elements/001-hydrogen/hydrogen
./tests/test_31_migrations.sh
```

> **Remember**: If you have edited any Helium migrations since the last build, run `mkt` or `mka` first (see "Important: Payload Rebuilds When Changing Migrations" above) so the tests see the updated migrations inside the Hydrogen payload.

This test:

- Validates SQL syntax for all migrations
- Tests macro expansion
- Checks for unsubstituted variables
- Verifies schema handling

### Test 71: Database Diagram Generation

Tests ERD generation from diagram migrations:

```bash
cd elements/001-hydrogen/hydrogen
./tests/test_71_database_diagrams.sh
```

> **Remember**: If you have edited any Helium migrations since the last build, run `mkt` or `mka` first (see "Important: Payload Rebuilds When Changing Migrations" above) so the tests see the updated migrations inside the Hydrogen payload (and so diagram extraction sees the latest JSON metadata).

## Debugging Common Issues

### Undefined Macro Errors

**Error**: `attempt to call a nil value (field 'UNKNOWN_MACRO')`

**Cause**: Using a macro not defined in database.lua or engine files

**Solution**:

- Check MACRO_REFERENCE.md for available macros
- Use environment variables for custom values: `${MY_VAR}`
- Ensure environment variables are set at runtime

### SQL Syntax Errors

**Error**: Database-specific SQL syntax issues

**Solution**:

- Test on the target database engine first
- Check database-specific macro expansions
- Use `${SCHEMA}` and `${TABLE}` macros for portability

### Multi-row seed fails on SQLite or DB2 only

**Symptoms** (seen on `acuranzo_1293` before the portable rewrite):

- SQLite: `near "(": syntax error` while applying a seed that uses
  `FROM (VALUES …) AS v(col1, col2, …)`
- DB2: `SQL0104N An unexpected token "UNION" was found…` for
  `INSERT … SELECT … FROM (SELECT … UNION ALL SELECT …)`

**Cause**: Row-source syntax that is valid on PostgreSQL but not on SQLite
and/or DB2.

**Solution**:

- Use multi-row `INSERT INTO … (cols, ${COMMON_FIELDS}) VALUES (…), (…);`
  with `${COMMON_VALUES}` — same pattern as `acuranzo_1280.lua`
- Do **not** use PostgreSQL-only `VALUES AS v(cols)` aliases
- Do **not** assume bare `UNION ALL` derived tables are portable
- For single-row `SELECT … WHERE NOT EXISTS` on DB2, include `${DUMMY_TABLE}`
  (see `docs/He/GUIDE.md` → **Portable Multi-Row Data Seeds**)
- Rebuild payload (`mkt` / `mka`) after fixing the migration; re-run
  `test_34_sqlite_migrations` and `test_35_db2_migrations`

### Migration State Issues

**Error**: Migration shows wrong state in queries table

**Solution**:

- Check that UPDATE statements in migration SQL are correct
- Verify query_ref matches migration number
- Ensure reverse migration resets state properly

### DB2 SQL0100W / SQLSTATE 02000 on reverse (zero-row DELETE)

**Read this first:** This is almost always a **bad reverse migration**, not a
Hydrogen or DB2 defect. DB2 is stricter than other engines on purpose — use
the failure to fix the Lua, do not teach the driver to ignore it.

**Rule (same as `docs/He/GUIDE.md` → Forward/Reverse Symmetry):**

- Forward `INSERT`s / `CREATE`s / `ALTER`s → reverse undoes **exactly** that
- Reverse must **not** `DELETE`/`UPDATE` rows or tables the forward never touched
- Hundreds of migrations never hit this because they already mirrored correctly

**Symptoms** (e.g. TestMigration reverse fails on `acuranzo_1293` statement 1):

- `SQL0100W  No row was found for FETCH, UPDATE or DELETE…`
- `SQLSTATE: 02000, Native Error: 100`
- `Reverse migration statement N failed` (often statement 1)
- PG/SQLite/MySQL may have **passed** the same reverse (0-row DELETE = OK there)

**Cause**: Reverse `DELETE`/`UPDATE` matched **zero rows**. On DB2 CLI that is
`SQL_NO_DATA` and Hydrogen correctly treats it as failure.

Concrete bad example (1293 before 1.0.3):

| Forward | Broken reverse |
|---------|----------------|
| Seed free `courses` only (FL-34: **no** `course_prices` rows) | `DELETE FROM course_prices WHERE …` then delete courses |

Fixed reverse: delete only the seeded `courses` by `code` — no prices statement.

**Solution:**

1. Open the reverse `code` block; list every DML statement
2. For each, confirm forward actually wrote matching rows/objects
3. Remove or rewrite any reverse that cannot match rows after a clean APPLY
4. Rebuild payload; re-run `test_35_db2_migrations`

**Do not:**

- Change Hydrogen DB2 execute to treat `SQL_NO_DATA` / SQL0100W as success
- “Fix” by inserting dummy reverse-side rows so DELETE finds something
- Assume empty DELETE is harmless because other engines accepted it

### DB2 SQL0668N reason code 7 (reorg-pending) on reverse

**Symptoms** (e.g. TestMigration after APPLY through N, reverse fails on N−1 or N−2):

- Reverse for migration **M** (DROP COLUMN) appears to succeed
- Next older reverse fails on DML (`DELETE`/`UPDATE`) against the same table with:
  `SQL0668N Operation not allowed for reason code "7" on table "SCHEMA.TABLE"`
  `SQLSTATE=57007`

**Cause**: DB2 marked the table reorg-pending after `DROP COLUMN` (or similar
structural ALTER). Reverse for **M** had `${REORG}` only before the DROP (or
not at all), not **after**.

**Solution**:

- After every structural `DROP COLUMN` / `ADD COLUMN` / relevant `ALTER COLUMN`,
  emit `${REORG}` (macro → `CALL SYSPROC.ADMIN_CMD('REORG TABLE …')` on DB2;
  no-op comment elsewhere)
- Match `acuranzo_1172.lua` / `acuranzo_1297.lua`: REORG before **and** after DROP
- See `docs/He/GUIDE.md` → **DB2: `${REORG}` after ADD/DROP COLUMN** and
  `MACRO_REFERENCE.md` → `${REORG}`
- Rebuild payload; re-run `test_35_db2_migrations`

### Encoding/Compression Failures

**Error**: Base64 or Brotli encoding issues

**Solution**:

- Verify lua-brotli is installed
- Check file size (compression only for >1KB)
- Test on database with required UDFs installed

### Environment Variable Missing

**Error**: `os.getenv() returned nil`

**Solution**:

- Set required environment variables before running
- Use defaults in deployment scripts: `${VAR:-default}`
- Document all required variables

## Performance Testing

### Large Content Testing

Test migrations with large embedded content:

```bash
# Check processing time
time lua database.lua postgresql acuranzo public < large_migration.lua > /dev/null

# Verify compression is applied for >1KB content
ls -la large_migration.lua  # Should be >1024 bytes
```

### Concurrent Migration Testing

Test multiple migrations running simultaneously:

```bash
# Run in parallel
lua database.lua ... < migration1.lua | psql ... &
lua database.lua ... < migration2.lua | psql ... &
wait
```

## Integration Testing

### Full Schema Testing

Test complete schema deployment:

```bash
# Run all migrations in order
for migration in acuranzo_*.lua; do
    echo "Running $migration"
    lua database.lua postgresql acuranzo public < "$migration" | psql ...
done
```

### Cross-Engine Consistency

Ensure same results across engines:

```bash
# PostgreSQL
lua ... postgresql ... < migration.lua | psql ...

# MySQL
lua ... mysql ... < migration.lua | mysql ...

# Compare results
# (Manual verification of table structures and data)
```

## Best Practices

1. **Test early, test often** - Validate after every change
2. **Test all engines** - Don't assume portability
3. **Use the test suite** - Hydrogen tests catch many issues
4. **Document test results** - Keep records of successful runs
5. **Automate testing** - Integrate into CI/CD pipelines
6. **Monitor performance** - Watch for slow migrations
7. **Test rollbacks** - Ensure reversibility works
8. **Validate in staging** - Test in environment like production

Following this guide ensures your migrations are robust, portable, and ready for production deployment.