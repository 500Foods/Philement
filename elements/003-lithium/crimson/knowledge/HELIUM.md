# Helium Database Schema Project

At a high level, Helium is essentially a colleciton of migration scripts that are used to configure a database schema in a particular database engine.
There's not much else here to interact with aside from these scripts. Essentially, you can connect to a database, apply the SQL that the scripts generate
that are specific to the database engine, and then you'll have a database with the current schema and any base or sample records included in the particular
schema you've applied (say, Acuranzo for example).  

The Hydrogen project has a built-in mechanism for doing all of this for you - point it at a database and turn on AutoMigrations in its config file, and it
will do the rest of the work, starting from an empty database. It even packages up the migrations and stores them in its payload so a single executable can be
distributed that can do everything - connect to a database, populate it, and be operational in one (big) step. Once running, Lithium can be configred to use it
by updating a similar config file to point wherever the Hydrogen server is at, and you're operational almost instantly.

The goal is to even package up Lithium into Hydrogen's payload and build a version of Hydrogen for the RaspberryPi platform so that you can just run it on,
say, a 3D printer, as a service and you've got 99% of what it takes to run Mainsail and Moonraker - all as a single executable.  

## Overview

The Helium project provides database schemas and migrations (collectively referred to as "designs") for the Hydrogen server system. It supports multiple database engines - currently including the following.

- [PostgreSQL 16+](https://www.postgresql.org/) - Advanced open source relational database
- [MariaDB 10.5+](https://mariadb.org/) - Enhanced, drop-in MySQL replacement[^mysql-note]
- [SQLite 3.35+](https://www.sqlite.org/) - Self-contained, serverless SQL database engine
- [IBM DB2 (LUW) 10+](https://www.ibm.com/products/db2-database) - Enterprise-grade relational database system

[^mysql-note]: The original intent was for MySQL/MariaDB support, but only MariaDB is included in the test environment. MySQL compatibility should be carefully evaluated if needd.

Within this project are the Lua scripts that make up the designs for each design/schema/migration.

- Creates tables and other database objects (DDL)
- Populates tables with default data (DML)
- Includes both forward and reverse migrations
- Diagram information is embedded in individaul migration files

The Hydrogen project is what uses these files. The migration files themsleves can be directly embedded into the Hydrogen executable payload. And they are included in the comprehensive test suite for the Hydrogen Project. That is also where the database diagram generation occurs, with the resulting SVG files landing here.

- [Test 30 - Database Subsystem Test](/docs/H/tests/test_30_database.md) - Validates Database Queue Manager (DQM) startup and functionality across multiple database engines
- [Test 31 - Migration Validation](/docs/H/tests/test_31_migrations.md) - Performs static analysis on database migration files using SQL linting tools
- [Test 32 - PostgreSQL Migrations](/docs/H/tests/test_32_postgres_migrations.md) - Tests PostgreSQL database migration performance and execution
- [Test 33 - MySQL Migrations](/docs/H/tests/test_33_mysql_migrations.md) - Tests MySQL/MariaDB database migration performance and execution
- [Test 34 - SQLite Migrations](/docs/H/tests/test_34_sqlite_migrations.md) - Tests SQLite database migration performance and execution
- [Test 35 - DB2 Migrations](/docs/H/tests/test_35_db2_migrations.md) - Tests IBM DB2 database migration performance and execution
- [Test 71 - Database Diagrams](/docs/H/tests/test_71_database_diagrams.md) - Generates SVG database diagrams for all supported database engine and design combinations
- [Test 98 - Lua Code Analysis](/docs/H/tests/test_98_luacheck.md) - Performs static analysis on Lua source files using luacheck tool

IN addtion, [Test 01 - Compilation](/docs/H/tests/test_01_compilation.md) builds the Hydrgoen payload which typically includes the migration files. As part of the build step, it checks to see if the migrations files have been updated more recently than the paylod. If so, it regenerates the payload and also runs the [helium_update.sh](/elements/002-helium/scripts/helium_update.sh) script.

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

### License

- [**MIT License**](/docs/He/LICENSE.md) - Standard open-source license

## Repository Information

Generated 2026-Mar-28 (Sat) 07:01:32 PDT

```cloc
github.com/AlDanial/cloc v 2.08  T=3.32 s (105.8 files/s, 57780.9 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
SVG                            133            266           3745         142386
Lua                            186           4457           3464          30997
Markdown                        29           1462              0           3818
Bourne Shell                     3            152            160            814
-------------------------------------------------------------------------------
SUM:                           351           6337           7369         178015
-------------------------------------------------------------------------------
```
