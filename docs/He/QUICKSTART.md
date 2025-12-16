# Quick Start: Your First Migration

This 10-minute tutorial will guide you through creating, testing, and rolling back your first database migration in the Helium system.

## Prerequisites

- **Lua 5.1+** installed with luarocks
- **lua-brotli** library: `luarocks install lua-brotli`
- **Database**: PostgreSQL, MySQL, or SQLite running locally
- **Basic SQL knowledge**

## Step 1: Create Your First Migration (2 minutes)

Create a new migration file for a simple "test_users" table:

```bash
# Navigate to the migrations directory
cd elements/002-helium/acuranzo/migrations/

# Create migration file (use next available number, e.g., 9999 for testing)
touch acuranzo_9999.lua
```

Edit `acuranzo_9999.lua` with this content:

```lua
-- Migration: acuranzo_9999.lua
-- Creates a test_users table for learning migrations

-- luacheck: no max line length
-- luacheck: no unused args

return function(engine, design_name, schema_name, cfg)
    local queries = {}

    cfg.TABLE = "test_users"
    cfg.MIGRATION = "9999"

    -- Forward Migration: Create the table
    table.insert(queries,{sql=[[
        INSERT INTO ${SCHEMA}${QUERIES} (
            ${QUERIES_INSERT}
        )
        WITH next_query_id AS (
            SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
            FROM ${SCHEMA}${QUERIES}
        )
        SELECT
            new_query_id,
            ${MIGRATION},
            ${STATUS_ACTIVE},
            ${TYPE_FORWARD_MIGRATION},
            ${DIALECT},
            ${QTC_SLOW},
            ${TIMEOUT},
            [=[
                CREATE TABLE ${SCHEMA}${TABLE} (
                    user_id ${INTEGER} PRIMARY KEY,
                    username ${VARCHAR_50} NOT NULL UNIQUE,
                    email ${VARCHAR_100},
                    ${COMMON_CREATE}
                );

                INSERT INTO ${SCHEMA}${TABLE} (user_id, username, email, ${COMMON_FIELDS})
                VALUES (1, 'testuser', 'test@example.com', ${COMMON_VALUES});
            ]=],
            'Create test_users table',
            [=[
                # Migration 9999: Test Users Table

                Creates a simple test_users table with sample data for learning purposes.
            ]=],
            '{}',
            ${COMMON_INSERT}
        FROM next_query_id;
    ]]}})

    -- Reverse Migration: Drop the table
    table.insert(queries,{sql=[[
        INSERT INTO ${SCHEMA}${QUERIES} (
            ${QUERIES_INSERT}
        )
        WITH next_query_id AS (
            SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
            FROM ${SCHEMA}${QUERIES}
        )
        SELECT
            new_query_id,
            ${MIGRATION},
            ${STATUS_ACTIVE},
            ${TYPE_REVERSE_MIGRATION},
            ${DIALECT},
            ${QTC_SLOW},
            ${TIMEOUT},
            [=[
                DROP TABLE ${SCHEMA}${TABLE};
            ]=],
            'Drop test_users table',
            [=[
                # Reverse Migration 9999: Drop Test Users Table

                Removes the test_users table created by the forward migration.
            ]=],
            '{}',
            ${COMMON_INSERT}
        FROM next_query_id;
    ]]}})

    return queries
end
```

## Step 2: Test the Migration Syntax (1 minute)

Validate your migration file syntax:

```bash
# Check Lua syntax
lua -e "require 'acuranzo_9999'"

# If no errors, the syntax is valid
```

## Step 3: Run the Forward Migration (2 minutes)

Execute the migration against your database:

```bash
# For PostgreSQL
lua database.lua postgresql acuranzo public < acuranzo_9999.lua | psql -h localhost -U your_user -d your_database

# For MySQL
lua database.lua mysql acuranzo your_schema < acuranzo_9999.lua | mysql -h localhost -u your_user -p your_database

# For SQLite
lua database.lua sqlite acuranzo '' < acuranzo_9999.lua | sqlite3 your_database.db
```

## Step 4: Verify the Migration (2 minutes)

Check that your migration worked:

```sql
-- Connect to your database and run:
SELECT * FROM test_users;
-- Should show: user_id=1, username='testuser', email='test@example.com'

SELECT * FROM queries WHERE query_ref = '9999';
-- Should show your migration records with query_type_a28 = 1003 (applied)
```

## Step 5: Run the Reverse Migration (2 minutes)

Test the rollback capability:

```bash
# Run the reverse migration (same command, it will execute the reverse query)
lua database.lua postgresql acuranzo public < acuranzo_9999.lua | psql -h localhost -U your_user -d your_database
```

Verify the rollback:

```sql
-- Check that the table is gone
SELECT * FROM test_users;
-- Should return: ERROR:  relation "test_users" does not exist

SELECT * FROM queries WHERE query_ref = '9999';
-- Should show query_type_a28 = 1000 (forward) again, ready to re-run
```

## Step 6: Clean Up (1 minute)

Remove your test migration:

```bash
rm acuranzo_9999.lua
```

## What You Learned

✅ **Migration Structure**: How migrations are Lua functions returning query arrays
✅ **Macro System**: Using `${TABLE}`, `${SCHEMA}`, etc. for database portability
✅ **Forward/Reverse Pattern**: Every change needs an undo operation
✅ **Query States**: How migrations transition from forward → applied → reverse → forward
✅ **Multi-Engine Support**: Same migration works on PostgreSQL, MySQL, SQLite, DB2

## Next Steps

- Read the [Migration Creation Guide](/docs/He/GUIDE.md) for advanced patterns
- Study existing migrations like `acuranzo_1001.lua`
- Learn about the [Macro System](/docs/He/MACRO_REFERENCE.md)
- Explore [Testing Guide](/docs/He/TESTING_GUIDE.md) for validation techniques

## Troubleshooting

### lua: cannot open acuranzo_9999.lua: No such file or directory"

- Make sure you're in the correct directory: `elements/002-helium/acuranzo/migrations/`

### Database connection errors

- Verify your database is running and credentials are correct
- Check that the `queries` table exists (run a real migration first)

### Migration doesn't appear in queries table

- Check for SQL syntax errors in the generated output
- Ensure your database user has INSERT permissions

### Macros not expanding

- Verify you're using the correct database engine name (postgresql, mysql, sqlite, db2)
- Check that database.lua and the engine-specific files are present