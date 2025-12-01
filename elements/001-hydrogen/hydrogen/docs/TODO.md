# Hydrogen Project TODO List

## CRITICAL INSTRUCTIONS - PLEASE FOLLOW FIRST

Hydrogen project. Use elements/001-hydrogen/hydrogen as the base for the project and everything we're doing here.

Review INSTRUCTIONS.md
Review tests/README.md
Review tests/UNITY.md
Review docs/plans/DATABASE_PLAN.md

## Purpose

This TODO.md file serves as a central location to track plenty of incomplete items in the Hydrogen project. Use this file to prioritize and manage development tasks as they are identified and addressed.

NOTE: There is a Tests secion later in this file that covers specifically Unity Framework unit tests that have been commented out due to either a FAIL result or a segfault.
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

## TODO: Tests

These are tetst that have been created but have been excluded, likely because they are either failing or causing a segfault.

At some point, we should review these all and either...

- Fix the test so that the test doesn't fail/crash
- Fix the code so that the test doesn't fail/crash
- Remove the test entirely if it isn't testing anything useful

## Instructions

This is for the AI Model to use to resolve these failing tests.

### Review Background information

DO THIS BEFORE DOING ANYTHING ELSE.

- Review INSTRUCTIONS.md
- Review tests/README.md
- Review tests/UNITY.md
- Review tests/unity/mocks/README.md
- Review docs/CURIOSITIES.md

NOTE: The primary purpose in having tests in the first place is to identify issues in our original
source implmentation. If we've got a test that appears to be written properly with reasonable test
expectations, but is failing because of an issue with the original source, it is absolutely OK and
in fact expected that we'll fix the source bugs rather than mask them with shady passing tests that
hide the problem from subsequent review.

NOTE: Some tests are old and might have been difficult to get working when they were first created.
Since then, many additional mock libraries have been added, and thousands of additional tests are
now available and working that we can look at for examples of how to write far more complex tests.

NOTE: In particular, tests that use malloc were initially disabled as the mock system for malloc was
having problems of all kinds, but this is now working properly and there are many tests that use it.
Comments referencing tests being disabled due to mallloc issues are likely out of data and should be
revisited for implmentation.

NOTE: Some tests were written long ago, before app_config was a thing, so be sure to use code like
that found in src/config/config_defaults.c if app_config is part of the test. Look for examples in
the many other tests that use this same approach to initialize app_config to a known state.

NOTE: Most tests are well-contained, meaning that the result of one test is not in any way dependeent
on the presence or success/failure of another test, certainly in other files but also within the same
file. The files are used primarily to group the tests and the dependencies. But this isn't always so.

### Examine one test at a time

- Run the test with `mku <testname>` to confirm that all the other tests in the file are currently all passing
- Review what the test is trying to accomplish and the resources it needs to work effectively (mocks and so on)
- Make a determination about whether the failing test is valuable enough to keep in the project (default to yes if undecided)

### Remove invaluable tests

- Remove the `RUN_TEST` line and the test function that it is calling
- Run the test again with `mku <testname>` to ensure the removal didn't break other tests

### Fix failing tests

- Remove the `if (0)` from the beginning of the individual test line
- Run the test again with `mku <testname>` to confirm either a FAIL result for the individual test, or a segfault
- Fix the test
- Run the test again with `mku <testname>` to confirm a working test
- Do not leave the test in a non-buildable state or with any tests failing or segfaulting

NOTE: If you encounter a segfault, please take the time to run gdb against the file and get a backtrace to precisely
locate the source of the problem before continuing to troubleshoot.

### After fixing or removing test

- Update TODO.md to remove the test that has been corrected or removed
- Start again with the next test in the section being worked on
- Stop when that section is complete

## Launch

NOTE: Launch tests require registry/subsystem mocking which has architectural limitations.
Tests that verify readiness with mock registry state remain disabled until better mock integration is available.

- tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c:286:    if (0) RUN_TEST(test_check_logging_launch_readiness_console_disabled);  
- tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c:287:    if (0) RUN_TEST(test_check_logging_launch_readiness_file_disabled);
- tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c:290:    if (0) RUN_TEST(test_check_logging_launch_readiness_successful);
- tests/unity/src/launch/launch_logging_test_launch_logging_subsystem.c:78:    if (0) RUN_TEST(test_launch_logging_subsystem_successful_launch);
- tests/unity/src/launch/launch_oidc_test_check_oidc_launch_readiness_with_registry.c:80:    if (0) RUN_TEST(test_check_oidc_launch_readiness_disabled_with_registry_mock);
- tests/unity/src/launch/launch_swagger_test_validation.c:358:    if (0) RUN_TEST(test_check_swagger_launch_readiness_valid_configuration);

## Landing

- tests/unity/src/landing/landing_api_test_readiness.c:100:    if (0) RUN_TEST(test_check_api_landing_readiness_both_running);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:224:    if (0) RUN_TEST(test_land_approved_subsystems_single_ready_subsystem);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:225:    if (0) RUN_TEST(test_land_approved_subsystems_multiple_ready_subsystems);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:226:    if (0) RUN_TEST(test_land_approved_subsystems_registry_skipped);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:227:    if (0) RUN_TEST(test_land_approved_subsystems_not_ready_subsystems_skipped);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:228:    if (0) RUN_TEST(test_land_approved_subsystems_unknown_subsystem_skipped);
- tests/unity/src/landing/landing_test_check_all_landing_readiness.c:238:    if (0) RUN_TEST(test_check_all_landing_readiness_shutdown_success);
- tests/unity/src/landing/landing_test_check_all_landing_readiness.c:239:    if (0) RUN_TEST(test_check_all_landing_readiness_restart_success);
- tests/unity/src/landing/landing_terminal_test_check_terminal_landing_readiness.c:180:    if (0) RUN_TEST(test_check_terminal_landing_readiness_malloc_failure);
- tests/unity/src/landing/landing_swagger_test_check_swagger_landing_readiness.c:163:    if (0) RUN_TEST(test_check_swagger_landing_readiness_webserver_not_running);
- tests/unity/src/landing/landing_payload_test_check_payload_landing_readiness.c:102:    if (0) RUN_TEST(test_check_payload_landing_readiness_memory_allocation_failure);
- tests/unity/src/landing/landing_mdns_client_test_readiness.c:142:    if (0) RUN_TEST(test_check_mdns_client_landing_readiness_network_not_running);
- tests/unity/src/landing/landing_mdns_client_test_readiness.c:143:    if (0) RUN_TEST(test_check_mdns_client_landing_readiness_logging_not_running);

## Logging

- tests/unity/src/logging/logging_test_basic.c:126:    if (0) RUN_TEST(test_count_format_specifiers_single_specifier);
- tests/unity/src/logging/logging_test_basic.c:127:    if (0) RUN_TEST(test_count_format_specifiers_multiple_specifiers);
- tests/unity/src/logging/logging_test_basic.c:129:    if (0) RUN_TEST(test_count_format_specifiers_mixed);
- tests/unity/src/logging/logging_test_basic.c:132:    if (0) RUN_TEST(test_get_fallback_priority_label_valid_priorities);
- tests/unity/src/logging/logging_test_basic.c:133:    if (0) RUN_TEST(test_get_fallback_priority_label_invalid_priority);
