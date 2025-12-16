# Migration Anatomy: Deep Dive into Migration Structure

This document provides a detailed explanation of how Helium migrations work internally, including the queries table pattern, state transitions, and the relationships between forward, reverse, and diagram migrations.

## The Queries Table Pattern

Unlike traditional migration systems that execute DDL directly, Helium migrations use an indirect approach where migrations insert metadata about themselves into a special `queries` table. The Hydrogen server system then processes these records to execute the actual database changes.

This design provides several benefits:

- **Metadata Tracking**: Every migration is tracked with full metadata
- **State Management**: Migrations can be in different states (pending, applied, reversed)
- **Multi-Engine Support**: Same migration format works across all database engines
- **Audit Trail**: Complete history of all migration operations

## The INSERT INTO queries Pattern

Every migration follows this pattern:

```sql
INSERT INTO ${SCHEMA}queries (
    ${QUERIES_INSERT}
)
WITH next_query_id AS (
    SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
    FROM ${SCHEMA}queries
)
SELECT
    new_query_id                                                        AS query_id,
    '${MIGRATION}'                                                      AS query_ref,
    ${STATUS_ACTIVE}                                                    AS query_status_a27,
    ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
    ${DIALECT}                                                          AS query_dialect_a30,
    ${QTC_SLOW}                                                         AS query_queue_a58,
    ${TIMEOUT}                                                          AS query_timeout,
    [=[
        -- The actual SQL to execute
        CREATE TABLE ${SCHEMA}${TABLE} (...);
    ]=]                                                                 AS code,
    'Migration ${MIGRATION}: Description'                               AS name,
    [=[
        # Migration ${MIGRATION}

        Detailed description in Markdown
    ]=]                                                                 AS summary,
    '{}'                                                                AS collection,
    ${COMMON_INSERT}
FROM next_query_id;
```

### Key Components

- **next_query_id CTE**: Generates the next available query_id
- **query_ref**: The migration number (e.g., '1001')
- **query_type_a28**: The migration type (forward/reverse/diagram)
- **code**: The actual SQL to execute, embedded in [=[...]=]
- **collection**: JSON metadata (used for diagrams)
- **Common fields**: Audit fields (created_at, updated_at, etc.)

## Migration State Transitions

Migrations transition through states as they are applied and potentially rolled back:

### State Values

| State | Type Value | Description |
|-------|------------|-------------|
| Forward | 1000 | Ready to be applied |
| Applied | 1003 | Successfully applied |
| Reverse | 1001 | Ready to undo changes |
| Diagram | 1002 | Metadata for ERD generation |

### Transition Flow

```flow
Initial State → Forward Migration (1000) → Applied (1003) → Reverse Migration (1001) → Forward (1000)
     ↓              ↓                           ↓              ↓              ↓
  Not Run       Apply                      Applied       Rollback       Ready to re-apply
```

### How Transitions Work

1. **Forward Migration**: Inserts a record with type 1000 and includes SQL to:
   - Perform the schema/data changes
   - Update its own record to type 1003 (applied)

2. **Applied State**: The migration record has type 1003, indicating it was successful

3. **Reverse Migration**: Inserts a record with type 1001 and includes SQL to:
   - Undo the forward changes
   - Update the applied record back to type 1000

4. **Diagram Migration**: Inserts metadata with type 1002 for ERD generation

## Forward/Reverse/Diagram Relationships

Every schema change requires three migration components:

### Forward Migration (Required)

- **Purpose**: Applies the actual database changes
- **Type**: 1000 (`TYPE_FORWARD_MIGRATION`)
- **Content**: DDL statements (CREATE, ALTER, INSERT, etc.)
- **State Change**: Updates itself to applied (1003) when successful

### Reverse Migration (Required)

- **Purpose**: Undoes the forward migration for rollback/testing
- **Type**: 1001 (`TYPE_REVERSE_MIGRATION`)
- **Content**: DROP, DELETE, or reverse ALTER statements
- **State Change**: Updates applied migration back to forward (1000)

### Diagram Migration (Optional but Recommended)

- **Purpose**: Provides metadata for Entity-Relationship Diagrams
- **Type**: 1002 (`TYPE_DIAGRAM_MIGRATION`)
- **Content**: JSON schema definitions in the `collection` field
- **No State Change**: Pure metadata, doesn't modify database state

## Query Types Workflow

The system uses query types to manage the migration lifecycle:

### Forward Migration Workflow

1. Migration inserts record with type 1000
2. Hydrogen executes the `code` field SQL
3. If successful, the SQL includes an UPDATE to change type to 1003
4. Migration is now "applied"

### Reverse Migration Workflow

1. Migration inserts record with type 1001
2. Hydrogen executes the reverse SQL
3. If successful, the SQL updates the applied record back to type 1000
4. Migration is now "unapplied" and can be re-run

### Diagram Generation

1. Migration inserts record with type 1002
2. `collection` field contains JSON table definitions
3. Hydrogen uses this for ERD generation
4. No database changes occur

## Diagram Migration Details

Diagram migrations provide JSON metadata for automatic ERD generation. The JSON is stored in the `collection` field and defines table structures for visualization.

### JSON Schema Structure

```json
{
  "diagram": [
    {
      "object_type": "table",
      "object_id": "table.${TABLE}",
      "object_ref": "${MIGRATION}",
      "table": [
        {
          "name": "id",
          "datatype": "${INTEGER}",
          "nullable": false,
          "primary_key": true,
          "unique": false
        },
        {
          "name": "status_a1",
          "datatype": "${INTEGER}",
          "nullable": false,
          "primary_key": false,
          "unique": false,
          "lookup": true
        }
      ]
    }
  ]
}
```

### Field Properties

- **lookup**: true for fields that reference lookup tables
- **standard**: true for audit fields (created_at, updated_at, etc.)
- **primary_key**: true for primary key fields
- **nullable**: true if field allows NULL values
- **unique**: true if field has unique constraint

### Example from acuranzo_1001.lua

The lookups table diagram defines fields with `lookup: true` for status fields and `standard: true` for audit fields.

## Database-Specific Quirks

Different databases have unique behaviors that affect migration design:

### MySQL Auto-Increment

MySQL uses `AUTO_INCREMENT` instead of `SERIAL`. The changelog in `acuranzo_1001.lua` mentions "thanks, MySQL" for alternate increment mechanisms.

### PostgreSQL JSON Support

PostgreSQL has native `jsonb` type, while others store JSON as text and use functions for processing.

### SQLite Limitations

SQLite doesn't support some advanced features and requires extensions for compression/crypto functions.

### DB2 Case Sensitivity

DB2 is case-sensitive and uses uppercase for system objects, requiring different schema handling.

### Encoding and Compression

- **PostgreSQL**: Native Base64 and Brotli UDF
- **MySQL**: FROM_BASE64() and custom UDF
- **SQLite**: sqlean crypto extension
- **DB2**: Custom UDFs for all encoding

## Common Patterns

### Table Creation Pattern

```lua
-- Forward: Create table + mark as applied
CREATE TABLE ${SCHEMA}${TABLE} (...);
UPDATE ${SCHEMA}queries SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
WHERE query_ref = ${MIGRATION};

-- Reverse: Drop table + mark as forward
DROP TABLE ${SCHEMA}${TABLE};
UPDATE ${SCHEMA}queries SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
WHERE query_ref = ${MIGRATION};

-- Diagram: JSON metadata in collection field
```

### Data Insertion Pattern

```lua
-- Insert reference data
INSERT INTO ${SCHEMA}${TABLE} VALUES (...);

-- Reverse: Delete the inserted data
DELETE FROM ${SCHEMA}${TABLE} WHERE ...;
```

### Schema Modification Pattern

```lua
-- Forward: ALTER TABLE
ALTER TABLE ${SCHEMA}${TABLE} ADD COLUMN ...;

-- Reverse: Remove the column
ALTER TABLE ${SCHEMA}${TABLE} DROP COLUMN ...;
```

## Best Practices

1. **Always include reverse migrations** for testing and rollback
2. **Use diagram migrations** for any schema changes
3. **Test on all supported databases** before committing
4. **Include comprehensive state updates** in your SQL
5. **Use descriptive names and summaries** for clarity
6. **Leverage macros** for database portability
7. **Document database-specific quirks** in changelogs

This architecture provides a robust, metadata-driven migration system that supports complex deployment scenarios while maintaining full auditability and reversibility.