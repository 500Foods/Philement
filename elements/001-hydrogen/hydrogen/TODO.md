# Hydrogen Project TODO List

## CRITICAL INSTRUCTIONS - PLEASE FOLLOW FIRST

Hydrogen project. Use elements/001-hydrogen/hydrogen as the base for the project and everything we're doing here.

Review RECIPE.md
Review tests/README.md
Review tests/UNITY.md
Review docs/plans/DATABASE_PLAN.md

## Purpose

This TODO.md file serves as a central location to track plenty of incomplete items in the Hydrogen project. Use this file to prioritize and manage development tasks as they are identified and addressed.

### Status Folder (4 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/status/status_process.c:297 | Implement message counting for logging metrics |
| 2 | Pending | src/status/status_process.c:302 | Implement request tracking for webserver metrics |
| 3 | Pending | src/status/status_process.c:322 | Implement discovery counting for mDNS metrics |
| 4 | Pending | src/status/status_process.c:327 | Implement job counting for print metrics |

### Print Folder (1 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/print/print_queue_manager.c:66 | Implement actual print job processing |

### API Folder (1 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/api/api_utils.c:364 | Implement actual JWT creation with the provided secret |

### OIDC Folder (5 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/oidc/oidc_service.c:202 | Implement actual authorization flow |
| 2 | Pending | src/oidc/oidc_service.c:247 | Implement actual token request handling |
| 3 | Pending | src/oidc/oidc_service.c:279 | Implement actual userinfo request handling |
| 4 | Pending | src/oidc/oidc_service.c:316 | Implement actual introspection request handling |
| 5 | Pending | src/oidc/oidc_service.c:353 | Implement actual revocation request handling |

### Launch Folder (3 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_notify.c:201 | Add actual notification service initialization |
| 2 | Pending | src/launch/launch_mail_relay.c:162 | Add proper mail relay initialization |
| 3 | Pending | src/launch/launch_mdns_client.c:188 | Implement actual mDNS client initialization |

### Database Folder (18 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/database/postgresql/transaction.c:106 | Generate unique transaction IDs for PostgreSQL |
| 3 | Pending | src/database/postgresql/transaction.c:106 | Generate unique transaction IDs for PostgreSQL |
| 4 | Pending | src/database/database_engine.c:304 | Implement dynamic resizing for database engine |
| 5 | Pending | src/database/database.c:236 | Implement clean shutdown of queue manager and connections |
| 6 | Pending | src/database/database.c:369 | Implement database removal logic |
| 7 | Pending | src/database/database.c:399 | Implement comprehensive health check |
| 8 | Pending | src/database/database.c:417 | Implement query submission to queue system |
| 9 | Pending | src/database/database.c:428 | Implement query status checking |
| 10 | Pending | src/database/database.c:438 | Implement result retrieval |
| 11 | Pending | src/database/database.c:448 | Implement query cancellation |
| 12 | Pending | src/database/database.c:462 | Implement configuration reload |
| 13 | Pending | src/database/database.c:473 | Implement connection testing |
| 14 | Pending | src/database/database.c:506 | Implement API query processing |
| 15 | Pending | src/database/database.c:522 | Implement more comprehensive validation |
| 16 | Pending | src/database/database.c:532 | Implement parameter escaping |
| 17 | Pending | src/database/database.c:542 | Implement query age tracking |
| 18 | Pending | src/database/database.c:552 | Implement result cleanup |
| 19 | Pending | src/database/dbqueue/process.c:113 | Implement actual database query execution (Phase 2) |
| 21 | Pending | src/database/dbqueue/process.c:113 | Implement actual database query execution (Phase 2) |
| 22 | Pending | src/database/dbqueue/process.c:160 | Implement intelligent queue management |

### Queue Folder (1 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/queue/queue.c:182 | Initialize queue with attributes |

### Logging Folder (2 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/logging/log_queue_manager.c:155 | Implement database logging when needed |
| 2 | Pending | src/logging/log_queue_manager.c:168 | Call SMTP send function for notification subsystem |

### Mail Relay Folder

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_mail_relay.c:162 | Add proper mail relay initialization (already included in Launch folder) |

### MDNS Client Folder

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_mdns_client.c:161 | Add proper mDNS client initialization (already included in Launch folder) |
| 2 | Pending | src/launch/launch_mdns_client.c:188 | Implement actual mDNS client initialization (already included in Launch folder) |
