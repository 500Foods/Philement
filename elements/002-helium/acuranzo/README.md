# Acuranzo Database Design

This folder contains the migrations (aka database DDL and SQL) for createing a new insttance of an Acuranzo database. Current supported engines include SQLite, PostgreSQL, MySQL, and IBM DB2.

## Design Notes

1. Some tables do not have a primary key, such as "sessions".  This is deliberate.
2. Scripts assume that working Base64 Decode funtionality is present across all engines.
3. Scripts assume that working JSON_INGEST functionality is present across all engines.
4. Database engines currently supported use the labels 'postgresql', 'mysql', 'sqlite', and 'db2'.

## Database Files

| File | Purpose |
|------|---------|
| [`database.lua`](migrations/database.lua) | Defines the Helium schema, supported database engines, and SQL defaults for migrations used in Hydrogen |
| [`database_mysql.lua`](migrations/database_mysql.lua) | MySQL-specific database configuration |
| [`database_postgresql.lua`](migrations/database_postgresql.lua) | PostgreSQL-specific database configuration |
| [`database_sqlite.lua`](migrations/database_sqlite.lua) | SQLite-specific database configuration |
| [`database_db2.lua`](migrations/database_db2.lua) | IBM DB2-specific database configuration |

## Migrations

| M# | Table | Version | Released | Migrations | Diagram | Description |
|----|-------|---------|----------|------------|---------|-------------|
| [1000](migrations/acuranzo_1000.lua) | queries | 3.1.0 | 2025-11-23 | 11 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| [1001](migrations/acuranzo_1001.lua) | lookups | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the lookups table |
| [1002](migrations/acuranzo_1002.lua) | account_access | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the account_access table |
| [1003](migrations/acuranzo_1003.lua) | account_contacts | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the account_contacts table |
| [1004](migrations/acuranzo_1004.lua) | account_roles | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the account_roles table |
| [1005](migrations/acuranzo_1005.lua) | accounts | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the accounts table |
| [1006](migrations/acuranzo_1006.lua) | actions | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the actions table |
| [1007](migrations/acuranzo_1007.lua) | connections | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the connections table |
| [1008](migrations/acuranzo_1008.lua) | convos | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the convos table |
| [1009](migrations/acuranzo_1009.lua) | dictionaries | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the dictionaries table |
| [1010](migrations/acuranzo_1010.lua) | documents | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the documents table |
| [1011](migrations/acuranzo_1011.lua) | languages | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the languages table |
| [1012](migrations/acuranzo_1012.lua) | licenses | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the licenses table |
| [1013](migrations/acuranzo_1013.lua) | lists | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the lists table |
| [1014](migrations/acuranzo_1014.lua) | notes | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the notes table |
| [1015](migrations/acuranzo_1015.lua) | reports | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the reports table |
| [1016](migrations/acuranzo_1016.lua) | roles | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the roles table |
| [1017](migrations/acuranzo_1017.lua) | rules | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the rules table |
| [1018](migrations/acuranzo_1018.lua) | sessions | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the sessions table |
| [1019](migrations/acuranzo_1019.lua) | systems | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the systems table |
| [1020](migrations/acuranzo_1020.lua) | templates | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the templates table |
| [1021](migrations/acuranzo_1021.lua) | tokens | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the tokens table |
| [1022](migrations/acuranzo_1022.lua) | workflow_steps | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the workflow_steps table |
| [1023](migrations/acuranzo_1023.lua) | workflows | 3.1.0 | 2025-11-23 | 9 | ✓ | Creates the workflows table |
| [1024](migrations/acuranzo_1024.lua) | accounts | 1.0.0 | 2025-11-11 | 6 |  | Creates administrator account |
| [1025](migrations/acuranzo_1025.lua) | lookups | 1.0.0 | 2025-11-20 | 6 |  | Defaults for Lookup 000 - Lookup Items |
| [1026](migrations/acuranzo_1026.lua) | lookups | 1.0.0 | 2025-11-21 | 10 |  | Defaults for Lookup 001 - Lookup Status |
| [1027](migrations/acuranzo_1027.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 002 - Language Status |
| [1028](migrations/acuranzo_1028.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 003 - Dictionary Status |
| [1029](migrations/acuranzo_1029.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 004 - Connection Type |
| [1030](migrations/acuranzo_1030.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 005 - Connected Status |
| [1031](migrations/acuranzo_1031.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 006 - Connection Status |
| [1032](migrations/acuranzo_1032.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 007 - Rule Status |
| [1033](migrations/acuranzo_1033.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 008 - Note Status |
| [1034](migrations/acuranzo_1034.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 009 - Entity Type |
| [1035](migrations/acuranzo_1035.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 010 - Workflow Status |
| [1036](migrations/acuranzo_1036.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 011 - Workflow Step Status |
| [1037](migrations/acuranzo_1037.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 010 - Template Status |
| [1038](migrations/acuranzo_1038.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 013 - License Status |
| [1039](migrations/acuranzo_1039.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 014 - System Status |
| [1040](migrations/acuranzo_1040.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 015 - System Access Type |
| [1041](migrations/acuranzo_1041.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 016 - Account Access Type |
| [1042](migrations/acuranzo_1042.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 017 - IANA Timezone |
| [1043](migrations/acuranzo_1043.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 018 - Contact Type |
| [1044](migrations/acuranzo_1044.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 019 - Authenticate |
| [1045](migrations/acuranzo_1045.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 020 - Contact Status |
| [1046](migrations/acuranzo_1046.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 021 - Feature |
| [1047](migrations/acuranzo_1047.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 022 - Access Type |
| [1048](migrations/acuranzo_1048.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 023 - Access Status |
| [1049](migrations/acuranzo_1049.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 024 - Action Type |
| [1050](migrations/acuranzo_1050.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 025 - Session Status |
| [1051](migrations/acuranzo_1051.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 026 - Session Flag |
| [1052](migrations/acuranzo_1052.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 027 - Query Status |
| [1053](migrations/acuranzo_1053.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 028 - Query Type |
| [1054](migrations/acuranzo_1054.lua) | lookups | 1.0.0 | 2025-11-22 | 10 |  | Defaults for Lookup 029 - Query Quadrant |
| **55** | | | | **520** | **24** | |
