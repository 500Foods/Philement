# Key Features

The Helium project provides a comprehensive database migration and schema management system with several powerful features designed for modern application development.

## Multi-Engine Support

Single migration files work seamlessly across all supported database engines:

- **PostgreSQL** v16/v17 - Primary development platform
- **MySQL/MariaDB** - Widely tested alternative
- **SQLite** v3 - Embedded applications
- **IBM DB2** v10-12 - Enterprise deployments

The same migration produces database-appropriate SQL for each engine without modification.

## Macro System

Database-agnostic SQL generation using `${MACRO}` placeholders:

```lua
-- Works on all databases
CREATE TABLE ${SCHEMA}${TABLE} (
    id ${INTEGER} PRIMARY KEY,
    name ${VARCHAR_100} NOT NULL,
    created_at ${TIMESTAMP_TZ} DEFAULT ${NOW}
);
```

Macros automatically expand to the correct syntax for each database engine.

## Content Embedding

Direct inclusion of complex content without escaping:

### SQL

```lua
[=[
    CREATE TABLE users (
        id INTEGER PRIMARY KEY,
        profile_data JSON NOT NULL
    );

    INSERT INTO users (profile_data) VALUES (
        '{"name": "John", "preferences": {"theme": "dark"}}'::jsonb
    );
]=]
```

### JSON

```lua
[=[
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
]=]
```

### Markdown and Other Formats

```lua
[=[
    # Migration Documentation

    This migration creates the users table with the following features:
    - Primary key with auto-increment
    - JSON profile storage
    - Audit timestamp fields
]=]
```

## Migration Types

Three types of migrations for complete database lifecycle management:

### Forward Migrations

Apply schema and data changes to move forward in the migration sequence.

### Reverse Migrations

Undo forward migrations for rollback and testing capabilities.

### Diagram Migrations

Generate Entity-Relationship Diagrams (ERDs) with JSON metadata describing table structures and relationships.

## Automatic Content Processing

Intelligent handling of embedded content:

### Base64 Encoding

Large content blocks (>1KB) are automatically Base64 encoded for database storage.

### Brotli Compression

Content exceeding 1KB is compressed using Brotli for efficient storage and transfer.

### Environment Variables

Runtime injection of sensitive or environment-specific values:

```lua
-- Migration
INSERT INTO users (username, password_hash) VALUES (
    '${ADMIN_USERNAME}',
    '${ADMIN_PASSHASH}'
);

-- Runtime environment
export ADMIN_USERNAME="admin"
export ADMIN_PASSHASH="sha256_hash_here"
```

## Built-in Testing and Validation

Comprehensive validation ensures migration reliability:

### Syntax Validation

- Lua syntax checking
- Macro expansion verification
- SQL syntax validation per database engine

### Migration Testing

- Forward and reverse migration testing
- State transition validation
- Rollback capability verification

### Cross-Engine Compatibility

- Automated testing across all supported databases
- Macro expansion validation
- Content encoding/decoding verification

## Documentation Generation

Automated documentation updates:

### README Updates

Migration index tables automatically updated in schema READMEs.

### Diagram Generation

SVG Entity-Relationship Diagrams created from migration metadata.

### Cross-References

Automatic linking between migrations, schemas, and documentation.

## Lua-Based Architecture

### Lightweight Integration

- Minimal runtime footprint
- Easy embedding in larger applications
- No external dependencies beyond Lua standard library

### String Processing Power

- Excellent multiline string support with `[=[...]=]` syntax
- Advanced pattern matching and templating
- UTF-8 support for international content

### Functional Programming

- Migrations as pure functions
- Immutable configuration objects
- Predictable execution model

## Schema Agnostic Design

### Dynamic Schema Management

- No hardcoded table structures
- Runtime schema discovery
- Flexible metadata-driven operations

### Migration Metadata

- Complete audit trail of all changes
- State tracking for rollback capabilities
- Relationship mapping for diagrams

### Content Type Agnostic

- Support for SQL, JSON, Markdown, CSS, and other formats
- Automatic encoding based on content type
- Extensible processing pipeline

## Enterprise-Ready Features

### Transaction Safety

- Atomic migration execution
- Rollback on failure
- State consistency guarantees

### Performance Optimization

- Query queuing system (Slow/Medium/Fast/Cached)
- Timeout management
- Connection pooling support

### Security Features

- Environment variable isolation
- Content encoding for sensitive data
- Audit trail for compliance

## Developer Experience

### Comprehensive Tooling

- Migration creation templates
- Validation and linting tools
- Testing frameworks

### Clear Documentation

- Step-by-step tutorials
- Complete API reference
- Troubleshooting guides

### Community Support

- Open source development
- Active maintenance
- Extensive example library

These features combine to create a robust, flexible, and developer-friendly database migration system suitable for projects ranging from small applications to large-scale enterprise deployments.