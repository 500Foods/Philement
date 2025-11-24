# Acuranzo Database Design

This folder contains the migrations (aka database DDL and SQL) for creating a new instance of an Acuranzo database. Current supported engines include PostgreSQL, MySQL, SQLite, and IBM DB2.

## Design Notes

1. The [migration_index.sh](migration_index.sh) script can be used to automatically update the Migrations table below.
1. Some tables do not have a primary key, such as "sessions".  This is deliberate.
1. Scripts assume that working Base64 Decode funtionality is present across all engines.
1. Scripts assume that working JSON_INGEST functionality is present across all engines.
1. Database engines currently supported use the labels 'postgresql', 'mysql', 'sqlite', and 'db2'.
1. Each Lua migration script is focused on one element, and contains both FORWARD and REVERSE migrations.
1. Migrations that change the schema also include a DIAGRAM migration.
1. Query and Subquery delimiters are required, particularly for DB2.
1. Migrations are processed as individual transactions. Any errors at all rollback and stop the migration process.
1. Migrations are loaded into the Hydrogen Payload, so it should be updated after changes are made.
1. The Stmts count reflects the number of queries and subqueries included in each migration.

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
| [1000](acuranzo/migrations/acuranzo_1000.lua) | queries | 3.1.0 | 2025-11-23 | 9 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| [1001](acuranzo/migrations/acuranzo_1001.lua) | lookups | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the lookups table |
| [1002](acuranzo/migrations/acuranzo_1002.lua) | account_access | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the account_access table |
| [1003](acuranzo/migrations/acuranzo_1003.lua) | account_contacts | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the account_contacts table |
| [1004](acuranzo/migrations/acuranzo_1004.lua) | account_roles | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the account_roles table |
| [1005](acuranzo/migrations/acuranzo_1005.lua) | accounts | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the accounts table |
| [1006](acuranzo/migrations/acuranzo_1006.lua) | actions | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the actions table |
| [1007](acuranzo/migrations/acuranzo_1007.lua) | connections | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the connections table |
| [1008](acuranzo/migrations/acuranzo_1008.lua) | convos | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the convos table |
| [1009](acuranzo/migrations/acuranzo_1009.lua) | dictionaries | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the dictionaries table |
| [1010](acuranzo/migrations/acuranzo_1010.lua) | documents | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the documents table |
| [1011](acuranzo/migrations/acuranzo_1011.lua) | languages | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the languages table |
| [1012](acuranzo/migrations/acuranzo_1012.lua) | licenses | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the licenses table |
| [1013](acuranzo/migrations/acuranzo_1013.lua) | lists | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the lists table |
| [1014](acuranzo/migrations/acuranzo_1014.lua) | notes | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the notes table |
| [1015](acuranzo/migrations/acuranzo_1015.lua) | reports | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the reports table |
| [1016](acuranzo/migrations/acuranzo_1016.lua) | roles | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the roles table |
| [1017](acuranzo/migrations/acuranzo_1017.lua) | rules | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the rules table |
| [1018](acuranzo/migrations/acuranzo_1018.lua) | sessions | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the sessions table |
| [1019](acuranzo/migrations/acuranzo_1019.lua) | systems | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the systems table |
| [1020](acuranzo/migrations/acuranzo_1020.lua) | templates | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the templates table |
| [1021](acuranzo/migrations/acuranzo_1021.lua) | tokens | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the tokens table |
| [1022](acuranzo/migrations/acuranzo_1022.lua) | workflow_steps | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the workflow_steps table |
| [1023](acuranzo/migrations/acuranzo_1023.lua) | workflows | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the workflows table |
| [1024](acuranzo/migrations/acuranzo_1024.lua) | accounts | 1.0.0 | 2025-11-11 | 4 |  | Creates administrator account |
| [1025](acuranzo/migrations/acuranzo_1025.lua) | lookups | 1.0.0 | 2025-11-20 | 4 |  | Defaults for Lookup 000 - Lookup Items |
| [1026](acuranzo/migrations/acuranzo_1026.lua) | lookups | 1.0.0 | 2025-11-21 | 6 |  | Defaults for Lookup 001 - Lookup Status |
| [1027](acuranzo/migrations/acuranzo_1027.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 002 - Language Status |
| [1028](acuranzo/migrations/acuranzo_1028.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 003 - Dictionary Status |
| [1029](acuranzo/migrations/acuranzo_1029.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 004 - Connection Type |
| [1030](acuranzo/migrations/acuranzo_1030.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 005 - Connected Status |
| [1031](acuranzo/migrations/acuranzo_1031.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 006 - Connection Status |
| [1032](acuranzo/migrations/acuranzo_1032.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 007 - Rule Status |
| [1033](acuranzo/migrations/acuranzo_1033.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 008 - Note Status |
| [1034](acuranzo/migrations/acuranzo_1034.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 009 - Entity Type |
| [1035](acuranzo/migrations/acuranzo_1035.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 010 - Workflow Status |
| [1036](acuranzo/migrations/acuranzo_1036.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 011 - Workflow Step Status |
| [1037](acuranzo/migrations/acuranzo_1037.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 012 - Template Status |
| [1038](acuranzo/migrations/acuranzo_1038.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 013 - License Status |
| [1039](acuranzo/migrations/acuranzo_1039.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 014 - System Status |
| [1040](acuranzo/migrations/acuranzo_1040.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 015 - System Access Type |
| [1041](acuranzo/migrations/acuranzo_1041.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 016 - Account Access Type |
| [1042](acuranzo/migrations/acuranzo_1042.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 017 - IANA Timezone |
| [1043](acuranzo/migrations/acuranzo_1043.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 018 - Contact Type |
| [1044](acuranzo/migrations/acuranzo_1044.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 019 - Authenticate |
| [1045](acuranzo/migrations/acuranzo_1045.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 020 - Contact Status |
| [1046](acuranzo/migrations/acuranzo_1046.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 021 - Feature |
| [1047](acuranzo/migrations/acuranzo_1047.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 022 - Access Type |
| [1048](acuranzo/migrations/acuranzo_1048.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 023 - Access Status |
| [1049](acuranzo/migrations/acuranzo_1049.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 024 - Action Type |
| [1050](acuranzo/migrations/acuranzo_1050.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 025 - Session Status |
| [1051](acuranzo/migrations/acuranzo_1051.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 026 - Session Flag |
| [1052](acuranzo/migrations/acuranzo_1052.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 027 - Query Status |
| [1053](acuranzo/migrations/acuranzo_1053.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 028 - Query Type |
| [1054](acuranzo/migrations/acuranzo_1054.lua) | lookups | 1.0.0 | 2025-11-22 | 6 |  | Defaults for Lookup 029 - Query Quadrant |
| **55** | | | | **329** | **24** | |
