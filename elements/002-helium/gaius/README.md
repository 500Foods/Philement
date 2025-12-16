# GAIUS Database Design

This is the database schema for the [GAIUS](http://gaiusmodel.com) database. This is mostly an add-on to the Acuranzo database, outfitting it with additional DDL and SQL specific to the GAIUS project. The project overall involves a lot of Lua work, so you're likely to find some Lua-specific UDFs in the mix.

## Database Files

| File | Purpose |
|------|---------|
| [`database.lua`](migrations/database.lua) | Converts migration files to engine-specific SQL using macros and fancy formatting tricks |
| [`database_mysql.lua`](migrations/database_mysql.lua) | MySQL-specific database configuration |
| [`database_postgresql.lua`](migrations/database_postgresql.lua) | PostgreSQL-specific database configuration |
| [`database_sqlite.lua`](migrations/database_sqlite.lua) | SQLite-specific database configuration |
| [`database_db2.lua`](migrations/database_db2.lua) | IBM DB2-specific database configuration |

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
|----|-------|---------|---------|-------|---------|-------------|
| [2000](/elements/002-helium/gaius/migrations/gaius_2000.lua) | queries | 3.1.0 | 2025-11-23 | 9 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| **1** | | | | **9** | **1** | |
