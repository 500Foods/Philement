# GAIUS Database Design

This is the database schema for the [GAIUS](http://gaiusmodel.com) database. This is mostly an add-on to the Acuranzo database, outfitting it with additional DDL and SQL specific to the GAIUS project. The project overall involves a lot of Lua work, so you're likely to find some Lua-specific UDFs in the mix.

Primary database target for the overall Helium project is PostgreSQL 15+ via YugabyteDB. See `/docs/He/README.md`, `/docs/He/GUIDE.md` (especially the AI/LLM migration section), and `/docs/He/MIGRATION_ANATOMY.md`.

## Database Files

Macro scripts are shared. They live in [`/elements/002-helium/helium/`](/elements/002-helium/helium/): `database.lua` and one `database_<engine>.lua` for PostgreSQL, MySQL, MariaDB, SQLite, DB2, Firebird, and SQL Server. This design does not keep a copy. Gaius still bootstraps its own `queries` table in `gaius_2000.lua`.

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
| ---- | ------- | --------- | --------- | ------- | --------- | ------------- |
| [2000](/elements/002-helium/gaius/migrations/gaius_2000.lua) | queries | 3.4.0 | 2026-09-19 | 10 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| **1** | | | | **10** | **1** | |
