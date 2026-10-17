# Shared database scripts

`database.lua` and `database_<engine>.lua` in this folder are the one macro set every Helium design loads. They are not migrations, and this folder is not a schema. The payload packs them once as `helium/database.lua` and `helium/database_<engine>.lua`.

A design's migration files stay in that design's `migrations/` folder. Hydrogen uses these scripts when the payload has no `{design}/database.lua` of its own.

## Files

| File | Purpose |
| ------ | --------- |
| [`database.lua`](/elements/002-helium/helium/database.lua) | Macro expansion and migration runner |
| [`database_postgresql.lua`](/elements/002-helium/helium/database_postgresql.lua) | PostgreSQL configuration |
| [`database_mysql.lua`](/elements/002-helium/helium/database_mysql.lua) | MySQL configuration |
| [`database_mariadb.lua`](/elements/002-helium/helium/database_mariadb.lua) | MariaDB configuration |
| [`database_sqlite.lua`](/elements/002-helium/helium/database_sqlite.lua) | SQLite configuration |
| [`database_db2.lua`](/elements/002-helium/helium/database_db2.lua) | IBM DB2 configuration |
| [`database_firebird.lua`](/elements/002-helium/helium/database_firebird.lua) | Firebird configuration |
| [`database_mssql.lua`](/elements/002-helium/helium/database_mssql.lua) | SQL Server configuration |
