# Test 39: Database Diagram Generation

## Overview

Test 39 generates SVG database diagrams for all supported database engine and design combinations. This test creates visual representations of database schemas using migration files from the Helium project.

## Purpose

- **Schema Visualization**: Creates visual diagrams of database structures
- **Documentation**: Provides graphical representations for documentation
- **Validation**: Ensures migration files can be processed into diagrams
- **Multi-Engine Support**: Tests diagram generation across PostgreSQL, SQLite, MySQL, and DB2

## Test Configuration

- **Test Name**: Database Diagrams
- **Test Abbreviation**: ERD
- **Test Number**: 39
- **Version**: 1.1.0

## What It Does

1. **Migration Discovery**: Finds the highest-numbered migration file for each design
2. **Parallel Generation**: Creates diagrams for all engine/design combinations simultaneously
3. **SVG Output**: Generates scalable vector graphics in the `images/` directory
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

Diagrams are saved in: `images/{design}/{design}-{engine}{schema}-{migration}.svg`

Example files:

```files
images/acuranzo/acuranzo-postgresql-app-042.svg
images/acuranzo/acuranzo-sqlite--042.svg
images/acuranzo/acuranzo-mysql-acuranzo-042.svg
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

- **Helium Project**: Access to migration files in `elements/002-helium/`
- **generate_diagram.sh**: Script that processes migrations into diagrams
- **Database Tools**: Access to database engines for schema extraction
- **SVG Libraries**: Tools for generating vector graphics

## Integration

This test is part of the Hydrogen testing framework and runs automatically with:

```bash
# Run this specific test
./tests/test_39_database_diagrams.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [Test Framework Overview](../README.md)
- [Database Tests](../docs/test_30_database.md)
- [Migration Tests](../docs/test_31_migrations.md)
- [Database Architecture](../../docs/database.md)