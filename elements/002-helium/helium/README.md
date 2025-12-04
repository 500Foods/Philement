# Helium Database Design

The Helium migration is intended for use within the 3D printing environment. This could be used by someone with a single printer, in an embedded capacity, typically with SQLite. Or it could be used to host substantially larger datasets like broad printer part assemblies. Or maybe it could be used for someone with a print farm, managing jobs and that ssort of thing. Filament tracking also comes to mind.

Early days, so lots of things could find their way into here.

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
| [4000](helium/migrations/helium_4000.lua) | queries | 3.1.0 | 2025-11-23 | 9 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| **1** | | | | **9** | **1** | |
