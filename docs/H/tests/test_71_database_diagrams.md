# Test 71: Database Diagram Generation

## CHANGELOG

- **3.0.0** (2026-01-01): Major restructuring - diagrams now generated in `elements/002-helium/{design}/diagrams/{engine}/` directories with simplified filename format `{design}-{engine}-{migration}.svg`
- **2.0.0** (2025-11-17): Fixed diagram numbering bug, added before/after highlighting, generates diagrams for ALL migrations (not just latest)
- **1.2.0** (2025-11-15): Fixed filename generation to use last_diagram_migration from metadata
- **1.1.0** (2025-09-30): Added metadata, starting with 'Tables included' to output
- **1.0.0** (2025-09-29): Initial creation for database diagram generation

## Overview

Test 71 generates SVG database diagrams for all supported database engine and design combinations. This test creates visual representations of database schemas using migration files from the Helium project and stores them in the appropriate Helium design directories.

## Purpose

- **Schema Visualization**: Creates visual diagrams of database structures
- **Documentation**: Provides graphical representations for documentation
- **Validation**: Ensures migration files can be processed into diagrams
- **Multi-Engine Support**: Tests diagram generation across PostgreSQL, SQLite, MySQL, and DB2

## Test Configuration

- **Test Name**: Database Diagrams
- **Test Abbreviation**: ERD
- **Test Number**: 71
- **Version**: 3.0.0

## What It Does

1. **Migration Discovery**: Finds the highest-numbered migration file for each design
2. **Parallel Generation**: Creates diagrams for all engine/design combinations simultaneously
3. **SVG Output**: Generates scalable vector graphics in the `elements/002-helium/{design}/diagrams/{engine}/` directories
4. **Metadata Collection**: Captures table counts and other statistics
5. **Result Analysis**: Reports success/failure for each combination

## Supported Combinations

### Designs

- **acuranzo**: Main application schema
- **helium**: Core system schema (planned)
- **glm**: GLM2 schema (planned)
- **gaius**: GAIUS schema (planned)

### Database Engines

- **PostgreSQL**: Full-featured relational database
- **SQLite**: Embedded database engine
- **MySQL**: Popular open-source database
- **DB2**: Enterprise-grade database system

## Output Structure

Diagrams are saved in: `elements/002-helium/{design}/diagrams/{engine}/{design}-{engine}-{migration}.svg`

Example files:

```files
elements/002-helium/acuranzo/diagrams/postgresql/acuranzo-postgresql-1027.svg
elements/002-helium/acuranzo/diagrams/mysql/acuranzo-mysql-1027.svg
elements/002-helium/acuranzo/diagrams/db2/acuranzo-db2-1027.svg
elements/002-helium/acuranzo/diagrams/sqlite/acuranzo-sqlite-1027.svg
```

## Metadata Features

Each diagram generation includes:

- **Table Count**: Number of tables in the schema
- **Migration Number**: Version of the migration used
- **Engine-Specific**: Optimized for each database engine's syntax
- **Schema Support**: Handles multiple schemas within designs

## Test Output

The test provides:

- **Combination Count**: Total number of diagrams to generate
- **Success/Failure**: Results for each engine/design pair
- **Table Statistics**: Number of tables included in each diagram
- **File Locations**: Paths to generated SVG files

## Dependencies

- **Helium Project**: Access to migration files in `elements/002-helium/` and writes output to `elements/002-helium/{design}/diagrams/{engine}/`
- **generate_diagram.sh**: Script that processes migrations into diagrams
- **Database Tools**: Access to database engines for schema extraction
- **SVG Libraries**: Tools for generating vector graphics

## Integration

This test is part of the Hydrogen testing framework and runs automatically with:

```bash
# Run this specific test
./tests/test_71_database_diagrams.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [Test Framework Overview](/docs/H/tests/TESTING.md)
- [Database Tests](/docs/H/tests/test_30_database.md)
- [Migration Tests](/docs/H/tests/test_31_migrations.md)
- [Database Architecture](/docs/H/core/subsystems/database/database.md)