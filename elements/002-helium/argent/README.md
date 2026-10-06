# Argent Database Design

Argent is optional double-entry bookkeeping on the **Acuranzo database**. It is a second set of migrations in the same schema, using the same `queries`, `lookups`, and `scripts` tables. It is never applied by itself.

Allowed packs on that database:

- Acuranzo alone (1xxx)
- Acuranzo + Argent (1xxx, then 2xxx)
- Acuranzo + Gaius (1xxx, then 3xxx) — Gaius pack is later
- Acuranzo + Gaius + Argent

Gaius alone and Argent alone are refused. The on-disk `elements/002-helium/gaius/` tree still bootstraps its own `queries` table at `gaius_2000.lua`. That file is not this pack. When Gaius joins the Acuranzo database, that pack is numbered 3xxx. GLM and the Helium printing design stay on their own databases.

Primary target is PostgreSQL 15+ via YugabyteDB. Authoritative guidance is in `/docs/He/GUIDE.md`, `/docs/He/MIGRATION_ANATOMY.md`, and `/docs/He/MACRO_REFERENCE.md`. The plan is [`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md).

`database*.lua` here is a copy of the Acuranzo macro set (including MariaDB and MSSQL). The copy stays. The payload packs it as `argent/database*.lua`, and the loader uses that copy when it runs an `argent_*.lua` file.

Lookup ids **200–299** are reserved for Argent on the shared `lookups` table. `organizations.status_a200` is lookup 200. `ledgers.ledger_type_a201` is lookup 201 (asset, liability, equity, income, expense). `ledgers.status_a202` is lookup 202 (open, closed, archive). Those three seeds are later migrations.

## Database Files

| File | Purpose |
| ------ | --------- |
| [`database.lua`](migrations/database.lua) | Converts migration files to engine-specific SQL using macros and fancy formatting tricks |
| [`database_mysql.lua`](migrations/database_mysql.lua) | MySQL-specific database configuration |
| [`database_mariadb.lua`](migrations/database_mariadb.lua) | MariaDB-specific database configuration |
| [`database_postgresql.lua`](migrations/database_postgresql.lua) | PostgreSQL-specific database configuration |
| [`database_sqlite.lua`](migrations/database_sqlite.lua) | SQLite-specific database configuration |
| [`database_db2.lua`](migrations/database_db2.lua) | IBM DB2-specific database configuration |
| [`database_firebird.lua`](migrations/database_firebird.lua) | Firebird-specific database configuration |
| [`database_mssql.lua`](migrations/database_mssql.lua) | MS SQL Server-specific database configuration |

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
| ---- | ------- | --------- | --------- | ------- | --------- | ------------- |
| [2000](/elements/002-helium/argent/migrations/argent_2000.lua) | organizations | 1.0.0 | 2026-10-05 | 6 | ✓ | Creates the organizations table |
| [2001](/elements/002-helium/argent/migrations/argent_2001.lua) | ledgers | 1.0.0 | 2026-10-05 | 6 | ✓ | Creates the ledgers table |
| **2** | | | | **12** | **2** | |
