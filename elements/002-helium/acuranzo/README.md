# Acuranzo Database Design

This folder contains the migrations (aka database DDL and SQL) for creating a new instance of an Acuranzo database. Current supported engines include PostgreSQL, MySQL, SQLite, and IBM DB2.

 The [migration_index.sh](../scripts/migration_index.sh) script can be used to automatically update the Migrations table below - [helium_update.sh](../scripts/helium_update.sh) will update all schema documents as well as perform other repository housekeeping tasks.

This is a summary of the migrations included. Details about individual migrations should be supplied
in the migrations themselves so that they get populated in the database directly.

## Design Notes

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
| ------ | --------- |
| [`database.lua`](migrations/database.lua) | Converts migration files to engine-specific SQL using macros and fancy formatting tricks |
| [`database_mysql.lua`](migrations/database_mysql.lua) | MySQL-specific database configuration |
| [`database_postgresql.lua`](migrations/database_postgresql.lua) | PostgreSQL-specific database configuration |
| [`database_sqlite.lua`](migrations/database_sqlite.lua) | SQLite-specific database configuration |
| [`database_db2.lua`](migrations/database_db2.lua) | IBM DB2-specific database configuration |

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
| ---- | ------- | --------- | --------- | ------- | --------- | ------------- |
| [1000](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua) | queries | 4.0.0 | 2025-11-27 | 13 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| [1001](/elements/002-helium/acuranzo/migrations/acuranzo_1001.lua) | lookups | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the lookups table |
| [1002](/elements/002-helium/acuranzo/migrations/acuranzo_1002.lua) | account_access | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the account_access table |
| [1003](/elements/002-helium/acuranzo/migrations/acuranzo_1003.lua) | account_contacts | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the account_contacts table |
| [1004](/elements/002-helium/acuranzo/migrations/acuranzo_1004.lua) | account_roles | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the account_roles table |
| [1005](/elements/002-helium/acuranzo/migrations/acuranzo_1005.lua) | accounts | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the accounts table |
| [1006](/elements/002-helium/acuranzo/migrations/acuranzo_1006.lua) | actions | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the actions table |
| [1007](/elements/002-helium/acuranzo/migrations/acuranzo_1007.lua) | connections | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the connections table |
| [1008](/elements/002-helium/acuranzo/migrations/acuranzo_1008.lua) | convos | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the convos table |
| [1009](/elements/002-helium/acuranzo/migrations/acuranzo_1009.lua) | dictionaries | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the dictionaries table |
| [1010](/elements/002-helium/acuranzo/migrations/acuranzo_1010.lua) | documents | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the documents table |
| [1011](/elements/002-helium/acuranzo/migrations/acuranzo_1011.lua) | languages | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the languages table |
| [1012](/elements/002-helium/acuranzo/migrations/acuranzo_1012.lua) | licenses | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the licenses table |
| [1013](/elements/002-helium/acuranzo/migrations/acuranzo_1013.lua) | lists | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the lists table |
| [1014](/elements/002-helium/acuranzo/migrations/acuranzo_1014.lua) | notes | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the notes table |
| [1015](/elements/002-helium/acuranzo/migrations/acuranzo_1015.lua) | reports | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the reports table |
| [1016](/elements/002-helium/acuranzo/migrations/acuranzo_1016.lua) | roles | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the roles table |
| [1017](/elements/002-helium/acuranzo/migrations/acuranzo_1017.lua) | rules | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the rules table |
| [1018](/elements/002-helium/acuranzo/migrations/acuranzo_1018.lua) | sessions | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the sessions table |
| [1019](/elements/002-helium/acuranzo/migrations/acuranzo_1019.lua) | systems | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the systems table |
| [1020](/elements/002-helium/acuranzo/migrations/acuranzo_1020.lua) | templates | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the templates table |
| [1021](/elements/002-helium/acuranzo/migrations/acuranzo_1021.lua) | tokens | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the tokens table |
| [1022](/elements/002-helium/acuranzo/migrations/acuranzo_1022.lua) | workflow_steps | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the workflow_steps table |
| [1023](/elements/002-helium/acuranzo/migrations/acuranzo_1023.lua) | workflows | 3.1.0 | 2025-11-23 | 6 | ✓ | Creates the workflows table |
| [1024](/elements/002-helium/acuranzo/migrations/acuranzo_1024.lua) | accounts | 1.0.0 | 2025-11-11 | 4 | ✗ | Creates administrator account |
| [1025](/elements/002-helium/acuranzo/migrations/acuranzo_1025.lua) | lookups | 1.0.0 | 2025-11-20 | 4 | ✗ | Defaults for Lookup 000 - Lookup Items |
| [1026](/elements/002-helium/acuranzo/migrations/acuranzo_1026.lua) | lookups | 1.0.0 | 2025-11-21 | 6 | ✗ | Defaults for Lookup 001 - Lookup Status |
| [1027](/elements/002-helium/acuranzo/migrations/acuranzo_1027.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 002 - Language Status |
| [1028](/elements/002-helium/acuranzo/migrations/acuranzo_1028.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 003 - Dictionary Status |
| [1029](/elements/002-helium/acuranzo/migrations/acuranzo_1029.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 004 - Connection Type |
| [1030](/elements/002-helium/acuranzo/migrations/acuranzo_1030.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 005 - Connected Status |
| [1031](/elements/002-helium/acuranzo/migrations/acuranzo_1031.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 006 - Connection Status |
| [1032](/elements/002-helium/acuranzo/migrations/acuranzo_1032.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 007 - Rule Status |
| [1033](/elements/002-helium/acuranzo/migrations/acuranzo_1033.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 008 - Note Status |
| [1034](/elements/002-helium/acuranzo/migrations/acuranzo_1034.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 009 - Entity Type |
| [1035](/elements/002-helium/acuranzo/migrations/acuranzo_1035.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 010 - Workflow Status |
| [1036](/elements/002-helium/acuranzo/migrations/acuranzo_1036.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 011 - Workflow Step Status |
| [1037](/elements/002-helium/acuranzo/migrations/acuranzo_1037.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 012 - Template Status |
| [1038](/elements/002-helium/acuranzo/migrations/acuranzo_1038.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 013 - License Status |
| [1039](/elements/002-helium/acuranzo/migrations/acuranzo_1039.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 014 - System Status |
| [1040](/elements/002-helium/acuranzo/migrations/acuranzo_1040.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 015 - System Access Type |
| [1041](/elements/002-helium/acuranzo/migrations/acuranzo_1041.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 016 - Account Access Type |
| [1042](/elements/002-helium/acuranzo/migrations/acuranzo_1042.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 017 - IANA Timezone |
| [1043](/elements/002-helium/acuranzo/migrations/acuranzo_1043.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 018 - Contact Type |
| [1044](/elements/002-helium/acuranzo/migrations/acuranzo_1044.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 019 - Authenticate |
| [1045](/elements/002-helium/acuranzo/migrations/acuranzo_1045.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 020 - Contact Status |
| [1046](/elements/002-helium/acuranzo/migrations/acuranzo_1046.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 021 - Feature |
| [1047](/elements/002-helium/acuranzo/migrations/acuranzo_1047.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 022 - Access Type |
| [1048](/elements/002-helium/acuranzo/migrations/acuranzo_1048.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 023 - Access Status |
| [1049](/elements/002-helium/acuranzo/migrations/acuranzo_1049.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 024 - Action Type |
| [1050](/elements/002-helium/acuranzo/migrations/acuranzo_1050.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 025 - Session Status |
| [1051](/elements/002-helium/acuranzo/migrations/acuranzo_1051.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 026 - Session Flag |
| [1052](/elements/002-helium/acuranzo/migrations/acuranzo_1052.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 027 - Query Status |
| [1053](/elements/002-helium/acuranzo/migrations/acuranzo_1053.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 028 - Query Type |
| [1054](/elements/002-helium/acuranzo/migrations/acuranzo_1054.lua) | lookups | 1.0.0 | 2025-11-22 | 6 | ✗ | Defaults for Lookup 029 - Query Quadrant |
| [1055](/elements/002-helium/acuranzo/migrations/acuranzo_1055.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 030 - Query Dialect |
| [1056](/elements/002-helium/acuranzo/migrations/acuranzo_1056.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 031 - List Types |
| [1057](/elements/002-helium/acuranzo/migrations/acuranzo_1057.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 032 - List Status |
| [1058](/elements/002-helium/acuranzo/migrations/acuranzo_1058.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 033 - Login Controls |
| [1059](/elements/002-helium/acuranzo/migrations/acuranzo_1059.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 034 - Role Status |
| [1060](/elements/002-helium/acuranzo/migrations/acuranzo_1060.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 035 - Role Scope |
| [1061](/elements/002-helium/acuranzo/migrations/acuranzo_1061.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 036 - Role Type |
| [1062](/elements/002-helium/acuranzo/migrations/acuranzo_1062.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 037 - Access Status |
| [1063](/elements/002-helium/acuranzo/migrations/acuranzo_1063.lua) | lookups | 1.0.0 | 2025-11-24 | 6 | ✗ | Defaults for Lookup 038 - AI Chat Engines |
| [1064](/elements/002-helium/acuranzo/migrations/acuranzo_1064.lua) | lookups | 1.0.0 | 2025-11-25 | 6 | ✗ | Defaults for Lookup 039 - AI Auditor Engines |
| [1065](/elements/002-helium/acuranzo/migrations/acuranzo_1065.lua) | lookups | 1.0.0 | 2025-11-25 | 6 | ✗ | Defaults for Lookup 040 - Icons |
| [1066](/elements/002-helium/acuranzo/migrations/acuranzo_1066.lua) | lookups | 1.0.0 | 2025-11-26 | 4 | ✗ | Defaults for Lookup 041 - Themes |
| [1067](/elements/002-helium/acuranzo/migrations/acuranzo_1067.lua) | lookups | 1.0.1 | 2025-11-28 | 4 | ✗ | Theme - Default |
| [1068](/elements/002-helium/acuranzo/migrations/acuranzo_1068.lua) | lookups | 1.0.0 | 2025-11-26 | 4 | ✗ | Theme - Bluish |
| [1069](/elements/002-helium/acuranzo/migrations/acuranzo_1069.lua) | lookups | 1.0.0 | 2025-12-04 | 4 | ✗ | Theme - Whiteout |
| [1070](/elements/002-helium/acuranzo/migrations/acuranzo_1070.lua) | lookups | 1.0.0 | 2025-12-16 | 4 | ✗ | Theme - Mustafar |
| [1071](/elements/002-helium/acuranzo/migrations/acuranzo_1071.lua) | lookups | 1.0.0 | 2025-12-17 | 4 | ✗ | Theme - 1977 |
| [1072](/elements/002-helium/acuranzo/migrations/acuranzo_1072.lua) | lookups | 1.0.0 | 2025-12-18 | 4 | ✗ | Theme - Lanboss |
| [1073](/elements/002-helium/acuranzo/migrations/acuranzo_1073.lua) | lookups | 1.0.0 | 2025-12-18 | 4 | ✗ | Theme - Bland |
| [1074](/elements/002-helium/acuranzo/migrations/acuranzo_1074.lua) | lookups | 1.0.0 | 2025-12-18 | 4 | ✗ | Theme - Stormy |
| [1075](/elements/002-helium/acuranzo/migrations/acuranzo_1075.lua) | lookups | 1.0.0 | 2025-12-20 | 6 | ✗ | Defaults for Lookup 042 - Modules |
| [1076](/elements/002-helium/acuranzo/migrations/acuranzo_1076.lua) | lookups | 1.0.0 | 2025-12-21 | 6 | ✗ | Defaults for Lookup 043 - Tours |
| [1077](/elements/002-helium/acuranzo/migrations/acuranzo_1077.lua) | lookups | 1.0.0 | 2025-12-22 | 6 | ✗ | Defaults for Lookup 044 - Server Version History |
| [1078](/elements/002-helium/acuranzo/migrations/acuranzo_1078.lua) | lookups | 1.0.0 | 2025-12-22 | 6 | ✗ | Defaults for Lookup 045 - Client Version History |
| [1079](/elements/002-helium/acuranzo/migrations/acuranzo_1079.lua) | lookups | 1.0.0 | 2025-12-24 | 6 | ✗ | Defaults for Lookup 046 - Macro Expansion |
| [1080](/elements/002-helium/acuranzo/migrations/acuranzo_1080.lua) | lookups | 1.0.0 | 2025-12-24 | 6 | ✗ | Defaults for Lookup 047 - Translations |
| [1081](/elements/002-helium/acuranzo/migrations/acuranzo_1081.lua) | lookups | 1.0.0 | 2025-12-25 | 6 | ✗ | Defaults for Lookup 048 - Module Groups |
| [1082](/elements/002-helium/acuranzo/migrations/acuranzo_1082.lua) | lookups | 1.0.0 | 2025-12-25 | 6 | ✗ | Defaults for Lookup 049 - Document Libraries |
| [1083](/elements/002-helium/acuranzo/migrations/acuranzo_1083.lua) | lookups | 1.0.0 | 2025-12-25 | 6 | ✗ | Defaults for Lookup 050 - Document Status |
| [1084](/elements/002-helium/acuranzo/migrations/acuranzo_1084.lua) | lookups | 1.0.0 | 2025-12-25 | 6 | ✗ | Defaults for Lookup 051 - Document Types |
| [1085](/elements/002-helium/acuranzo/migrations/acuranzo_1085.lua) | lookups | 1.0.0 | 2025-12-25 | 6 | ✗ | Defaults for Lookup 052 - AI Summary Engines |
| [1086](/elements/002-helium/acuranzo/migrations/acuranzo_1086.lua) | lookups | 1.0.0 | 2025-12-25 | 6 | ✗ | Defaults for Lookup 052 - Report Object Types |
| **87** | | | | **507** | **87** | |
