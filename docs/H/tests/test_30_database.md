# Database Subsystem Test (test_30_database.sh)

This test verifies the Database Queue Manager (DQM) startup and functionality across multiple database engines.

## Purpose

The test_30_database.sh script validates that the Database Queue Manager (DQM) starts correctly and operates as intended. It specifically checks for the "DQM-{DatabaseName}-00-{TagLetters} Worker thread started" log message to confirm proper initialization.

## Features

- Parallel testing across multiple database engines (PostgreSQL, MySQL, SQLite, DB2)
- DQM startup verification
- Log message validation
- Configuration testing
- Connectivity checks

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
