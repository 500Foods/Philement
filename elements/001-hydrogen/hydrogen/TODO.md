# Hydrogen Project TODO List

## CRITICAL INSTRUCTIONS - PLEASE FOLLOW FIRST

Hydrogen project. Use elements/001-hydrogen/hydrogen as the base for the project and everything we're doing here.

Review RECIPE.md
Review tests/README.md
Review tests/UNITY.md
Review docs/plans/DATABASE_PLAN.md

## Purpose

This TODO.md file serves as a central location to track plenty of incomplete items in the Hydrogen project. Use this file to prioritize and manage development tasks as they are identified and addressed.

NOTE: There is also a [TODO TESTS](TODO_TESTS.md) file that covers specifically Unity Framework unit tests that have been commented out due to either a FAIL result or a segfault.
These aren't critical to warrant express attention, but are important enough to track and resolve when time and resources permit.

### API Folder (2 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/api/api_utils.c:364 | Implement actual JWT creation with the provided secret |
| 2 | Pending | src/api/api_utils.c:364 | Remove static flag from create_jwt_token function |

### Database Folder (23 TODOs - Ordered by Difficulty)

#### Easy Tasks (3)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Completed | src/database/database.c:542 | Implement query age tracking (lightweight in-flight tracking) |
| 2 | Completed | src/database/database.c:532 | Implement parameter escaping |
| 3 | Completed | src/database/database.c:473 | Implement connection testing |

#### Easy-Medium Tasks (5)

| # | Status | File | Description |
|---|--------|------|-------------|
| 4 | Pending | src/database/database.c:428 | Implement query status checking |
| 5 | Pending | src/database/database.c:438 | Implement result retrieval |
| 6 | Pending | src/database/database.c:522 | Implement more comprehensive validation |
| 7 | Pending | src/database/database.c:552 | Implement result cleanup |
| 8 | Pending | src/database/database.c:399 | Implement comprehensive health check |

#### Medium Tasks (7)

| # | Status | File | Description |
|---|--------|------|-------------|
| 9 | Pending | src/database/postgresql/transaction.c:106 | Generate unique transaction IDs for PostgreSQL |
| 10 | Pending | src/database/database.c:369 | Implement database removal logic |
| 11 | Pending | src/database/database.c:417 | Implement query submission to queue system |
| 12 | Pending | src/database/database.c:448 | Implement query cancellation |
| 13 | Pending | src/database/database.c:506 | Implement API query processing |
| 14 | Pending | src/database/migration/ | Implement migration tracking table structure for available vs installed migrations |
| 15 | Pending | src/database/dbqueue/lead.c | Update lead.c to handle TestMigration as separate operation mode |

#### Medium-Hard Tasks (4)

| # | Status | File | Description |
|---|--------|------|-------------|
| 16 | Pending | src/database/database.c:236 | Implement clean shutdown of queue manager and connections |
| 17 | Pending | src/database/database.c:462 | Implement configuration reload |
| 18 | Pending | src/database/dbqueue/lead.c | Update lead.c migration workflow to handle both phases as transactions with error stopping |
| 19 | Pending | src/database/migration/ | Add validation that migrations only delete records they added and won't drop non-empty tables |

#### Hard Tasks (4)

| # | Status | File | Description |
|---|--------|------|-------------|
| 20 | Pending | src/database/database_engine.c:304 | Implement dynamic resizing for database engine |
| 21 | Completed | src/database/dbqueue/process.c:113 | Implement actual database query execution (Phase 2) |
| 22 | Pending | src/database/dbqueue/process.c:160 | Implement intelligent queue management |
| 23 | Pending | src/database/dbqueue/lead.c | Design TestMigration mechanism for rollback functionality with built-in safety |
| 24 | Pending | src/database/dbqueue/lead.c | Implement reverse migration execution that stops on SQL errors like other phases |

### Launch Folder (3 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_notify.c:201 | Add actual notification service initialization |
| 2 | Pending | src/launch/launch_mail_relay.c:162 | Add proper mail relay initialization |
| 3 | Pending | src/launch/launch_mdns_client.c:188 | Implement actual mDNS client initialization |

### Logging Folder (4 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/logging/log_queue_manager.c:155 | Implement database logging when needed |
| 2 | Pending | src/logging/log_queue_manager.c:168 | Call SMTP send function for notification subsystem |
| 3 | Pending | src/logging/log_queue_manager.c:155 | Remove static flag from database logging implementation |
| 4 | Pending | src/logging/log_queue_manager.c:168 | Remove static flag from SMTP send function call |

### OIDC Folder (5 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/oidc/oidc_service.c:202 | Implement actual authorization flow |
| 2 | Pending | src/oidc/oidc_service.c:247 | Implement actual token request handling |
| 3 | Pending | src/oidc/oidc_service.c:279 | Implement actual userinfo request handling |
| 4 | Pending | src/oidc/oidc_service.c:316 | Implement actual introspection request handling |
| 5 | Pending | src/oidc/oidc_service.c:353 | Implement actual revocation request handling |

### Print Folder (1 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/print/print_queue_manager.c:66 | Implement actual print job processing |
