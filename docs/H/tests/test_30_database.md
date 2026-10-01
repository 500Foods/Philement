# Database Subsystem Test (test_30_database.sh)

This test verifies the Database Queue Manager (DQM) startup and functionality across multiple database engines.

## Purpose

The test_30_database.sh script validates that the Database Queue Manager (DQM) starts correctly and operates as intended. It specifically checks for the "DQM-{DatabaseName}-00-{TagLetters} Worker thread started" log message to confirm proper initialization.

## Features

- Parallel testing across eight database engines (PostgreSQL, MySQL, SQLite, DB2, MariaDB, Firebird, YugabyteDB, MSSQL) plus one combined configuration
- YugabyteDB uses the PostgreSQL driver. It is a separate server, reached through `YUGABYTE_DB_*`, not the Acuranzo PostgreSQL connection.
- DQM startup verification
- Log message validation
- Configuration testing
- Connectivity checks

## Engines and ports

Each dedicated config listens on its own web port in the 530x range. AutoMigration stays off. The bootstrap query reads a queries table that is already present: `demo.queries` on MariaDB and YugabyteDB, `demoms.queries` on MSSQL.

| Engine | Web port | Configuration |
| --- | --- | --- |
| PostgreSQL | 5300 | `hydrogen_test_30_postgres.json` |
| MySQL | 5301 | `hydrogen_test_30_mysql.json` |
| SQLite | 5302 | `hydrogen_test_30_sqlite.json` |
| DB2 | 5303 | `hydrogen_test_30_db2.json` |
| Multi (all eight) | 5304 | `hydrogen_test_30_multi.json` |
| Firebird | 5305 | `hydrogen_test_30_firebird.json` |
| MariaDB | 5306 | `hydrogen_test_30_mariadb.json` |
| YugabyteDB | 5307 | `hydrogen_test_30_yugabytedb.json` |
| MSSQL | 5308 | `hydrogen_test_30_mssql.json` |

## Test Coverage

- Database connectivity verification
- DQM initialization timing
- Worker thread startup confirmation
- Multi-engine support validation

## Functions

- check_database_connectivity()
- run_database_test_parallel()
- analyze_database_test_results()
- test_database_configuration()
- check_dqm_startup()
- count_dqm_launches()
- wait_for_dqm_initialization()
