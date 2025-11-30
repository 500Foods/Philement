# TODO: Tests

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

- Update TODO_TESTS.md to remove the test that has been corrected or removed
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

## mDNS Server

- tests/unity/src/mdns/mdns_server_init_test.c:814:    if (0) RUN_TEST(test_mdns_server_init_null_services_array); // Disabled due to double free issues - needs investigation
- tests/unity/src/mdns/mdns_server_init_test.c:815:    if (0) RUN_TEST(test_mdns_server_init_network_failure);
- tests/unity/src/mdns/mdns_server_init_test.c:816:    if (0) RUN_TEST(test_mdns_server_init_malloc_failure);
- tests/unity/src/mdns/mdns_server_init_test.c:817:    if (0) RUN_TEST(test_mdns_server_init_socket_failure);
- tests/unity/src/mdns/mdns_server_init_test.c:818:    if (0) RUN_TEST(test_mdns_server_init_hostname_failure);
- tests/unity/src/mdns/mdns_server_init_test_setup_hostname.c:91:    if (0) RUN_TEST(test_mdns_server_setup_hostname_gethostname_failure);
- tests/unity/src/mdns/mdns_server_init_test_setup_hostname.c:92:    if (0) RUN_TEST(test_mdns_server_setup_hostname_malloc_failure);
- tests/unity/src/mdns/mdns_server_init_test_allocate.c:64:    if (0) RUN_TEST(test_mdns_server_allocate_malloc_failure);

## WebSocket

✅ **SECTION COMPLETED**: All WebSocket tests are now passing! (100% success rate)

**Mock Infrastructure Improvements:**
- Fixed pthread mock architecture: state variables are now global (not static) and shared across compilation units
- Added force-include of mock_pthread.h for websocket source files via CMake
- Restored mock_terminal_websocket functions for terminal tests while avoiding jansson library conflicts
- Initialized session manager in websocket message tests to enable real terminal session creation

**Fixed Tests (6 total):**
- ✅ test_start_websocket_server_pthread_create_failure - pthread mock now works across compilation units
- ✅ test_ws_handle_receive_message_too_large - Fixed buffer size calculation
- ✅ test_authentication_edge_cases - Fixed segfault by nulling authenticated_key after memset(0xFF)
- ✅ test_start_websocket_server_successful_start - pthread mock enabled
- ✅ test_setup_pty_select_valid_fd - Removed invalid timeout assertions
- ✅ test_handle_message_type_terminal_message_processing - Session manager initialization + real terminal processing

**Test Coverage Summary:**
- websocket_server_test_coverage_gaps: 24 tests passing
- websocket_server_message_test_comprehensive: 26 tests passing
- websocket_server_auth_test: 23 tests passing
- websocket_server_test_start_server_error_paths: 5 tests passing
- websocket_server_pty_test_pty_bridge: 11 tests passing
- websocket_server_message_test_handle_message_type: 11 tests passing
