# Helium Database Schema Project

The Helium project provides database schemas and migrations for the Hydrogen server system. It supports multiple database engines and enables schema-agnostic database operations.

## Table of Contents

### Getting Started

- [**Quick Start Tutorial**](/docs/He/QUICKSTART.md) - 10-minute tutorial for your first migration
- [**Getting Started Guide**](/docs/He/GETTING_STARTED.md) - Complete setup and learning path
- [**Setup Guide**](/docs/He/SETUP.md) - Detailed environment setup instructions
- [**Migration Creation Guide**](/docs/He/GUIDE.md) - Complete guide for creating new migrations with examples

### Fundamentals

- [**Prerequisites**](/docs/He/SETUP.md#prerequisites) - Required tools and knowledge
- [**Lua Introduction**](/docs/He/LUA_INTRO.md) - Lua basics for migration development
- [**Migrations Introduction**](/docs/He/MIGRATIONS_INTRO.md) - Database migration concepts
- [**Supported Databases**](/docs/He/DATABASES.md) - Database engine compatibility and setup
- [**Key Features**](/docs/He/FEATURES.md) - Overview of Helium's capabilities

### Advanced Topics

- [**Macro Reference**](/docs/He/MACRO_REFERENCE.md) - Complete list of all available macros with expansions
- [**Migration Anatomy**](/docs/He/MIGRATION_ANATOMY.md) - Deep dive into migration structure and state transitions
- [**Environment Variables**](/docs/He/ENVIRONMENT_VARIABLES.md) - Runtime variables available in migrations
- [**Brotli Compression**](/docs/He/BROTLI_COMPRESSION.md) - Automatic compression for large content in migrations
- [**Testing Guide**](/docs/He/TESTING_GUIDE.md) - How to validate, test, and debug migrations

### Scripts & Configuration

- [**migration_index.sh**](/docs/He/SCRIPTS/migration_index.md) ([source](/elements/002-helium/scripts/migration_index.sh)) - Generates migration index tables for README files
- [**helium_update.sh**](/docs/He/SCRIPTS/helium_update.md) ([source](/elements/002-helium/scripts/helium_update.sh)) - Updates migration indexes across all schemas as well as other repository updates and checks
- [**github-sitemap.sh**](/docs/He/SCRIPTS/github-sitemap.md) ([source](/elements/002-helium/scripts/github-sitemap.sh)) - Checks markdown links and identifies orphaned files

### Database Configuration

- [**database.lua**](/docs/He/DATABASES/database.md) (sample: [Acuranzo](/elements/002-helium/acuranzo/migrations/database.lua)) - Core database framework and macro system
- [**database_postgresql.lua**](/docs/He/DATABASES/database_postgresql.md) (sample: [Acuranzo](/elements/002-helium/acuranzo/migrations/database_postgresql.lua)) - PostgreSQL-specific configuration
- [**database_mysql.lua**](/docs/He/DATABASES/database_mysql.md) (sample: [Acuranzo](/elements/002-helium/acuranzo/migrations/database_mysql.lua)) - MySQL/MariaDB-specific configuration
- [**database_sqlite.lua**](/docs/He/DATABASES/database_sqlite.md) (sample: [Acuranzo](/elements/002-helium/acuranzo/migrations/database_sqlite.lua)) - SQLite-specific configuration
- [**database_db2.lua**](/docs/He/DATABASES/database_db2.md) (sample: [Acuranzo](/elements/002-helium/acuranzo/migrations/database_db2.lua)) - IBM DB2-specific configuration

### Schemas

Each schema provides a complete database design with migrations, supporting PostgreSQL, MySQL/MariaDB, SQLite, and IBM DB2.

- [**Main Helium README**](/elements/002-helium/README.md) - Overview of all schemas and scripts
- [**Acuranzo Schema**](/elements/002-helium/acuranzo/README.md) - Frontend web application schema
- [**GAIUS Schema**](/elements/002-helium/gaius/README.md) - GAIUS project database design
- [**GLM Schema**](/elements/002-helium/glm/README.md) - Greenhouse Labour Management schema
- [**Helium Schema**](/elements/002-helium/helium/README.md) - 3D printing focused schema

### Integration

- [**Hydrogen Integration**](/docs/He/INTEGRATION.md) - How Helium works with the Hydrogen server system

## Repository Information

Generated 2025-Dec-28 (Sun) 21:40:25 PST

```cloc
github.com/AlDanial/cloc v 2.06  T=0.42 s (338.8 files/s, 70709.2 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Lua                            112           2592           2635          18357
Markdown                        28           1442              0           3689
Bourne Shell                     3            152            160            814
-------------------------------------------------------------------------------
SUM:                           143           4186           2795          22860
-------------------------------------------------------------------------------
```
