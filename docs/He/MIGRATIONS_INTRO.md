# Database Migrations: A Conceptual Introduction

This guide explains the fundamental concepts of database migrations and how they apply to software development, with specific focus on the Helium system's approach.

## What Are Database Migrations?

Database migrations are **versioned scripts** that modify database schema and data in a controlled, repeatable manner. They solve the problem of keeping database structures synchronized across different environments (development, staging, production) and team members.

### Why Migrations Matter

Without migrations, teams face these problems:

- **Schema drift**: Different environments have different table structures
- **Manual changes**: Developers make direct database changes that aren't tracked
- **Rollback difficulties**: Hard to undo changes safely
- **Collaboration issues**: Team members overwrite each other's changes

With migrations:

- **Version control**: Database changes are tracked like code
- **Repeatable**: Same changes can be applied to any environment
- **Reversible**: Changes can be rolled back safely
- **Collaborative**: Multiple developers can work on schema changes

## Core Concepts

### Forward Migration

A **forward migration** applies a change to bring the database to a newer state:

```sql
-- Example: Add a new column
ALTER TABLE users ADD COLUMN email VARCHAR(255);
```

### Reverse Migration

A **reverse migration** undoes the forward migration, returning to the previous state:

```sql
-- Example: Remove the column we added
ALTER TABLE users DROP COLUMN email;
```

### Migration States

```states
Initial State → Migration 001 → Migration 002 → Migration 003 → Current State
      ↓              ↓              ↓              ↓              ↓
   Rollback       Rollback       Rollback       Rollback       Rollback
      ↓              ↓              ↓              ↓              ↓
   Clean DB     State after 001  State after 002  etc.
```

## Migration Patterns

### Schema Changes

- **Creating tables**: `CREATE TABLE users (...)`
- **Adding columns**: `ALTER TABLE users ADD COLUMN email VARCHAR(255)`
- **Modifying columns**: `ALTER TABLE users ALTER COLUMN name TYPE TEXT`
- **Adding indexes**: `CREATE INDEX idx_users_email ON users(email)`
- **Creating constraints**: `ALTER TABLE users ADD CONSTRAINT chk_age CHECK (age > 0)`

### Data Changes

- **Inserting reference data**: `INSERT INTO roles VALUES (1, 'admin')`
- **Updating existing data**: `UPDATE users SET status = 'active' WHERE status IS NULL`
- **Data transformations**: Complex updates during schema changes

### Structural Changes

- **Table renames**: `ALTER TABLE old_name RENAME TO new_name`
- **Column renames**: `ALTER TABLE users RENAME COLUMN old_col TO new_col`
- **Splitting tables**: Creating new tables and migrating data

## Migration Lifecycle

### 1. Development

```bash
# Developer creates migration file
# Tests migration on local database
# Commits migration to version control
```

### 2. Code Review

```bash
# Team reviews migration code
# Checks for potential issues
# Approves or requests changes
```

### 3. Testing

```bash
# Migration tested on staging environment
# Rollback tested to ensure reversibility
# Integration tests verify application still works
```

### 4. Deployment

```bash
# Migration applied to production
# Monitored for any issues
# Rollback plan ready if needed
```

## Migration Types in Helium

### Forward Migrations

Apply changes to move database forward:

```lua
table.insert(queries, {
    sql = [=[
        CREATE TABLE ${SCHEMA}${TABLE} (
            id ${INTEGER} PRIMARY KEY,
            name ${VARCHAR_100}
        );
    ]=],
    description = "Create users table"
})
```

### Reverse Migrations

Undo forward changes for rollback:

```lua
table.insert(queries, {
    sql = [=[
        DROP TABLE ${SCHEMA}${TABLE};
    ]=],
    description = "Drop users table"
})
```

### Diagram Migrations

Provide metadata for database diagrams (Entity-Relationship Diagrams):

```lua
table.insert(queries, {
    sql = [=[
        {
            "diagram": [
                {
                    "object_type": "table",
                    "object_id": "table.users",
                    "table": [
                        {
                            "name": "id",
                            "datatype": "INTEGER",
                            "nullable": false,
                            "primary_key": true
                        }
                    ]
                }
            ]
        }
    ]=],
    description = "Users table diagram metadata"
})
```

## Best Practices

### Migration Design

1. **Atomic changes**: Each migration should be a single, logical change
2. **Reversible**: Always provide reverse migrations
3. **Idempotent**: Running multiple times should be safe
4. **Tested**: Test both forward and reverse migrations
5. **Documented**: Clear descriptions of what each migration does

### Naming Conventions

- **Descriptive names**: `add_user_email_column` not `migration_042`
- **Consistent numbering**: Sequential numbering across the project
- **Schema prefixes**: `acuranzo_1001`, `helium_2001`

### Safety Considerations

- **Backups**: Always backup before running migrations
- **Staging first**: Test on staging environment before production
- **Monitoring**: Watch for long-running migrations
- **Rollback plans**: Have rollback procedures ready

## Common Migration Scenarios

### Adding a New Feature

1. **Create migration**: Add new tables/columns needed for the feature
2. **Update application**: Modify code to use new schema
3. **Test thoroughly**: Ensure feature works with new schema
4. **Deploy together**: Migration and code changes deployed as a unit

### Refactoring Existing Schema

1. **Plan carefully**: Map out all changes needed
2. **Create migrations**: Forward migration for new structure
3. **Data migration**: Move data from old to new structure
4. **Reverse migration**: Handle rollback to old structure
5. **Test extensively**: Multiple rollback/forward cycles

### Fixing Data Issues

1. **Identify problem**: Understand the data inconsistency
2. **Create migration**: Script to fix the data
3. **Test on subset**: Test fix on sample data first
4. **Monitor execution**: Watch for unexpected side effects

## Migration Tools and Workflow

### Helium-Specific Tools

- **migration_index.sh**: Updates documentation with migration info
- **migration_update.sh**: Runs index updates across all schemas
- **database.lua**: Processes migrations with macro expansion

### Development Workflow

1. **Create migration file**: `acuranzo_1xxx.lua`
2. **Implement forward migration**: The main change
3. **Implement reverse migration**: For rollback capability
4. **Add diagram migration**: For schema visualization
5. **Test on all databases**: PostgreSQL, MySQL, SQLite, DB2
6. **Update documentation**: Run migration_update.sh
7. **Commit and deploy**: Through normal CI/CD process

## Troubleshooting Common Issues

### Migration Fails Mid-Execution

- **Problem**: Database left in inconsistent state
- **Solution**: Have rollback migration ready, or manual intervention script

### Long-Running Migrations

- **Problem**: Large data migrations take too long
- **Solution**: Break into smaller batches, add progress tracking

### Conflicting Changes

- **Problem**: Two developers create migrations with same number
- **Solution**: Use version control to coordinate, renumber as needed

### Environment Differences

- **Problem**: Migration works locally but fails in production
- **Solution**: Test on environment replicas, check for permission differences

## Further Reading

- [Database Migration Best Practices](https://en.wikipedia.org/wiki/Schema_migration)
- [Evolutionary Database Design](https://martinfowler.com/articles/evodb.html)
- [Refactoring Databases](https://databaserefactoring.com/)

Remember: Migrations are code that modifies your most critical asset (your data). Treat them with the same care and testing rigor as your application code!